import * as vscode from "vscode";
import * as fs from "fs";
import { Z80DebugSession } from "./Z80DebugSession";
import { MemoryViewPanel } from "./MemoryViewPanel";
import { HardwarePanel } from "./HardwarePanel";
import { CrtcAsicPanel } from "./CrtcAsicPanel";
import { GateArrayPanel } from "./GateArrayPanel";
import { PsgPanel } from "./PsgPanel";
import { PpiPanel } from "./PpiPanel";
import { FdcPanel } from "./FdcPanel";
import { TapePanel } from "./TapePanel";
import { HardwarePanelTreeProvider } from "./HardwarePanelTreeProvider";

// ─── Disassembly virtual document provider ────────────────────────────────────
// URI scheme (new):  z80disasm:/TYPE/BANK/NNNN.z80disasm
// URI scheme (compat): z80disasm:/NNNN.z80disasm  (TYPE=read, BANK=-1)
// Content is fetched from the active debug session via customRequest "getDisasmAt".

class Z80DisasmProvider implements vscode.TextDocumentContentProvider {
    private _onDidChange = new vscode.EventEmitter<vscode.Uri>();
    readonly onDidChange = this._onDidChange.event;

    // Call this to force VS Code to re-fetch a document (e.g. after a step)
    refresh(uri: vscode.Uri): void { this._onDidChange.fire(uri); }

