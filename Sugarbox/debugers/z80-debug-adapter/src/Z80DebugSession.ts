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
    this.sendEvent(new InitializedEvent());
}


private loadSymbols(args: any): void {
    if (args.symbolFile) {
        this.symbolTable = SymbolTable.fromRasm(args.symbolFile);
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
}

protected async launchRequest(
    response: DebugProtocol.LaunchResponse,
    args: any
) {
    console.log("DAP: Launch...");
    this.loadSymbols(args);
    const port = args.port ?? 1234;

    // Build a temporary CSL script if any media is specified
    let cslFile: string | null = null;
    if (args.disk || args.tape || args.snapshot) {
        const lines = ["cslversion 2.0"];
        if (args.snapshot) lines.push(`snapshot_load '${args.snapshot}'`);
        if (args.disk)     lines.push(`disk_insert 0 '${args.disk}'`);
        if (args.tape)     lines.push(`tape_insert '${args.tape}'`);
        cslFile = nodePath.join(os.tmpdir(), `sugarbox_${Date.now()}.csl`);
        fs.writeFileSync(cslFile, lines.join("\n") + "\n");
        console.log("DAP: CSL script written to", cslFile);
    }

    // Build Sugarbox arguments
    const spawnArgs: string[] = ["--debug", "--debug_server", String(port)];
    if (cslFile)            spawnArgs.push("--csl", cslFile);
    if (args.hideEmulator)  spawnArgs.push("--hide");

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

// Return the sourceReference for the 4KB page containing addr.
// sourceRef = (addr >> 12) + 1  → values 1..16 for the 16 possible 4KB pages.
private pageSourceRef(addr: number): number {
    return (addr >> 12) + 1;
}

private pageStartAddress(addr: number): number {
    return addr & 0xF000;
}

// Fetch and cache the disassembly region for the 4KB page containing addr.
private async ensureRegion(addr: number): Promise<DisasmRegion> {
    const sourceRef = this.pageSourceRef(addr);
    if (this.disasmCache.has(sourceRef)) {
        return this.disasmCache.get(sourceRef)!;
    }

    const startAddress = this.pageStartAddress(addr);
    // 2048 instructions covers ~4KB at average 2 bytes/instruction
    const reply = await this.emulator.send({
        cmd: "disassemble",
        address: startAddress,
        count: 2048
    });

    const rawLines: { address: number; instruction: string }[] = reply.instructions ?? [];
    const addressToLine = new Map<number, number>();
    // lineToAddress maps text line numbers → address (instruction lines only)
    const lineToAddress = new Map<number, number>();
    let text = "";
    let textLineNo = 0; // 1-based, incremented for every emitted line

    rawLines.forEach((l) => {
        // Insert label lines before this instruction if symbols exist at this address
        const labels = this.symbolTable?.getLabelsAt(l.address) ?? [];
        if (labels.length > 0) {
            // Blank separator before the label group (skip at very start)
            if (text.length > 0) {
                text += "\n";
                textLineNo++;
            }
            for (const label of labels) {
                text += `${label}:\n`;
                textLineNo++;
            }
        }

        // Instruction line
        textLineNo++;
        addressToLine.set(l.address, textLineNo);
        lineToAddress.set(textLineNo, l.address);
        const addrHex = "0x" + l.address.toString(16).padStart(4, "0");
        text += `${addrHex}  ${l.instruction.trimEnd()}\n`;
    });

    const region: DisasmRegion = {
        sourceRef,
        startAddress,
        lines: rawLines,
        addressToLine,
        lineToAddress,
        text,
    };
    this.disasmCache.set(sourceRef, region);
    return region;
}

// Invalidate a cached region (e.g., after a reset or memory write)
private invalidateRegion(addr: number): void {
    this.disasmCache.delete(this.pageSourceRef(addr));
}

// ─── Stack trace ──────────────────────────────────────────────────────────────

protected async stackTraceRequest(
    response: DebugProtocol.StackTraceResponse,
    args: DebugProtocol.StackTraceArguments
) {
    console.log("DAP: stackTraceRequest");

    const state = await this.emulator.send({ cmd: "getState" });
    const pc = state?.pc ?? 0;
    const pcHex = "0x" + pc.toString(16).padStart(4, "0");

    // Build or retrieve the virtual disassembly source for this 4KB page
    const region = await this.ensureRegion(pc);
    const lineNo = region.addressToLine.get(pc) ?? 1;

    const sourceName = `Z80 RAM 0x${region.startAddress.toString(16).padStart(4, "0")}`;
    const frame: DebugProtocol.StackFrame = {
        id: 1,
        name: "Z80",
        line: lineNo,
        column: 1,
        source: {
            name: sourceName,
            sourceReference: region.sourceRef,
        },
        instructionPointerReference: "MemoryRead:" + pcHex,
    };
    (frame as any).memoryReference = pcHex;

    response.body = {
        stackFrames: [frame],
        totalFrames: 1
    };

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
    response.body = { content: region.text };
    this.sendResponse(response);
}

// ─── Disassembly view ─────────────────────────────────────────────────────────

protected async disassembleRequest(
    response: DebugProtocol.DisassembleResponse,
    args: DebugProtocol.DisassembleArguments
){
    const [type, bank] = args.memoryReference.split(":");

    const startAddress = (args.offset ?? 0);
    const count = args.instructionCount ?? 64;
    console.log("DAP: DisassembleRequest : Bank : " + type + "/" + bank + " - From "+startAddress+" couting "+count );

    const reply = await this.emulator.send({
        cmd: "disassemble",
        address: startAddress,
        type:type,
        bank:bank,
        count
    });

    const disasm = reply.instructions;

    if (!Array.isArray(disasm)) {
        response.body = { instructions: [] };
        this.sendResponse(response);
        return;
    }

    response.body = {
        instructions: disasm.map((ins: any) => ({
            address: "0x" + ins.address.toString(16),
            instruction: ins.instruction
        }))
    };

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


}
