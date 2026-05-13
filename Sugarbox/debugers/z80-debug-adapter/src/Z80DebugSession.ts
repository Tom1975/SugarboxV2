import {
    InitializedEvent,
    TerminatedEvent,
    ContinuedEvent,
    OutputEvent,
} from "vscode-debugadapter";

import { DebugSession } from "vscode-debugadapter";
import { DebugProtocol } from "vscode-debugprotocol";
import { EmulatorClient } from "./EmulatorClient";
import { SymbolTable } from "./SymbolTable";
import { SourceAnnotations } from "./SourceAnnotations";
import { StoppedEvent } from 'vscode-debugadapter';
import { Thread } from 'vscode-debugadapter';
import { StackFrame, Source } from 'vscode-debugadapter';
import { Scope } from 'vscode-debugadapter';
import { Variable } from 'vscode-debugadapter';
import * as cp from "child_process";
import * as fs from "fs";
import * as os from "os";
import * as nodePath from "path";
import * as net from "net";

// One disassembled instruction
interface DisasmLine {
    address: number;
    instruction: string;
    bytes?: number[];
}

// Column width for the mnemonic field in the virtual source view
const COL_INSTR = 20;   // mnemonic + operands, padded to this width

// Maximum Z80 instruction size in bytes (DD CB dd op = 4 bytes)
const MAX_INSTR_BYTES = 4;

// Width of the hex field: "XX XX XX XX" = 4×2 + 3 spaces = 11 chars
const HEX_FIELD_WIDTH = MAX_INSTR_BYTES * 3 - 1;   // 11

/**
 * Format raw bytes for the virtual source view.
 *
 * Returns a "; XX XX  .." style inline comment so the TextMate grammar
 * can colour it as a comment (pale/dim).
 */
function fmtHexAsciiComment(bytes: number[] | undefined): string {
    if (!bytes || bytes.length === 0) return "";
    const hex   = bytes.map(b => b.toString(16).toUpperCase().padStart(2, "0")).join(" ");
    const ascii = bytes.map(b => (b >= 0x20 && b <= 0x7E) ? String.fromCharCode(b) : ".").join("");
    return `; ${hex.padEnd(HEX_FIELD_WIDTH)}  ${ascii}`;
}

/**
 * Format raw bytes for the DAP instructionBytes field.
 *
 * VS Code renders that field in a dedicated column with a distinct style
 * (typically grey/dim) in the Disassembly View.
 *
 * Returns "XX XX  .." (hex padded to HEX_FIELD_WIDTH then two spaces then ASCII).
 */
function fmtInstructionBytes(bytes: number[] | undefined): string | undefined {
    if (!bytes || bytes.length === 0) return undefined;
    const hex   = bytes.map(b => b.toString(16).toUpperCase().padStart(2, "0")).join(" ");
    const ascii = bytes.map(b => (b >= 0x20 && b <= 0x7E) ? String.fromCharCode(b) : ".").join("");
    return `${hex.padEnd(HEX_FIELD_WIDTH)}  ${ascii}`;
}

// A cached disassembly region (one 4KB page of Z80 address space)
interface DisasmRegion {
    sourceRef: number;
    startAddress: number;
    memType: string;   // "read" | "write" | "ram" | "rom" | "cart"
    bank: number;      // -1 = lower/mapped bank; ≥0 = specific bank index
    lines: DisasmLine[];
    addressToLine: Map<number, number>; // address → 1-based text line number
    lineToAddress: Map<number, number>; // text line number → address (instruction lines only)
    text: string;
}

// 16-bit register names (for memoryReference and 4-digit hex formatting)
const REG16 = new Set(["bc", "de", "hl", "sp", "pc", "ix", "iy", "bc'", "de'", "hl'", "af", "af'"]);
// 8-bit register names (2-digit hex)
const REG8  = new Set(["i", "r"]);

export class Z80DebugSession extends DebugSession {

    private emulator = new EmulatorClient();
    private isAttach = false;
    private emulatorProcess: cp.ChildProcess | null = null;

    // Disassembly cache: sourceRef → region
    private disasmCache: Map<number, DisasmRegion> = new Map();
    // Reverse index: "memType:bank:startAddr" → sourceRef (avoids duplicate builds)
    private disasmKeyToRef: Map<string, number> = new Map();
    private disasmRefCounter: number = 1;

    // Symbol table (optional, loaded from symbolFile arg)
    private symbolTable: SymbolTable | null = null;

    // Source annotations (optional, loaded from sourceFile arg)
    private sourceAnnotations: SourceAnnotations | null = null;

    // Global breakpoint registry: key → list of addresses
    // "src:<sourceRef>" for source breakpoints, "instr" for instruction breakpoints
    private bpRegistry: Map<string, number[]> = new Map();

    constructor() {
        super();
        console.log("Z80 Debug Adapter started");
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);
        this.on("stopped", (reason: string) => {
            this.sendEvent(new StoppedEvent(reason, 1));
        });
    }

onStopped(reason: string)
{
    this.sendEvent(new StoppedEvent(reason, 1));
}

protected initializeRequest(
    response: DebugProtocol.InitializeResponse,
    args: DebugProtocol.InitializeRequestArguments
): void {

    console.log("DAP: initialization");
    response.body = {
        supportsConfigurationDoneRequest: true,
        supportsEvaluateForHovers: true,
        supportsSetVariable: true,
        supportsStepBack: false,
        supportsDisassembleRequest: true,
        supportsRestartRequest: true,
        supportsReadMemoryRequest: true,
        supportsWriteMemoryRequest: true,
        supportsFunctionBreakpoints: true,
        supportsCompletionsRequest: true,
    };
    // supportsInlineBreakpoints not in old typings — add via cast
    (response.body as any).supportsInlineBreakpoints = true;

    this.sendResponse(response);
    // InitializedEvent is sent at the end of launchRequest / attachRequest,
    // after the TCP connection to the emulator is established. Sending it here
    // would trigger VS Code to send configurationDone before the socket exists.
}


