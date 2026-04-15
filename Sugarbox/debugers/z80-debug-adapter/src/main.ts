import * as vscode from "vscode";
import * as fs from "fs";
import { Z80DebugSession } from "./Z80DebugSession";
import { MemoryViewPanel } from "./MemoryViewPanel";

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
