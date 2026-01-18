import * as vscode from "vscode";
import { Z80DebugSession } from "./Z80DebugSession";
import { DebugAdapterInlineImplementation } from "vscode";

export function activate(context: vscode.ExtensionContext) {
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterDescriptorFactory(
            "z80",
            {
                createDebugAdapterDescriptor: () => {
                    return new vscode.DebugAdapterInlineImplementation(
                        new Z80DebugSession()
                    );
                }
            }
        )
    );
}

export function deactivate() {}