private loadSymbols(args: any): void {
    console.log(`DAP: loadSymbols — symbolFile=${args.symbolFile ?? "(none)"} sourceFile=${args.sourceFile ?? "(none)"} snapshot=${args.snapshot ?? "(none)"}`);
    if (args.symbolFile) {
        this.symbolTable = SymbolTable.fromRasm(args.symbolFile);
    }
    if (args.sourceFile) {
        this.sourceAnnotations = SourceAnnotations.fromFile(args.sourceFile);
    }
    if (args.snapshot) {
        const { table, breakpoints } = SymbolTable.fromSnapshotRemu(args.snapshot);
        if (this.symbolTable) {
            this.symbolTable.merge(table);
        } else if (table.size > 0) {
            this.symbolTable = table;
        }
        if (breakpoints.length > 0) {
            this.bpRegistry.set("snapshot", breakpoints);
            console.log(`DAP: ${breakpoints.length} breakpoint(s) loaded from snapshot REMU`);
        }
    }
    console.log(`DAP: loadSymbols done — symbolTable=${this.symbolTable?.size ?? "null"} symbols, sourceAnnotations=${this.sourceAnnotations ? "loaded" : "null"}`);
}

protected async launchRequest(
    response: DebugProtocol.LaunchResponse,
    args: any
) {
    console.log("DAP: Launch...");
    this.loadSymbols(args);
    const port = args.port ?? 1234;

    // ── Pre-flight: validate emulator path ────────────────────────────────────
    if (!args.emulator) {
        const msg = "Emulator path not set — configure 'emulator' in launch.json or run Z80 Debug: Configure workspace.\n";
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = "Emulator path not configured";
        this.sendResponse(response);
        return;
    }
    if (!fs.existsSync(args.emulator)) {
        const msg = `Emulator binary not found: "${args.emulator}"\nCheck the 'emulator' field in launch.json or the z80debug.sugarbox setting.\n`;
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = `Emulator not found: ${args.emulator}`;
        this.sendResponse(response);
        return;
    }

    // Build a temporary CSL script for disk/tape (snapshot is loaded via DAP command after connect)
    let cslFile: string | null = null;
    if (args.disk || args.diskB || args.tape) {
        const lines = ["cslversion 2.0"];
        if (args.disk)  lines.push(`disk_insert 0 '${args.disk}'`);
        if (args.diskB) lines.push(`disk_insert 1 '${args.diskB}'`);
        if (args.tape)  lines.push(`tape_insert '${args.tape}'`);
        cslFile = nodePath.join(os.tmpdir(), `sugarbox_${Date.now()}.csl`);
        fs.writeFileSync(cslFile, lines.join("\n") + "\n");
        console.log("DAP: CSL script written to", cslFile);
    }

    // Build Sugarbox arguments
    const spawnArgs: string[] = ["--debug", "--debug_server", String(port)];
    if (cslFile)              spawnArgs.push("--csl", cslFile);
    if (args.cartridge)       spawnArgs.push("--cart", args.cartridge);
    if (args.configuration)   spawnArgs.push("--cfg", args.configuration);
    if (args.hideEmulator)    spawnArgs.push("--hide");

    // Check if the port is already in use before spawning
    const portInUse = await this.isPortInUse(port);
    if (portInUse) {
        const msg = `Port ${port} is already in use — a previous Sugarbox instance may still be running.\n` +
                    `Run: fuser ${port}/tcp  or  ss -tlnp | grep ${port}\n`;
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = `Port ${port} already in use`;
        this.sendResponse(response);
        return;
    }

    // Set cwd to the emulator's own directory so that Sugarbox finds its
    // data files (Sugarbox.ini, ROM/, CONF/) relative to itself, regardless
    // of the VS Code workspace folder.
    const emulatorDir = nodePath.dirname(args.emulator);
    console.log("DAP: Spawning emulator:", args.emulator, spawnArgs.join(" "), "(cwd:", emulatorDir, ")");

    // Track early exit so we can give a more actionable error message.
    let emulatorExitCode: number | null = null;
    let spawnError: string | null = null;
    // Guard: only fire TerminatedEvent once launchRequest has fully completed
    // (i.e. InitializedEvent was sent). If the emulator exits during the launch
    // phase, launchRequest detects it via emulatorExitCode and sends a proper
    // failure response instead.
    let launchCompleted = false;

    this.emulatorProcess = cp.spawn(args.emulator, spawnArgs, {
        stdio: ["ignore", "ignore", "pipe"],
        detached: true,  // GUI process — survit si le parent Node.js est tué
        cwd: emulatorDir
    });
    // Relay emulator stderr to the Debug Console for diagnostics
    this.emulatorProcess.stderr?.on("data", (data: Buffer) => {
        this.sendEvent(new OutputEvent(`[Sugarbox] ${data.toString()}`, "stderr"));
    });
    this.emulatorProcess.on("error", err => {
        spawnError = err.message;
        const msg = `Failed to start emulator "${args.emulator}": ${err.message}\n`;
        console.error(msg);
        this.sendEvent(new OutputEvent(msg, "stderr"));
        if (launchCompleted) this.sendEvent(new TerminatedEvent());
    });
    this.emulatorProcess.on("exit", code => {
        emulatorExitCode = code ?? -1;
        console.log("DAP: Emulator exited with code", code);
        if (launchCompleted) this.sendEvent(new TerminatedEvent());
    });

    // Wait for the TCP debug port to open (up to 10 s)
    try {
        await this.waitForPort(port, 10000);
    } catch (e) {
        // Build a diagnostic message based on what we observed
        let reason: string;
        if (spawnError) {
            reason = `Emulator failed to start: ${spawnError}`;
        } else if (emulatorExitCode !== null) {
            reason = `Emulator exited immediately (code ${emulatorExitCode}) — check the binary and its arguments`;
        } else {
            reason = `Emulator did not open port ${port} within 10 s — check that Sugarbox supports --debug_server`;
        }
        const msg = `Launch failed: ${reason}\nCommand: ${args.emulator} ${spawnArgs.join(" ")}\n`;
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = reason;
        this.sendResponse(response);
        return;
    }

    try {
        await this.emulator.connect(port);
    } catch (e) {
        const msg = `Emulator port ${port} closed unexpectedly after opening — emulator may have crashed.\n`;
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = `Connection to port ${port} failed: ${e}`;
        this.sendResponse(response);
        return;
    }
    console.log("DAP: Connected to emulator");

    // Load snapshot via DAP command — send file content as base64 to avoid
    // path-resolution issues (relative paths, remote machines, etc.)
    if (args.snapshot) {
        console.log("DAP: Loading snapshot", args.snapshot);
        let snapshotData: string;
        try {
            snapshotData = fs.readFileSync(args.snapshot).toString("base64");
        } catch (e: any) {
            const msg = `Cannot read snapshot file "${args.snapshot}": ${e.message}\n`;
            this.sendEvent(new OutputEvent(msg, "stderr"));
            response.success = false;
            (response as any).message = msg.trim();
            this.sendResponse(response);
            return;
        }
        const r = await this.emulator.send({ cmd: "loadSnapshot", data: snapshotData });
        if (r?.status !== "ok") {
            const msg = `Failed to load snapshot: ${r?.message ?? args.snapshot}\n`;
            this.sendEvent(new OutputEvent(msg, "stderr"));
            response.success = false;
            (response as any).message = msg.trim();
            this.sendResponse(response);
            return;
        }
    }

    this.emulator.onEvent = (evt) => {
        if (evt.event === "stopped") {
            const reason = evt.body?.reason ?? "breakpoint";
            console.log("DAP: async stopped event:", reason);
            this.sendEvent(new StoppedEvent(reason, 1));
        } else if (evt.event === "mediaChanged") {
            this.sendEvent({ type: "event", event: "mediaChanged", seq: 0, body: evt.body } as DebugProtocol.Event);
        }
    };

    this.sendResponse(response);
    launchCompleted = true;
    this.sendEvent(new InitializedEvent());
}

