import * as vscode from "vscode";
import * as fs from "fs";
import { Z80DebugSession } from "./Z80DebugSession";

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
