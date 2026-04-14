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
    lines: DisasmLine[];
    addressToLine: Map<number, number>; // address → 1-based text line number
    lineToAddress: Map<number, number>; // text line number → address (instruction lines only)
    text: string;
}

// 16-bit register names (for memoryReference)
const REG16 = new Set(["bc", "de", "hl", "sp", "pc", "ix", "iy", "bc'", "de'", "hl'"]);

export class Z80DebugSession extends DebugSession {

    private emulator = new EmulatorClient();
    private isAttach = false;
    private emulatorProcess: cp.ChildProcess | null = null;

    // Disassembly cache: sourceRef → region
    private disasmCache: Map<number, DisasmRegion> = new Map();

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
    };

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

    // Build a temporary CSL script for disk/tape (snapshot is loaded via DAP command after connect)
    let cslFile: string | null = null;
    if (args.disk || args.tape) {
        const lines = ["cslversion 2.0"];
        if (args.disk) lines.push(`disk_insert 0 '${args.disk}'`);
        if (args.tape) lines.push(`tape_insert '${args.tape}'`);
        cslFile = nodePath.join(os.tmpdir(), `sugarbox_${Date.now()}.csl`);
        fs.writeFileSync(cslFile, lines.join("\n") + "\n");
        console.log("DAP: CSL script written to", cslFile);
    }

    // Build Sugarbox arguments
    const spawnArgs: string[] = ["--debug", "--debug_server", String(port)];
    if (cslFile)            spawnArgs.push("--csl", cslFile);
    if (args.hideEmulator)  spawnArgs.push("--hide");

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

    console.log("DAP: Spawning emulator:", args.emulator, spawnArgs.join(" "));
    this.emulatorProcess = cp.spawn(args.emulator, spawnArgs, {
        stdio: ["ignore", "ignore", "pipe"],
        detached: true   // GUI process — survit si le parent Node.js est tué
    });
    // Relay emulator stderr to the Debug Console for diagnostics
    this.emulatorProcess.stderr?.on("data", (data: Buffer) => {
        this.sendEvent(new OutputEvent(`[Sugarbox] ${data.toString()}`, "stderr"));
    });
    this.emulatorProcess.on("error", err => {
        const msg = `DAP: Failed to start emulator "${args.emulator}": ${err.message}\n`;
        console.error(msg);
        this.sendEvent(new OutputEvent(msg, "stderr"));
        this.sendEvent(new TerminatedEvent());
    });
    this.emulatorProcess.on("exit", code => {
        console.log("DAP: Emulator exited with code", code);
        this.sendEvent(new TerminatedEvent());
    });

    // Wait for the TCP debug port to open (up to 10 s)
    try {
        await this.waitForPort(port, 10000);
    } catch (e) {
        response.success = false;
        (response as any).message = `Emulator did not open port ${port} in time`;
        this.sendResponse(response);
        return;
    }

    await this.emulator.connect(port);
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
        }
    };

    this.sendResponse(response);
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
    this.emulator.connect(args.port ?? 1234).then(() => {
        console.log("attached");

        this.emulator.onEvent = (evt) => {
            if (evt.event === "stopped") {
                const reason = evt.body?.reason ?? "breakpoint";
                console.log("DAP: async stopped event:", reason);
                this.sendEvent(new StoppedEvent(reason, 1));
            }
        };

        this.sendEvent(new InitializedEvent());
        this.sendResponse(response);
    });
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
                const v: DebugProtocol.Variable = {
                    name,
                    value: "0x" + val.toString(16).padStart(4, "0"),
                    variablesReference: 0
                };
                // Provide memoryReference for 16-bit registers so VS Code can
                // open a Disassembly View or Memory View at that address
                if (REG16.has(name.toLowerCase())) {
                    (v as any).memoryReference = "0x" + val.toString(16).padStart(4, "0");
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
// sourceRef = addr + 1  (unique per starting address, avoids page-sharing bugs
// where two frames in the same 4 KB page would overwrite each other's cache and
// produce wrong line numbers / wrong source content in VS Code).
//
// Fast path: scan existing regions for one that already has addr mapped.
// Slow path: disassemble 2048 instructions starting from addr.
private async ensureRegion(addr: number): Promise<DisasmRegion> {
    // Reuse any cached region that already contains addr
    for (const region of this.disasmCache.values()) {
        if (region.addressToLine.has(addr)) {
            return region;
        }
    }

    // Build a new region anchored at addr
    const sourceRef    = addr + 1;   // unique, positive
    const startAddress = addr;

    const reply = await this.emulator.send({
        cmd: "disassemble",
        address: startAddress,
        count: 2048
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

    const region: DisasmRegion = { sourceRef, startAddress, lines: rawLines, addressToLine, lineToAddress, text };
    this.disasmCache.set(sourceRef, region);
    return region;
}

// Remove all cached regions that contain addr (e.g. after a memory write).
private invalidateRegion(addr: number): void {
    for (const [key, region] of this.disasmCache.entries()) {
        if (region.addressToLine.has(addr)) {
            this.disasmCache.delete(key);
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
    const sourceName = `Z80 0x${region.startAddress.toString(16).padStart(4, "0")}`;
    const frame: DebugProtocol.StackFrame = {
        id,
        name,
        line: lineNo,
        column: 1,
        source: { name: sourceName, sourceReference: region.sourceRef },
        instructionPointerReference: "MemoryRead:" + pcHex,
    };
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
    // args.offset is a BYTE offset relative to that base (DAP spec).
    // We must NOT use args.offset alone as the address.
    const parts = args.memoryReference.split(":");
    const type   = parts[0];
    const addrPart = parts.length > 1 ? parts[parts.length - 1] : "0x0000";
    const base = parseInt(addrPart, 16) || 0;
    const startAddress = (base + (args.offset ?? 0)) & 0xFFFF;
    const count = args.instructionCount ?? 64;
    console.log(`DAP: DisassembleRequest — base=${addrPart} offset=${args.offset ?? 0} → 0x${startAddress.toString(16).padStart(4,"0")} count=${count}`);

    const reply = await this.emulator.send({
        cmd: "disassemble",
        address: startAddress,
        type,
        count
    });

    const disasm = reply.instructions;

    if (!Array.isArray(disasm)) {
        response.body = { instructions: [] };
        this.sendResponse(response);
        return;
    }

    const instructions: DebugProtocol.DisassembledInstruction[] = [];
    for (const ins of disasm) {
        const addrStr = "0x" + ins.address.toString(16);
        const labels = this.symbolTable?.getLabelsAt(ins.address) ?? [];

        // Inject preamble comment lines before the first instruction of a labeled section
        if (labels.length > 0) {
            for (const label of labels) {
                const ann = this.sourceAnnotations?.getAnnotation(label);
                if (ann?.preamble.length) {
                    for (const pLine of ann.preamble) {
                        instructions.push({
                            address: addrStr,
                            instruction: pLine
                        });
                    }
                }
            }
        }

        const entry: DebugProtocol.DisassembledInstruction = {
            address: addrStr,
            instruction: (ins.instruction ?? "").trimEnd(),
            instructionBytes: fmtInstructionBytes(ins.bytes)
        };

        // First label at this address → show in the symbol column
        if (labels.length > 0) {
            const label = labels[0];
            const ann = this.sourceAnnotations?.getAnnotation(label);
            // Append inline comment to the label displayed as symbol
            entry.symbol = ann?.comment
                ? `${label}  ${ann.comment}`
                : label;
        }

        instructions.push(entry);
    }

    response.body = { instructions };

    this.sendResponse(response);
}

// ─── Breakpoint management ────────────────────────────────────────────────────

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

    if (sourceRef === 0) {
        // Real source file — not supported yet
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

    // Map each requested line to an instruction address.
    // If the line is a label/blank line, scan forward to find the next instruction line.
    const resolvedAddresses: number[] = [];
    const resultBps: DebugProtocol.Breakpoint[] = bps.map(bp => {
        let line = bp.line;
        const maxLine = line + region.lines.length; // safety bound
        while (line <= maxLine) {
            const addr = region.lineToAddress.get(line);
            if (addr !== undefined) {
                resolvedAddresses.push(addr);
                return {
                    verified: true,
                    line,
                    instructionReference: "0x" + addr.toString(16).padStart(4, "0")
                };
            }
            line++;
        }
        return { verified: false, message: "Line out of range" };
    });

    this.bpRegistry.set(`src:${sourceRef}`, resolvedAddresses);
    await this.flushBreakpoints();

    response.body = { breakpoints: resultBps };
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

protected async customRequest(
    command: string,
    response: DebugProtocol.Response,
    args: any
): Promise<void> {
    if (command === "getDisasmAt") {
        try {
            const addr = (args?.address ?? 0) & 0xFFFF;
            const region = await this.ensureRegion(addr);
            response.body = { text: region.text, sourceRef: region.sourceRef };
            this.sendResponse(response);
        } catch (e) {
            this.sendErrorResponse(response, 1234, `Disassembly failed: ${e}`);
        }
    } else {
        this.sendErrorResponse(response, 1014, `Unknown custom request: ${command}`);
    }
}


}