    async provideTextDocumentContent(uri: vscode.Uri): Promise<string> {
        // Parse URI path: /TYPE/BANK/NNNN.z80disasm  or  /NNNN.z80disasm (compat)
        const parts = uri.path.replace(/^\//, "").split("/");
        let memType: string;
        let bank: number;
        let hex: string;

        if (parts.length >= 3) {
            // New format: TYPE/BANK/NNNN.z80disasm
            memType = parts[0];
            bank    = parseInt(parts[1], 10);
            if (isNaN(bank)) bank = -1;
            hex = parts[2].replace(/\.z80disasm$/, "");
        } else {
            // Compat format: NNNN.z80disasm
            memType = "read";
            bank    = -1;
            hex = parts[0].replace(/\.z80disasm$/, "");
        }

        const addr = parseInt(hex, 16);
        if (isNaN(addr)) return `; Invalid address: ${uri.path}`;

        const session = vscode.debug.activeDebugSession;
        if (!session) return `; No active debug session`;

        try {
            const result = await session.customRequest("getDisasmAt", { address: addr, memType, bank });
            return result?.text ?? `; No disassembly returned`;
        } catch (e) {
            return `; Error: ${e}`;
        }
    }
}

// ─── Gutter / line decorations for z80disasm:/ documents ─────────────────────
// BP and PC decorations are managed manually because FunctionBreakpoints don't
// appear in the editor gutter natively, and the PC arrow must reflect across
// ALL open z80disasm:/ windows (not just the "main" one VS Code navigates to).

let bpDecoration: vscode.TextEditorDecorationType;
let pcDecoration: vscode.TextEditorDecorationType;

// Local set of active breakpoint addresses (bypasses vscode.debug.addBreakpoints
// which is not forwarded to inline adapters in Remote-WSL).
const bpAddresses = new Set<number>();

// Current PC address when the debugger is stopped (undefined while running).
let currentPcAddress: number | undefined;

function refreshZ80BpDecorations() {
    for (const editor of vscode.window.visibleTextEditors) {
        if (editor.document.languageId !== 'z80-disasm') continue;
        const ranges: vscode.Range[] = [];
        for (let l = 0; l < editor.document.lineCount; l++) {
            const m = editor.document.lineAt(l).text.match(/^0x([0-9a-fA-F]{4})/i);
            if (m && bpAddresses.has(parseInt(m[1], 16))) {
                ranges.push(new vscode.Range(l, 0, l, 0));
            }
        }
        editor.setDecorations(bpDecoration, ranges);
    }
}

// Apply the PC-arrow decoration to every visible z80disasm:/ editor that
// contains the current PC address in its text.  The "main" window (first
// opened, navigated by VS Code via the DAP stack frame) also gets the
// decoration; it visually overlaps with VS Code's built-in frame highlight
// using the same theme colour, so there is no visual artefact.
function refreshPcDecoration() {
    for (const editor of vscode.window.visibleTextEditors) {
        if (editor.document.languageId !== 'z80-disasm') continue;
        const ranges: vscode.Range[] = [];
        if (currentPcAddress !== undefined) {
            for (let l = 0; l < editor.document.lineCount; l++) {
                const m = editor.document.lineAt(l).text.match(/^0x([0-9a-fA-F]{4})/i);
                if (m && parseInt(m[1], 16) === currentPcAddress) {
                    ranges.push(new vscode.Range(l, 0, l, 0));
                    break; // at most one PC per editor
                }
            }
        }
        editor.setDecorations(pcDecoration, ranges);
    }
}

export function activate(context: vscode.ExtensionContext) {

    bpDecoration = vscode.window.createTextEditorDecorationType({
        gutterIconPath: vscode.Uri.joinPath(context.extensionUri, 'images', 'breakpoint.svg'),
        gutterIconSize: 'contain'
    });
    context.subscriptions.push(bpDecoration);

    // PC-arrow decoration: same background colour as VS Code's native frame
    // highlight so it looks consistent in the main window and in secondary ones.
    pcDecoration = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        backgroundColor: new vscode.ThemeColor('editor.stackFrameHighlightBackground'),
        overviewRulerColor: new vscode.ThemeColor('editorOverviewRuler.stackFrameForeground'),
        overviewRulerLane: vscode.OverviewRulerLane.Left
    });
    context.subscriptions.push(pcDecoration);

    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(() => { refreshZ80BpDecorations(); refreshPcDecoration(); }),
        vscode.window.onDidChangeVisibleTextEditors(() => { refreshZ80BpDecorations(); refreshPcDecoration(); })
    );

    // Ensure z80disasm:/ documents opened via stack frame navigation (not by
    // openDisasmAt) get the correct language ID so gutter decorations and
    // syntax highlighting work correctly.
    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(doc => {
            if (doc.uri.scheme === "z80disasm" && doc.languageId !== "z80-disasm") {
                vscode.languages.setTextDocumentLanguage(doc, "z80-disasm").then(
                    () => { refreshZ80BpDecorations(); refreshPcDecoration(); },
                    () => {}
                );
            }
        })
    );

    // ── DebugAdapterTracker ───────────────────────────────────────────────────
    // Responsibilities:
    //   1. Extract the current PC from the stackTrace response (via the non-standard
    //      memoryReference field set by buildStackFrame) and refresh PC decorations.
    //   2. Ensure the frame's z80disasm:/ source document is visible: if it is not
    //      already shown in any editor, open it in the same column as the first
    //      existing z80disasm:/ editor (so it "replaces" the previous view rather
    //      than appearing in a random column or as the native Disassembly view).
    //      Note: instructionPointerReference has been removed from frames to
    //      prevent VS Code from auto-opening the native Disassembly view.
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterTrackerFactory("z80", {
            createDebugAdapterTracker(_session: vscode.DebugSession) {
                return {
                    onDidSendMessage(message: any) {
                        if (message.type === "response" && message.command === "stackTrace") {
                            const frame0 = message.body?.stackFrames?.[0];

                            // PC comes from the custom memoryReference field on the frame
                            // (instructionPointerReference was removed to suppress the
                            // native Disassembly view).
                            const memRef: string | undefined = frame0?.memoryReference;
                            if (memRef) {
                                const addr = parseInt(memRef.replace(/^0x/i, ""), 16);
                                if (!isNaN(addr)) {
                                    currentPcAddress = addr & 0xFFFF;
                                    refreshPcDecoration();
                                }
                            }

                            // Ensure the frame's z80disasm:/ source is visible.
                            // If it is not already open in any editor, open it in
                            // the same column as any existing z80disasm:/ editor so
                            // the "main" window is updated in-place rather than a
                            // new tab appearing in an unrelated column.
                            const framePath: string | undefined = frame0?.source?.path;
                            if (framePath?.startsWith("z80disasm:")) {
                                const frameUri    = vscode.Uri.parse(framePath);
                                const frameUriStr = frameUri.toString();

                                const frameVisible = vscode.window.visibleTextEditors.some(
                                    e => e.document.uri.toString() === frameUriStr
                                );

                                if (!frameVisible) {
                                    // Prefer the column of the first z80disasm:/ editor
                                    const existingCol = vscode.window.visibleTextEditors.find(
                                        e => e.document.uri.scheme === "z80disasm"
                                    )?.viewColumn ?? vscode.ViewColumn.Active;

                                    vscode.workspace.openTextDocument(frameUri)
                                        .then(doc => Promise.resolve(
                                            vscode.window.showTextDocument(doc, {
                                                viewColumn: existingCol,
                                                preview:    false
                                            })
                                        ), () => {});
                                }
                                // If already visible: VS Code's own frame navigation
                                // scrolls it to frame.line automatically.
                            }
                        } else if (
                            message.type === "event" &&
                            message.event === "stopped"
                        ) {
                            HardwarePanel.refreshAll().catch(() => {});
                        } else if (
                            message.type === "event" &&
                            (message.event === "continued" || message.event === "terminated")
                        ) {
                            currentPcAddress = undefined;
                            refreshPcDecoration();
                        }
                    }
                };
            }
        })
    );

    // ── Register debug adapter ────────────────────────────────────────────────
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory(
            "z80",
            {
                createDebugAdapterDescriptor: () =>
                    new vscode.DebugAdapterInlineImplementation(new Z80DebugSession())
            }
        )
    );

    // ── Register disassembly content provider ─────────────────────────────────
    const disasmProvider = new Z80DisasmProvider();
    context.subscriptions.push(
        vscode.workspace.registerTextDocumentContentProvider("z80disasm", disasmProvider)
    );

    // ── Hardware panels TreeView ──────────────────────────────────────────────
    const hwTree = new HardwarePanelTreeProvider();
    context.subscriptions.push(
        vscode.window.registerTreeDataProvider("z80debug.hardwarePanels", hwTree)
    );

    // ── Commands: hardware panels ─────────────────────────────────────────────
    const notReady = (name: string) => () =>
        vscode.window.showInformationMessage(`Z80 Debug: ${name} panel — coming soon.`);

    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.showCrtcPanel",     () => CrtcAsicPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showGateArrayPanel", () => GateArrayPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showPsgPanel",      () => PsgPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showFdcPanel",      () => FdcPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showPpiPanel",      () => PpiPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showTapePanel",      () => TapePanel.createOrShow()),
    );

    // ── Command: open disassembly at address ──────────────────────────────────
    // arg is provided when invoked from debug/variables/context (variable node)
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.openDisasmAt", async (arg?: any) => {
            const session = vscode.debug.activeDebugSession;
            if (!session) {
                vscode.window.showWarningMessage("Z80 Debug: no active debug session.");
                return;
            }

            let addr = addrFromVariableArg(arg) ?? addrFromEditor();

            // Fall back to InputBox
            if (addr === undefined) {
                const input = await vscode.window.showInputBox({
                    title: "Désassembler à l'adresse",
                    prompt: "Adresse Z80 16 bits",
                    placeHolder: "0xBB5A  ou  BB5A  ou  47962",
                    validateInput: validateAddr
                });
                if (input === undefined) return;
                addr = parseAddrInput(input);
            }

            // Ask user to select memory source (QuickPick populated from getMemBanks)
            let memType = "read";
            let bank    = -1;

            try {
                const result = await session.customRequest("getMemBanks");
                const sources: Array<{ type: string; bank: number; label: string }> | null =
                    result?.sources ?? null;

                if (sources && sources.length > 1) {
                    interface SourceItem extends vscode.QuickPickItem {
                        memType: string;
                        srcBank: number;
                    }
                    const items: SourceItem[] = sources.map(s => ({
                        label:       s.label,
                        description: s.type + (s.bank >= 0 ? ` #${s.bank}` : ""),
                        memType:     s.type,
                        srcBank:     s.bank
                    }));

                    const picked = await vscode.window.showQuickPick(items, {
                        title:       "Source mémoire pour le désassemblage",
                        placeHolder: "READ (défaut)"
                    });

                    if (picked === undefined) return; // user cancelled
                    memType = picked.memType;
                    bank    = picked.srcBank;
                } else if (sources && sources.length === 1) {
                    memType = sources[0].type;
                    bank    = sources[0].bank;
                }
            } catch (_) {
                // Old binary or error — fall back to READ
            }

            const hex4 = (addr & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
            const uri  = vscode.Uri.parse(`z80disasm:/${memType}/${bank}/${hex4}.z80disasm`);
            const doc  = await vscode.workspace.openTextDocument(uri);
            await vscode.languages.setTextDocumentLanguage(doc, "z80-disasm");
            await vscode.window.showTextDocument(doc, { preview: false });
        })
    );

    // ── Command: open memory view at address ──────────────────────────────────
    // arg is provided when invoked from debug/variables/context (variable node)
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.openMemoryAt", async (arg?: any) => {
            if (!vscode.debug.activeDebugSession) {
                vscode.window.showWarningMessage("Z80 Debug: no active debug session.");
                return;
            }

            let addr = addrFromVariableArg(arg) ?? addrFromEditor();

            // Fall back to InputBox
            if (addr === undefined) {
                const input = await vscode.window.showInputBox({
                    title: "Mémoire à l'adresse",
                    prompt: "Adresse Z80 16 bits",
                    placeHolder: "0x4000  ou  4000  ou  16384",
                    validateInput: validateAddr
                });
                if (input === undefined) return;
                addr = parseAddrInput(input);
            }

            MemoryViewPanel.createOrShow(addr);
        })
    );

    // ── Command: add breakpoint by address or label ───────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.addBreakpointAt", async () => {
            const session = vscode.debug.activeDebugSession;
            if (!session) {
                vscode.window.showWarningMessage("Z80 Debug: no active debug session.");
                return;
            }
            const input = await vscode.window.showInputBox({
                title: "Ajouter un breakpoint",
                prompt: "Adresse Z80 16 bits ou label assembleur",
                placeHolder: "0xBB5A  ou  BB5A  ou  47962  ou  monLabel",
            });
            if (input === undefined || input.trim() === "") return;
            // Resolve label or address via the adapter (labels need symbolTable)
            try {
                const result = await session.customRequest("z80bp", { name: input.trim(), enable: true });
                if (result?.address !== undefined) {
                    bpAddresses.add(result.address & 0xFFFF);
                    refreshZ80BpDecorations();
                }
            } catch (e) {
                vscode.window.showWarningMessage(`Z80 Debug: breakpoint non ajouté — ${e}`);
            }
        })
    );

    // ── Command: toggle breakpoint on the current line of a z80disasm:/ editor ─
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.toggleBreakpointAt", async () => {
            const session = vscode.debug.activeDebugSession;
            if (!session) {
                vscode.window.showWarningMessage("Z80 Debug: no active debug session.");
                return;
            }
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const lineText = editor.document.lineAt(editor.selection.active.line).text;
            const m = lineText.match(/^0x([0-9a-fA-F]{4})/i);
            if (!m) {
                vscode.window.showWarningMessage("Z80 Debug: aucune adresse sur cette ligne.");
                return;
            }
            const addr   = parseInt(m[1], 16);
            const enable = !bpAddresses.has(addr);
            try {
                await session.customRequest("z80bp", { address: addr, enable });
                if (enable) bpAddresses.add(addr); else bpAddresses.delete(addr);
                refreshZ80BpDecorations();
            } catch (e) {
                vscode.window.showWarningMessage(`Z80 Debug: impossible de modifier le breakpoint — ${e}`);
            }
        })
    );

    // ── Clear local BP/PC state when session ends ─────────────────────────────
    context.subscriptions.push(
        vscode.debug.onDidTerminateDebugSession(() => {
            bpAddresses.clear();
            currentPcAddress = undefined;
            refreshZ80BpDecorations();
            refreshPcDecoration();
        })
    );

    // ── Register configure command ────────────────────────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.configure", configureWorkspace)
    );

    // ── Check configuration at startup ────────────────────────────────────────
    checkConfiguration();
}