// Check once if a port already accepts connections (residual process).
private isPortInUse(port: number, host = "127.0.0.1"): Promise<boolean> {
    return new Promise(resolve => {
        const sock = new net.Socket();
        sock.setTimeout(300);
        sock.connect(port, host, () => { sock.destroy(); resolve(true); });
        sock.on("error", () => { sock.destroy(); resolve(false); });
        sock.on("timeout", () => { sock.destroy(); resolve(false); });
    });
}

// Poll until the TCP port accepts connections, or timeout.
private waitForPort(port: number, timeoutMs: number, host = "127.0.0.1"): Promise<void> {
    return new Promise((resolve, reject) => {
        const deadline = Date.now() + timeoutMs;
        const tryConnect = () => {
            const sock = new net.Socket();
            sock.setTimeout(300);
            sock.connect(port, host, () => {
                sock.destroy();
                resolve();
            });
            sock.on("error", () => {
                sock.destroy();
                if (Date.now() < deadline) setTimeout(tryConnect, 250);
                else reject(new Error(`Port ${port} not available after ${timeoutMs}ms`));
            });
            sock.on("timeout", () => {
                sock.destroy();
                if (Date.now() < deadline) setTimeout(tryConnect, 250);
                else reject(new Error(`Port ${port} timed out after ${timeoutMs}ms`));
            });
        };
        tryConnect();
    });
}

protected async attachRequest(
    response: DebugProtocol.AttachResponse,
    args: any
) {
    console.log("DAP: Attach...");
    this.isAttach = true;
    this.loadSymbols(args);
    const port = args.port ?? 1234;
    try {
        await this.emulator.connect(port);
    } catch (e) {
        const msg = `Cannot attach: no emulator listening on port ${port}.\n` +
                    `Start Sugarbox with: --debug_server ${port}\n`;
        this.sendEvent(new OutputEvent(msg, "stderr"));
        response.success = false;
        (response as any).message = `No emulator on port ${port}`;
        this.sendResponse(response);
        return;
    }
    console.log("DAP: Attached");

    this.emulator.onEvent = (evt) => {
        if (evt.event === "stopped") {
            const reason = evt.body?.reason ?? "breakpoint";
            console.log("DAP: async stopped event:", reason);
            this.sendEvent(new StoppedEvent(reason, 1));
        } else if (evt.event === "mediaChanged") {
            this.sendEvent({ type: "event", event: "mediaChanged", seq: 0, body: evt.body } as DebugProtocol.Event);
        }
    };

    this.sendEvent(new InitializedEvent());
    this.sendResponse(response);
}

protected async configurationDoneRequest(
    response: DebugProtocol.ConfigurationDoneResponse,
    args: DebugProtocol.ConfigurationDoneArguments
) {
    console.log("DAP: configurationDone");
    this.sendResponse(response);

    // Apply all breakpoints (snapshot BPs + any VS Code BPs set during init)
    if (this.bpRegistry.size > 0) {
        await this.flushBreakpoints();
    }

    if (this.isAttach) {
        const state = await this.emulator.send({ cmd: "getState" });
        if (!state?.running) {
            this.sendEvent(new StoppedEvent("pause", 1));
        }
    } else {
        this.sendEvent(new StoppedEvent("entry", 1));
    }
}

private onEmulatorConnected() {
    this.sendEvent(new ContinuedEvent(1, true));
}

protected async continueRequest(
    response: DebugProtocol.ContinueResponse
) {
    console.log("DAP: Continue");
    await this.emulator.send({ cmd: "continue" });
    this.sendResponse(response);
}

protected async nextRequest(
    response: DebugProtocol.NextResponse
) {
    console.log("DAP: Step");
    await this.emulator.send({ cmd: "step" });
    this.sendResponse(response);
    // StoppedEvent will be sent by the async onEvent handler
}

protected async pauseRequest(
    response: DebugProtocol.PauseResponse,
    args: DebugProtocol.PauseArguments
) {
    console.log("DAP: Halt");
    await this.emulator.send({ cmd: "halt" });
    this.sendEvent(new StoppedEvent("pause", 1));
    this.sendResponse(response);
}

protected scopesRequest(response: DebugProtocol.ScopesResponse, args: DebugProtocol.ScopesArguments) {
    console.log("DAP: scopesRequest");
    response.body = {
        scopes: [
            // Variables as register, memory. Maybe memory banks ? tape/disks ? cartridge ?
            new Scope("Registers", 1, false),
            new Scope("Memory", 2, false),
            new Scope("Stack", 3, false)
        ]
    };
    this.sendResponse(response);
}

