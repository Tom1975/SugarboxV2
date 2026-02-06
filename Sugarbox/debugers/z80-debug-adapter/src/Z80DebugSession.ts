import {
    InitializedEvent,
    TerminatedEvent,
    ContinuedEvent,
} from "vscode-debugadapter";

import { DebugSession } from "vscode-debugadapter";
import { DebugProtocol } from "vscode-debugprotocol";
import { EmulatorClient } from "./EmulatorClient";
import { StoppedEvent } from 'vscode-debugadapter';
import { Thread } from 'vscode-debugadapter';
import { StackFrame, Source } from 'vscode-debugadapter';
import { Scope } from 'vscode-debugadapter';

export class Z80DebugSession extends DebugSession {

    private emulator = new EmulatorClient();

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
        supportsEvaluateForHovers: false,
        supportsSetVariable: false,
        supportsStepBack: false,
        supportsDisassembleRequest: true,
        supportsRestartRequest: true
        
    };

    this.sendResponse(response);
    this.sendEvent(new InitializedEvent());
}


protected async launchRequest(
    response: DebugProtocol.LaunchResponse,
    args: any
) {
    console.log("DAP: Connection...");
    this.emulator.connect(args.port).then(() => {
        console.log("connected");
        this.sendEvent(new InitializedEvent());
        this.sendResponse(response);
    });
}

protected configurationDoneRequest(
    response: DebugProtocol.ConfigurationDoneResponse,
    args: DebugProtocol.ConfigurationDoneArguments
) {
    console.log("DAP: configurationDone");
    this.sendResponse(response);

    this.sendEvent(new StoppedEvent("entry", 1));
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

    this.sendEvent(new StoppedEvent("step", 1));
    this.sendResponse(response);
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
            new Scope("Memory", 2, false)
        ]
    };
    this.sendResponse(response);
}

protected threadsRequest(response: DebugProtocol.ThreadsResponse): void {
    // Pour l’instant, un seul “CPU Z80” fictif
    console.log("DAP: threadsRequest");
    response.body = {
        threads: [new Thread(1, "Z80 CPU")]
    };
    this.sendResponse(response);
}

protected async stackTraceRequest(
    response: DebugProtocol.StackTraceResponse,
    args: DebugProtocol.StackTraceArguments
) {
    console.log("DAP: stackTraceRequest");

    // Demander le PC à l’émulateur
    const state = await this.emulator.send({
        cmd: "getState"
    });
    // state = { pc: number }

    const pc = state?.pc ?? 0;
    const frame: DebugProtocol.StackFrame = {
        id: 1,
        name: "Z80",
        line: 1,
        column: 1,

        // IMPORTANT : PAS de source → disassembly
        instructionPointerReference: "MemoryRead:0x" + pc.toString(16)
    };

    response.body = {
        stackFrames: [frame],
        totalFrames: 1
    };

    this.sendResponse(response);
}

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

protected async variablesRequest(
    response: DebugProtocol.VariablesResponse,
    args: DebugProtocol.VariablesArguments
) {
    console.log("DAP : variablesRequest " + args.variablesReference);
    if (args.variablesReference === 1) {
        const regs = await this.emulator.send({
            cmd: "readRegisters"
        }) as Record<string, number>;

        response.body = {
            variables: Object.keys(regs).map((k) => {
                const v = regs[k];

                return {
                    name: k,
                    value: "0x" + v.toString(16).padStart(4, "0"),
                    variablesReference: 0
                };
            })
        };
    }

    this.sendResponse(response);
}

protected async setInstructionBreakpointsRequest(
    response: DebugProtocol.SetInstructionBreakpointsResponse,
    args: DebugProtocol.SetInstructionBreakpointsArguments
) {
    const bps = args.breakpoints ?? [];

    const breakpoints = bps.map(bp => {
        const [type, addrHex] = bp.instructionReference.split(":");
        const address = parseInt(addrHex, 16) + (bp.offset ?? 0);
        return { type, address };
    });

    // Send full list to emulator
    await this.emulator.send({
        cmd: "setBreakpoints",
        breakpoints
    });

    // Answer to VSCode
    response.body = {
        breakpoints: breakpoints.map(bp => ({
            verified: true
        }))
    };

    this.sendResponse(response);
}

protected async stepInRequest(
    response: DebugProtocol.StepInResponse,
    args: DebugProtocol.StepInArguments
){
    await this.emulator.send({ cmd: "stepIn" });
    this.sendResponse(response);
}

protected async evaluateRequest(
    response: DebugProtocol.EvaluateResponse,
    args: DebugProtocol.EvaluateArguments
){
    const result = await this.emulator.send({
        cmd: "evaluate",
        expression: args.expression
    });

    response.body = {
        result: result.text,
        variablesReference: 0
    };

    this.sendResponse(response);
}

// TODO variableRequest
// TODO disconnectRequest
// TODO restartRequest

// TODO readMemoryRequest
// TODO writeMemoryRequest


}