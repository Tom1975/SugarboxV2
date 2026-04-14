import * as vscode from "vscode";
import * as fs from "fs";
import { Z80DebugSession } from "./Z80DebugSession";

// ─── Disassembly virtual document provider ────────────────────────────────────
// URI scheme: z80disasm:/NNNN.z80disasm  (NNNN = 4-digit hex address)
// Content is fetched from the active debug session via customRequest "getDisasmAt".

class Z80DisasmProvider implements vscode.TextDocumentContentProvider {
    private _onDidChange = new vscode.EventEmitter<vscode.Uri>();
    readonly onDidChange = this._onDidChange.event;

    // Call this to force VS Code to re-fetch a document (e.g. after a step)
    refresh(uri: vscode.Uri): void { this._onDidChange.fire(uri); }

    async provideTextDocumentContent(uri: vscode.Uri): Promise<string> {
        const hex = uri.path.replace(/^\//, "").replace(/\.z80disasm$/, "");
        const addr = parseInt(hex, 16);
        if (isNaN(addr)) return `; Invalid address: ${uri.path}`;

        const session = vscode.debug.activeDebugSession;
        if (!session) return `; No active debug session`;

        try {
            const result = await session.customRequest("getDisasmAt", { address: addr });
            return result?.text ?? `; No disassembly returned`;
        } catch (e) {
            return `; Error: ${e}`;
        }
    }
}

export function activate(context: vscode.ExtensionContext) {

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

    // ── Command: open disassembly at address ──────────────────────────────────
    // - From context menu: uses word/selection under cursor directly (no InputBox)
    // - From command palette or shortcut without valid selection: shows InputBox
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.openDisasmAt", async () => {
            if (!vscode.debug.activeDebugSession) {
                vscode.window.showWarningMessage("Z80 Debug: no active debug session.");
                return;
            }

            // Try to read an address from the current selection or word under cursor
            let addr: number | undefined;
            const editor = vscode.window.activeTextEditor;
            if (editor) {
                // 1. Use highlighted selection if any
                let text = editor.document.getText(editor.selection).trim();
                // 2. Otherwise get the word under cursor
                //    Matches: 0xNNNN  $NNNN  #NNNN  NNNN  (all treated as hex by default)
                if (!text) {
                    const wordRange = editor.document.getWordRangeAtPosition(
                        editor.selection.active,
                        /(?:0x|\$|#)?[0-9a-fA-F]{1,4}/
                    );
                    text = wordRange ? editor.document.getText(wordRange).trim() : "";
                }
                if (text) {
                    // Strip 0x / $ / # prefix — everything is hex by default
                    const digits = text.replace(/^(?:0x|\$|#)/i, "");
                    const n = parseInt(digits, 16);
                    if (!isNaN(n) && n >= 0 && n <= 0xFFFF) {
                        addr = n;
                    }
                }
            }

            // Fall back to InputBox if no valid address found at cursor
            if (addr === undefined) {
                const input = await vscode.window.showInputBox({
                    title: "Désassembler à l'adresse",
                    prompt: "Adresse Z80 16 bits",
                    placeHolder: "0xBB5A  ou  BB5A  ou  47962",
                    validateInput: v => {
                        const raw = v.trim();
                        const n = raw.match(/^\d+$/)
                            ? parseInt(raw, 10)
                            : parseInt(raw.replace(/^0x/i, ""), 16);
                        return (isNaN(n) || n < 0 || n > 0xFFFF)
                            ? "Adresse hex 16 bits (ex: 0xBB5A)" : null;
                    }
                });
                if (input === undefined) return;
                const raw = input.trim();
                addr = raw.match(/^\d+$/)
                    ? parseInt(raw, 10)
                    : parseInt(raw.replace(/^0x/i, ""), 16);
            }

            const hex4 = (addr & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
            const uri = vscode.Uri.parse(`z80disasm:/${hex4}.z80disasm`);
            const doc = await vscode.workspace.openTextDocument(uri);
            await vscode.languages.setTextDocumentLanguage(doc, "z80-disasm");
            await vscode.window.showTextDocument(doc, { preview: false });
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