protected async variablesRequest(
    response: DebugProtocol.VariablesResponse,
    args: DebugProtocol.VariablesArguments
) {
    // REGISTERS
    if (args.variablesReference == 1) {
        const regs = await this.emulator.send({
            cmd: "readRegisters"
        }) as Record<string, number>;

        response.body = {
            variables: Object.entries(regs).map(([name, val]) => {
                const nameLower = name.toLowerCase();
                const is8  = REG8.has(nameLower);
                const is16 = REG16.has(nameLower);
                const hex  = is8
                    ? "0x" + (val & 0xFF).toString(16).padStart(2, "0").toUpperCase()
                    : "0x" + (val & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
                const v: DebugProtocol.Variable = {
                    name,
                    value: hex,
                    variablesReference: 0
                };
                if (is16) {
                    (v as any).memoryReference = hex;
                }
                return v;
            })
        };
    }

    // MEMORY
    else if (args.variablesReference == 2) {
        response.body = {
            variables: [
                {
                    name: "0x0000",
                    value: "<expand>",
                    variablesReference: 0 // TODO
                }
            ]
        };
    }
    // Stack
    else if (args.variablesReference == 3) {
        // 1) Get SP
        const state = await this.emulator.send({ cmd: "getState" });
        const sp = state.sp as number;

        const WORDS = 16;
        const BYTES = WORDS * 2;

        // 2) Lire la mémoire
        const mem = await this.emulator.send({
            cmd: "readMemory",
            address: sp,
            size: BYTES
        }) as number[]; // tableau de bytes

        // 3) Construire les variables
        const vars = [];

        for (let i = 0; i < WORDS; i++) {
            const lo = mem[i * 2];
            const hi = mem[i * 2 + 1];
            const value = lo | (hi << 8);
            const addr = sp + i * 2;

            vars.push({
                name: `SP+${i * 2}`,
                value: `0x${value.toString(16).padStart(4, "0")} @0x${addr.toString(16)}`,
                variablesReference: 0
            });
        }
        response.body = { variables: vars };
    }
    else {
        response.body = { variables: [] };
        this.sendResponse(response);
        return;
    }

    this.sendResponse(response);
}

protected threadsRequest(response: DebugProtocol.ThreadsResponse): void {
    // Pour l'instant, un seul "CPU Z80" fictif
    console.log("DAP: threadsRequest");
    response.body = {
        threads: [new Thread(1, "Z80 CPU")]
    };
    this.sendResponse(response);
}

// ─── Disassembly cache helpers ────────────────────────────────────────────────

// Fetch (or reuse) a disassembly region that contains addr at a valid boundary.
//
// Fast path: find a cached region that already covers addr (same memType/bank).
// Slow path: disassemble 2048 instructions starting from addr.
// memType defaults to "read" (used for stack-trace frames).
private async ensureRegion(addr: number, memType = "read", bank = -1): Promise<DisasmRegion> {
    // Reuse any cached region that already contains addr with the same source
    for (const region of this.disasmCache.values()) {
        if (region.memType === memType && region.bank === bank && region.addressToLine.has(addr)) {
            return region;
        }
    }

    // Allocate a new unique sourceRef
    const sourceRef    = this.disasmRefCounter++;
    const startAddress = addr;

    const reply = await this.emulator.send({
        cmd: "disassemble",
        address: startAddress,
        count: 2048,
        memType,
        bank
    });

    const rawLines: DisasmLine[] = reply.instructions ?? [];
    const addressToLine = new Map<number, number>();
    const lineToAddress = new Map<number, number>();
    let text = "";
    let textLineNo = 0;

    rawLines.forEach((l) => {
        const labels = this.symbolTable?.getLabelsAt(l.address) ?? [];
        if (labels.length > 0) {
            if (text.length > 0) { text += "\n"; textLineNo++; }
            for (const label of labels) {
                const ann = this.sourceAnnotations?.getAnnotation(label);

                // Preamble: comment block from source preceding this label
                if (ann?.preamble.length) {
                    for (const pLine of ann.preamble) {
                        text += `${pLine}\n`;
                        textLineNo++;
                    }
                }

                // Label line, with optional inline comment from source
                const inlineComment = ann?.comment ? `  ${ann.comment}` : "";
                text += `${label}:${inlineComment}\n`;
                textLineNo++;
            }
        }
        textLineNo++;
        addressToLine.set(l.address, textLineNo);
        lineToAddress.set(textLineNo, l.address);
        const addrHex = "0x" + l.address.toString(16).padStart(4, "0");
        const instrPadded = l.instruction.trimEnd().padEnd(COL_INSTR);
        const hexAscii = fmtHexAsciiComment(l.bytes);
        const suffix = hexAscii ? `  ${hexAscii}` : "";
        text += `${addrHex}  ${instrPadded}${suffix}\n`;
    });

    const region: DisasmRegion = { sourceRef, startAddress, memType, bank, lines: rawLines, addressToLine, lineToAddress, text };
    this.disasmCache.set(sourceRef, region);
    this.disasmKeyToRef.set(`${memType}:${bank}:${startAddress}`, sourceRef);
    return region;
}

// Remove all cached regions that contain addr (e.g. after a memory write).
private invalidateRegion(addr: number): void {
    for (const [key, region] of this.disasmCache.entries()) {
        if (region.addressToLine.has(addr)) {
            this.disasmCache.delete(key);
            this.disasmKeyToRef.delete(`${region.memType}:${region.bank}:${region.startAddress}`);
        }
    }
}

// ─── Stack trace ──────────────────────────────────────────────────────────────

// CALL opcodes (3-byte instructions → return address is pushed as PC+3)
private static readonly CALL_OPCODES = new Set([
    0xCD,                                           // CALL nn
    0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC // CALL cc,nn
]);
// RST opcodes (1-byte instructions → return address is pushed as PC+1)
private static readonly RST_OPCODES = new Set([
    0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF
]);

// Return true if addr looks like a CALL/RST return address
// (i.e. addr-3 or addr-1 contains the corresponding opcode).
private async isReturnAddress(addr: number): Promise<boolean> {
    if (addr < 3) return false;
    const mem = await this.emulator.send({ cmd: "readMemory", address: addr - 3, size: 3 });
    const bytes: number[] = mem?.bytes ?? [];
    if (bytes.length < 3) return false;
    return Z80DebugSession.CALL_OPCODES.has(bytes[0])   // CALL at addr-3
        || Z80DebugSession.RST_OPCODES.has(bytes[2]);   // RST  at addr-1
}

// Build a single DAP StackFrame for a given PC and its disassembly region.
private buildStackFrame(id: number, pc: number, region: DisasmRegion): DebugProtocol.StackFrame {
    const pcHex = "0x" + pc.toString(16).padStart(4, "0");
    const lineNo = region.addressToLine.get(pc) ?? 1;
    const labels = this.symbolTable?.getLabelsAt(pc);
    const name = labels?.length ? labels[0] : (id === 0 ? "PC" : `ret #${pcHex}`);
    const hex4 = region.startAddress.toString(16).padStart(4, "0").toUpperCase();
    const sourceName = `Z80 0x${region.startAddress.toString(16).padStart(4, "0")}`;
    // Use the z80disasm:/ URI as path so VS Code navigates to the already-open
    // virtual document instead of opening a new one via sourceRequest.
    const sourcePath = `z80disasm:/${region.memType}/${region.bank}/${hex4}.z80disasm`;
    const frame: DebugProtocol.StackFrame = {
        id,
        name,
        line: lineNo,
        column: 1,
        source: { name: sourceName, path: sourcePath },
        // instructionPointerReference intentionally omitted: its presence causes
        // VS Code to auto-open the native Disassembly View even when a z80disasm:/
        // virtual document is already shown.  Navigation and the execution cursor
        // are fully handled via source.path + line above.
    };
    // memoryReference (non-standard extension) is read by the DebugAdapterTracker
    // in main.ts to obtain the current PC without an extra customRequest.
    (frame as any).memoryReference = pcHex;
    return frame;
}

protected async stackTraceRequest(
    response: DebugProtocol.StackTraceResponse,
    args: DebugProtocol.StackTraceArguments
) {
    console.log("DAP: stackTraceRequest");

    const state = await this.emulator.send({ cmd: "getState" });
    const pc  = state?.pc ?? 0;
    const sp  = state?.sp ?? 0;

    // Frame 0 — current PC
    const region0 = await this.ensureRegion(pc);
    const frames: DebugProtocol.StackFrame[] = [this.buildStackFrame(0, pc, region0)];

    // Walk the Z80 stack: each word is a potential return address pushed by CALL/RST.
    const MAX_DEPTH = 15;
    const memReply = await this.emulator.send({ cmd: "readMemory", address: sp, size: MAX_DEPTH * 2 });
    const bytes: number[] = memReply?.bytes ?? [];

    for (let i = 0; i < MAX_DEPTH && i * 2 + 1 < bytes.length; i++) {
        const retAddr = (bytes[i * 2] | (bytes[i * 2 + 1] << 8)) & 0xFFFF;
        if (!await this.isReturnAddress(retAddr)) continue;
        const region = await this.ensureRegion(retAddr);
        frames.push(this.buildStackFrame(frames.length, retAddr, region));
    }

    response.body = { stackFrames: frames, totalFrames: frames.length };
    this.sendResponse(response);
}

// ─── Virtual source content ───────────────────────────────────────────────────

protected async sourceRequest(
    response: DebugProtocol.SourceResponse,
    args: DebugProtocol.SourceArguments
) {
    console.log("DAP: sourceRequest ref=", args.sourceReference);
    const region = this.disasmCache.get(args.sourceReference);
    if (!region) {
        response.body = { content: "; Region not loaded yet\n" };
        this.sendResponse(response);
        return;
    }
    // text/x-z80-disasm is contributed by this extension in package.json.
    // The mimeType is only honoured after the extension is (re)loaded; before that
    // VS Code falls back to plain text automatically, so this is always safe.
    response.body = { content: region.text, mimeType: "text/x-z80-disasm" };
    this.sendResponse(response);
    console.log(`DAP: sourceRequest — served ${region.lines.length} instructions, ref=${args.sourceReference}`);
}

// ─── Disassembly view ─────────────────────────────────────────────────────────

protected async disassembleRequest(
    response: DebugProtocol.DisassembleResponse,
    args: DebugProtocol.DisassembleArguments
){
    // memoryReference format: "MemoryRead:0xNNNN"
    const parts = args.memoryReference.split(":");
    const type    = parts[0];
    const addrPart = parts.length > 1 ? parts[parts.length - 1] : "0x0000";
    const base    = parseInt(addrPart, 16) || 0;
    const instrOffset: number = (args as any).instructionOffset ?? 0;
    const count   = args.instructionCount ?? 64;

    // When instrOffset < 0 VS Code wants |instrOffset| instructions BEFORE base (= PC).
    // Z80 disassembly is forward-only, so naively going back N*2 bytes risks misalignment:
    // the PC address may end up as the interior of a multi-byte instruction, so it never
    // appears as an instruction start and the arrow lands at the wrong place.
    //
    // Two-pass approach:
    //   Pass 1 — context before PC: disassemble from (base - byteBack); keep only
    //            instructions whose address < base; pad with "???" if not enough.
    //   Pass 2 — from PC onwards: always correct (PC is a known instruction boundary).
    // Result: PC instruction is always at index |instrOffset|, so VS Code places the
    //         arrow correctly.
    //
    // Preamble comment lines are intentionally NOT injected here: inserting multiple
    // items at the same address confuses VS Code's Disassembly View (it creates a
    // visible gap by not requesting enough instructions to fill the viewport).
    // Labels are shown via the `symbol` field only.

    let rawInstructions: Array<{ address: number; instruction: string; bytes?: number[] }>;

    if (instrOffset < 0) {
        const beforeCount = -instrOffset;
        const afterCount  = Math.max(count - beforeCount, 0);
        const byteBack    = Math.min(beforeCount * 2, base);
        const startBefore = (base - byteBack) & 0xFFFF;

        // Pass 1: context before PC (sequential — emulator may not support concurrent)
        const beforeReply = await this.emulator.send({
            cmd: "disassemble", address: startBefore, count: beforeCount + 10, type
        });
        let beforeRaw: Array<{ address: number; instruction: string; bytes?: number[] }> =
            (beforeReply.instructions ?? []).filter((i: any) => (i.address as number) < base);
        beforeRaw = beforeRaw.slice(-beforeCount); // keep closest to PC

        // Pass 2: from PC onwards
        const afterReply = await this.emulator.send({
            cmd: "disassemble", address: base, count: afterCount + 1, type
        });
        const afterRaw = (afterReply.instructions ?? []).slice(0, afterCount + 1);

        // Pad front with dummy entries if the backward disassembly didn't yield enough
        const padCount = beforeCount - beforeRaw.length;
        const padRaw: Array<{ address: number; instruction: string }> = [];
        for (let i = 0; i < padCount; i++) {
            const a = beforeRaw.length > 0
                ? (beforeRaw[0].address - (padCount - i)) & 0xFFFF
                : (base - beforeCount + i) & 0xFFFF;
            padRaw.push({ address: a, instruction: "???" });
        }

        rawInstructions = [...padRaw, ...beforeRaw, ...afterRaw];
        console.log(`DAP: DisassembleRequest (2-pass) — base=${addrPart} instrOffset=${instrOffset} startBefore=0x${startBefore.toString(16)} before=${beforeRaw.length}(+${padCount} pad) after=${afterRaw.length}`);
    } else {
        const startAddress = (base + (args.offset ?? 0)) & 0xFFFF;
        console.log(`DAP: DisassembleRequest — base=${addrPart} → 0x${startAddress.toString(16).padStart(4,"0")} count=${count}`);
        const reply = await this.emulator.send({ cmd: "disassemble", address: startAddress, type, count });
        rawInstructions = reply.instructions ?? [];
    }

    if (!Array.isArray(rawInstructions)) {
        response.body = { instructions: [] };
        this.sendResponse(response);
        return;
    }

    const instructions: DebugProtocol.DisassembledInstruction[] = [];
    for (const ins of rawInstructions) {
        const addrStr = "0x" + (ins.address as number).toString(16);
        const labels  = this.symbolTable?.getLabelsAt(ins.address as number) ?? [];

        const entry: DebugProtocol.DisassembledInstruction = {
            address: addrStr,
            instruction: (ins.instruction ?? "").trimEnd(),
            instructionBytes: fmtInstructionBytes((ins as any).bytes)
        };

        if (labels.length > 0) {
            const label = labels[0];
            const ann   = this.sourceAnnotations?.getAnnotation(label);
            entry.symbol = ann?.comment ? `${label}  ${ann.comment}` : label;
        }

        instructions.push(entry);
    }

    response.body = { instructions };

    this.sendResponse(response);
}

// ─── Breakpoint management ────────────────────────────────────────────────────

// Map a set of DAP source breakpoints to instruction addresses using a region.
// Reads the "0xNNNN" address prefix directly from the region text lines, scanning
// forward from bp.line until an instruction line is found.  This avoids any
// line-number ↔ address mapping skew caused by label/blank lines or VS Code
// remapping the source to a different sourceReference.
private resolveBpsInRegion(
    region: DisasmRegion,
    bps: DebugProtocol.SourceBreakpoint[]
): { addresses: number[]; results: DebugProtocol.Breakpoint[] } {
    const textLines = region.text.split('\n');
    const addresses: number[] = [];
    const results: DebugProtocol.Breakpoint[] = bps.map(bp => {
        // bp.line is 1-based; textLines is 0-based
        for (let li = bp.line - 1; li < textLines.length; li++) {
            const m = textLines[li].match(/^0x([0-9a-fA-F]{4})/i);
            if (m) {
                const addr = parseInt(m[1], 16);
                addresses.push(addr);
                return {
                    verified: true,
                    line: li + 1,
                    instructionReference: "0x" + addr.toString(16).padStart(4, "0")
                };
            }
        }
        return { verified: false, message: "Line out of range" };
    });
    return { addresses, results };
}

// Resolve a z80disasm:/ URI path to the matching cached DisasmRegion (if any).
// Handles both full URIs ("z80disasm:/TYPE/BANK/NNNN.z80disasm") and bare fsPath
// variants ("/TYPE/BANK/NNNN.z80disasm") that VS Code may pass for virtual documents.
private regionFromDisasmPath(path: string): DisasmRegion | undefined {
    // Full URI with scheme — new 3-part format
    let m = path.match(/z80disasm:\/([^/]+)\/(-?\d+)\/([0-9a-fA-F]+)\.z80disasm/i);
    if (!m) m = path.match(/z80disasm:\/([0-9a-fA-F]+)\.z80disasm/i);  // full URI compat
    if (!m) m = path.match(/^\/([^/]+)\/(-?\d+)\/([0-9a-fA-F]+)\.z80disasm$/i); // fsPath 3-part
    if (!m) m = path.match(/^\/([0-9a-fA-F]+)\.z80disasm$/i);                  // fsPath compat
    if (!m) return undefined;

    let memType: string, bank: number, startAddr: number;
    if (m.length >= 4) {
        memType = m[1]; bank = parseInt(m[2], 10); startAddr = parseInt(m[3], 16);
    } else {
        memType = "read"; bank = -1; startAddr = parseInt(m[1], 16);
    }
    const ref = this.disasmKeyToRef.get(`${memType}:${bank}:${startAddr}`);
    return ref !== undefined ? this.disasmCache.get(ref) : undefined;
}

// Merge all registered breakpoints and send the unified list to the emulator.
private async flushBreakpoints(): Promise<void> {
    const allAddresses: number[] = [];
    for (const addrs of this.bpRegistry.values()) {
        allAddresses.push(...addrs);
    }
    // Deduplicate
    const unique = [...new Set(allAddresses)].map(a => ({ address: a }));
    await this.emulator.send({ cmd: "setBreakpoints", breakpoints: unique });
}

// Source breakpoints (virtual disassembly sources)
protected async setBreakpointsRequest(
    response: DebugProtocol.SetBreakpointsResponse,
    args: DebugProtocol.SetBreakpointsArguments
) {
    const sourceRef = args.source.sourceReference ?? 0;
    const bps = args.breakpoints ?? [];
    console.log(`DAP: setBreakpointsRequest — sourceRef=${sourceRef} path=${JSON.stringify(args.source.path)} name=${JSON.stringify(args.source.name)} bps=${JSON.stringify(bps.map(b => ({line: b.line, col: b.column})))}`);

    if (sourceRef === 0) {
        // Check if this is a z80disasm:/ virtual document — resolve via URI
        const srcPath = args.source.path ?? "";
        const region = this.regionFromDisasmPath(srcPath);
        if (region) {
            const { addresses, results } = this.resolveBpsInRegion(region, bps);
            this.bpRegistry.set(`disasm:${region.startAddress}`, addresses);
            await this.flushBreakpoints();
            response.body = { breakpoints: results };
            this.sendResponse(response);
            return;
        }
        // Real source file — not supported
        response.body = {
            breakpoints: bps.map(() => ({ verified: false, message: "Source file mapping not supported" }))
        };
        this.sendResponse(response);
        return;
    }

    const region = this.disasmCache.get(sourceRef);
    if (!region) {
        response.body = {
            breakpoints: bps.map(() => ({ verified: false, message: "Region not loaded" }))
        };
        this.sendResponse(response);
        return;
    }

    const { addresses, results } = this.resolveBpsInRegion(region, bps);
    this.bpRegistry.set(`src:${sourceRef}`, addresses);
    await this.flushBreakpoints();

    response.body = { breakpoints: results };
    this.sendResponse(response);
}

// Instruction breakpoints (VS Code Disassembly View)
protected async setInstructionBreakpointsRequest(
    response: DebugProtocol.SetInstructionBreakpointsResponse,
    args: DebugProtocol.SetInstructionBreakpointsArguments
) {
    const bps = args.breakpoints ?? [];

    const addresses = bps.map(bp => {
        const [type, addrHex] = bp.instructionReference.split(":");
        return parseInt(addrHex, 16) + (bp.offset ?? 0);
    });

    this.bpRegistry.set("instr", addresses);
    await this.flushBreakpoints();

    response.body = {
        breakpoints: addresses.map(() => ({ verified: true }))
    };

    this.sendResponse(response);
}

// Parse a Z80 address from a string: "0xBB5A", "$BB5A", "BB5A", "47962".
// Pure-digit strings are treated as decimal; strings with hex letters as hex.
private static parseAddress(s: string): number | undefined {
    const t = s.trim();
    let m = t.match(/^(?:0x|\$|#)([0-9a-fA-F]{1,4})$/i);
    if (m) { const n = parseInt(m[1], 16); return n <= 0xFFFF ? n : undefined; }
    if (/^[0-9a-fA-F]{1,4}$/.test(t) && /[a-fA-F]/.test(t)) {
        const n = parseInt(t, 16); return n <= 0xFFFF ? n : undefined;
    }
    if (/^\d{1,5}$/.test(t)) { const n = parseInt(t, 10); return n <= 0xFFFF ? n : undefined; }
    return undefined;
}

// Label breakpoints — VS Code "function breakpoints" panel, adapted for Z80 assembly labels.
// Also accepts raw addresses: "0xBB5A", "$BB5A", "BB5A", "47962".
protected async setFunctionBreakpointsRequest(
    response: DebugProtocol.SetFunctionBreakpointsResponse,
    args: DebugProtocol.SetFunctionBreakpointsArguments
) {
    const bps = args.breakpoints ?? [];
    console.log(`DAP: setFunctionBreakpointsRequest — ${bps.length} bp(s): ${JSON.stringify(bps.map(b => b.name))}`);

    const resolved: DebugProtocol.Breakpoint[] = bps.map(bp => {
        const addr = this.symbolTable?.resolveLabel(bp.name) ?? Z80DebugSession.parseAddress(bp.name);
        if (addr === undefined) {
            return { verified: false, message: `Label or address "${bp.name}" not found` };
        }
        return {
            verified: true,
            instructionReference: "0x" + addr.toString(16).padStart(4, "0"),
            message: `0x${addr.toString(16).padStart(4, "0")}`
        };
    });

    const addresses = resolved
        .filter(bp => bp.verified && bp.instructionReference)
        .map(bp => parseInt(bp.instructionReference!.replace("0x", ""), 16));

    this.bpRegistry.set("func", addresses);
    await this.flushBreakpoints();

    response.body = { breakpoints: resolved };
    this.sendResponse(response);
}

protected completionsRequest(
    response: DebugProtocol.CompletionsResponse,
    args: DebugProtocol.CompletionsArguments
): void {
    const prefix = (args.text ?? "").slice(0, (args.column ?? args.text?.length ?? 0) - 1);
    const names = this.symbolTable?.getAllNames() ?? [];
    const lower = prefix.toLowerCase();
    const items: DebugProtocol.CompletionItem[] = names
        .filter(n => n.toLowerCase().startsWith(lower))
        .map(n => ({ label: n, type: "function" as DebugProtocol.CompletionItemType }));
    response.body = { targets: items };
    this.sendResponse(response);
}

protected async stepInRequest(
    response: DebugProtocol.StepInResponse,
    args: DebugProtocol.StepInArguments
){
    await this.emulator.send({ cmd: "stepIn" });
    this.sendResponse(response);
    // StoppedEvent will be sent by the async onEvent handler
}

protected async stepOutRequest(
    response: DebugProtocol.StepOutResponse,
    args: DebugProtocol.StepOutArguments
){
    await this.emulator.send({ cmd: "stepOut" });
    this.sendResponse(response);
    // StoppedEvent will be sent by the async onEvent handler
}

protected async evaluateRequest(
    response: DebugProtocol.EvaluateResponse,
    args: DebugProtocol.EvaluateArguments
){
    try {
        const result = await this.emulator.send({
            cmd: "evaluate",
            expression: args.expression
        });
        response.body = {
            result: result?.text ?? "?",
            variablesReference: 0
        };
    } catch {
        response.body = { result: "?", variablesReference: 0 };
    }
    this.sendResponse(response);
}

protected async disconnectRequest(
    response: DebugProtocol.DisconnectResponse,
    args: DebugProtocol.DisconnectArguments
) {
    try {
        await this.emulator.send({ cmd: "continue" });
    } catch (_) {}
    this.emulator.disconnect();

    // Kill the emulator process if we spawned it (launch mode only)
    if (this.emulatorProcess && !this.emulatorProcess.killed) {
        this.emulatorProcess.kill();
        this.emulatorProcess = null;
    }

    this.sendResponse(response);
}

protected async restartRequest(
    response: DebugProtocol.RestartResponse,
    args: DebugProtocol.RestartArguments
) {
    // Invalidate all disassembly caches (memory may have changed after reset)
    this.disasmCache.clear();
    await this.emulator.send({ cmd: "reset" });
    this.sendResponse(response);
    this.sendEvent(new StoppedEvent("entry", 1));
}

protected async readMemoryRequest(
    response: DebugProtocol.ReadMemoryResponse,
    args: DebugProtocol.ReadMemoryArguments
) {
    // memoryReference: "0x1234" or "MemoryRead:0x1234"
    const ref = args.memoryReference.includes(':')
        ? args.memoryReference.split(':')[1]
        : args.memoryReference;
    const base = parseInt(ref, 16);
    const address = (base + (args.offset ?? 0)) & 0xFFFF;

    const reply = await this.emulator.send({
        cmd: "readMemory",
        address,
        size: args.count
    });

    const bytes: number[] = reply.bytes ?? [];
    response.body = {
        address: "0x" + address.toString(16).padStart(4, "0"),
        data: Buffer.from(bytes).toString("base64")
    };
    this.sendResponse(response);
}

protected async writeMemoryRequest(
    response: DebugProtocol.WriteMemoryResponse,
    args: DebugProtocol.WriteMemoryArguments
) {
    const ref = args.memoryReference.includes(':')
        ? args.memoryReference.split(':')[1]
        : args.memoryReference;
    const base = parseInt(ref, 16);
    const address = (base + (args.offset ?? 0)) & 0xFFFF;

    const bytes = Array.from(Buffer.from(args.data, "base64"));
    await this.emulator.send({ cmd: "writeMemory", address, bytes });

    // Invalidate the disassembly cache for the affected region
    this.invalidateRegion(address);

    response.body = { offset: 0, bytesWritten: bytes.length };
    this.sendResponse(response);
}

protected async setVariableRequest(
    response: DebugProtocol.SetVariableResponse,
    args: DebugProtocol.SetVariableArguments
) {
    if (args.variablesReference !== 1) {
        // Only registers scope is editable
        response.body = { value: args.value, variablesReference: 0 };
        this.sendResponse(response);
        return;
    }

    const val = Number(args.value);  // handles "0x1234" and decimal
    const key = args.name.toLowerCase();  // AF→af, AF'→af', etc.

    await this.emulator.send({ cmd: "setRegisters", [key]: val });

    response.body = {
        value: "0x" + (val & 0xFFFF).toString(16).padStart(4, "0"),
        variablesReference: 0
    };
    this.sendResponse(response);
}

// ─── Custom requests (called from extension via session.customRequest) ────────

private async _forwardHardwareRequest(
    response: DebugProtocol.Response,
    cmd: string,
    errCode: number
): Promise<void> {
    try {
        const result = await this.emulator.send({ cmd });
        response.body = result?.error ? { error: result.error } : result;
        this.sendResponse(response);
    } catch (e) {
        this.sendErrorResponse(response, errCode, `${cmd} failed: ${e}`);
    }
}

protected async customRequest(
    command: string,
    response: DebugProtocol.Response,
    args: any
): Promise<void> {
    if (command === "getDisasmAt") {
        try {
            const addr    = (args?.address  ?? 0) & 0xFFFF;
            const memType = args?.memType   ?? "read";
            const bank    = args?.bank      ?? -1;
            const region = await this.ensureRegion(addr, memType, bank);
            response.body = { text: region.text, sourceRef: region.sourceRef };
            this.sendResponse(response);
        } catch (e) {
            this.sendErrorResponse(response, 1234, `Disassembly failed: ${e}`);
        }
    } else if (command === "getMemBanks") {
        try {
            const result = await this.emulator.send({ cmd: "getMemBanks" });
            // The emulator may return {"error":"unknown command"} for old binaries —
            // that is NOT a throw, so we must check explicitly.
            if (result?.error) {
                console.log(`DAP: getMemBanks not supported by emulator (${result.error}), using defaults`);
                response.body = { sources: null };  // signal "not supported"
            } else {
                const sources = Array.isArray(result?.sources) ? result.sources : [];
                console.log(`DAP: getMemBanks returned ${sources.length} source(s): ` +
                    sources.map((s: any) => s.label).join(", "));
                response.body = { sources };
            }
            this.sendResponse(response);
        } catch (e) {
            console.log(`DAP: getMemBanks threw: ${e}`);
            this.sendErrorResponse(response, 1235, `getMemBanks failed: ${e}`);
        }
    } else if (command === "readMemoryEx") {
        try {
            const address  = (args?.address  ?? 0) & 0xFFFF;
            const count    = args?.count    ?? 256;
            const memType  = args?.memType  ?? "read";
            const bank     = args?.bank     ?? -1;
            const reply = await this.emulator.send({
                cmd: "readMemory",
                address,
                size: count,
                memType,
                bank
            });
            const bytes: number[] = reply?.bytes ?? [];
            response.body = { address, bytes };
            this.sendResponse(response);
        } catch (e) {
            this.sendErrorResponse(response, 1236, `readMemoryEx failed: ${e}`);
        }
    } else if (command === "z80bp") {
        // Accept either a numeric address or a label/address string (from addBreakpointAt)
        let addr: number;
        if (args?.name !== undefined) {
            const resolved = this.symbolTable?.resolveLabel(args.name) ?? Z80DebugSession.parseAddress(args.name);
            if (resolved === undefined) {
                this.sendErrorResponse(response, 1237, `z80bp: unknown label or address: ${args.name}`);
                return;
            }
            addr = resolved & 0xFFFF;
        } else {
            addr = (args?.address ?? 0) & 0xFFFF;
        }
        const enable = args?.enable !== false;
        const current = new Set<number>(this.bpRegistry.get("direct") ?? []);
        if (enable) current.add(addr); else current.delete(addr);
        this.bpRegistry.set("direct", [...current]);
        await this.flushBreakpoints();
        console.log(`DAP: z80bp addr=0x${addr.toString(16).padStart(4,"0")} enable=${enable} → direct set size=${current.size}`);
        response.body = { address: addr, enabled: enable };
        this.sendResponse(response);
    } else if (command === "getCrtcState") {
        await this._forwardHardwareRequest(response, "getCrtcState", 1240);
    } else if (command === "getGateArrayState") {
        await this._forwardHardwareRequest(response, "getGateArrayState", 1241);
    } else if (command === "getPsgState") {
        await this._forwardHardwareRequest(response, "getPsgState", 1242);
    } else if (command === "getPpiState") {
        await this._forwardHardwareRequest(response, "getPpiState", 1243);
    } else if (command === "getFdcState") {
        await this._forwardHardwareRequest(response, "getFdcState", 1244);
    } else if (command === "getTapeState") {
        await this._forwardHardwareRequest(response, "getTapeState", 1245);
    } else if (command === "getAsicState") {
        await this._forwardHardwareRequest(response, "getAsicState", 1246);
    } else if (command === "getTapeSignal") {
        await this._forwardHardwareRequest(response, "getTapeSignal", 1248);
    } else if (command === "getTrackRaw") {
        try {
            const result = await this.emulator.send({ cmd: "getTrackRaw", ...args });
            response.body = result?.error ? { error: result.error } : result;
            this.sendResponse(response);
        } catch (e) {
            this.sendErrorResponse(response, 1247, `getTrackRaw failed: ${e}`);
        }
    } else if (command === "insertDisk") {
        try {
            const result = await this.emulator.send({ cmd: "insertDisk", drive: args.drive ?? 0, path: args.path });
            if (result?.error) {
                this.sendErrorResponse(response, 1248, `insertDisk failed: ${result.error}`);
            } else {
                response.body = result;
                this.sendResponse(response);
            }
        } catch (e) {
            this.sendErrorResponse(response, 1248, `insertDisk failed: ${e}`);
        }
    } else {
        this.sendErrorResponse(response, 1014, `Unknown custom request: ${command}`);
    }
}


}