export function deactivate() {}

// ─── Address helpers ──────────────────────────────────────────────────────────

/**
 * Try to extract a Z80 address from the argument VS Code passes when a command
 * is invoked via debug/variables/context (right-click on a variable/register).
 *
 * VS Code wraps the variable in an object; the variable itself is at
 * arg.variable (newer API) or directly at arg (older API).  We check
 * memoryReference first (set by variablesRequest for 16-bit registers),
 * then fall back to parsing the value string.
 */
function addrFromVariableArg(arg: any): number | undefined {
    if (!arg || typeof arg !== "object") return undefined;

    // VS Code may wrap the variable under a `variable` property
    const v = arg.variable ?? arg;

    // 1. memoryReference is the most reliable source ("0x1234")
    const ref: string | undefined = v?.memoryReference;
    if (ref) {
        const n = parseInt(ref.replace(/^0x/i, ""), 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF) return n;
    }

    // 2. Parse the display value ("0x1234", "4660", "$1234", "#1234")
    const val: string | undefined = v?.value ?? v?.variable?.value;
    if (val) {
        const digits = String(val).trim().replace(/^(?:0x|\$|#)/i, "");
        const n = parseInt(digits, 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF) return n;
    }

    return undefined;
}

/**
 * Try to extract a Z80 address from the active text editor (selection → word).
 */
function addrFromEditor(): number | undefined {
    const editor = vscode.window.activeTextEditor;
    if (!editor) return undefined;

    let text = editor.document.getText(editor.selection).trim();
    if (!text) {
        const wordRange = editor.document.getWordRangeAtPosition(
            editor.selection.active,
            /(?:0x|\$|#)?[0-9a-fA-F]{1,4}/
        );
        text = wordRange ? editor.document.getText(wordRange).trim() : "";
    }
    if (!text) return undefined;

    const digits = text.replace(/^(?:0x|\$|#)/i, "");
    const n = parseInt(digits, 16);
    return (!isNaN(n) && n >= 0 && n <= 0xFFFF) ? n : undefined;
}

function validateAddr(v: string): string | null {
    const raw = v.trim();
    const n = raw.match(/^\d+$/)
        ? parseInt(raw, 10)
        : parseInt(raw.replace(/^0x/i, ""), 16);
    return (isNaN(n) || n < 0 || n > 0xFFFF) ? "Adresse hex 16 bits (ex: 0x4000)" : null;
}

function parseAddrInput(input: string): number {
    const raw = input.trim();
    return raw.match(/^\d+$/)
        ? parseInt(raw, 10)
        : parseInt(raw.replace(/^0x/i, ""), 16);
}

// ─── Configure workspace ──────────────────────────────────────────────────────

async function configureWorkspace(): Promise<void> {
    const config = vscode.workspace.getConfiguration("z80debug");

    // ── Sugarbox binary ───────────────────────────────────────────────────────
    const sugarboxResult = await vscode.window.showOpenDialog({
        title: "Select Sugarbox emulator binary",
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { "Executable": ["exe"] }
            : { "All files": ["*"] }
    });

    if (!sugarboxResult || sugarboxResult.length === 0) {
        vscode.window.showWarningMessage("Z80 Debug: Sugarbox path not set.");
        return;
    }
    const sugarboxPath = sugarboxResult[0].fsPath;

    // ── RASM binary ───────────────────────────────────────────────────────────
    const rasmResult = await vscode.window.showOpenDialog({
        title: "Select RASM assembler binary",
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { "Executable": ["exe"] }
            : { "All files": ["*"] }
    });

    if (!rasmResult || rasmResult.length === 0) {
        vscode.window.showWarningMessage("Z80 Debug: RASM path not set.");
        return;
    }
    const rasmPath = rasmResult[0].fsPath;

    // ── Write to workspace settings ───────────────────────────────────────────
    await config.update("sugarbox", sugarboxPath, vscode.ConfigurationTarget.Workspace);
    await config.update("rasm",     rasmPath,     vscode.ConfigurationTarget.Workspace);

    vscode.window.showInformationMessage(
        `Z80 Debug: workspace configured.\n  Sugarbox → ${sugarboxPath}\n  RASM → ${rasmPath}`
    );
}

// ─── Startup check ────────────────────────────────────────────────────────────

function checkConfiguration(): void {
    const config = vscode.workspace.getConfiguration("z80debug");
    const sugarbox = config.get<string>("sugarbox", "");

    if (!sugarbox || !fs.existsSync(sugarbox)) {
        vscode.window.showWarningMessage(
            "Z80 Debug: Sugarbox path is not configured.",
            "Configure now"
        ).then(choice => {
            if (choice === "Configure now") {
                vscode.commands.executeCommand("z80debug.configure");
            }
        });
    }
}
