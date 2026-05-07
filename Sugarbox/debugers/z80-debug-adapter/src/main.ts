import * as vscode from "vscode";
import * as fs from "fs";
import * as nodePath from "path";
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
import { initI18n, t } from "./i18n";

// ─── Disassembly virtual document provider ────────────────────────────────────

class Z80DisasmProvider implements vscode.TextDocumentContentProvider {
    private _onDidChange = new vscode.EventEmitter<vscode.Uri>();
    readonly onDidChange = this._onDidChange.event;

    refresh(uri: vscode.Uri): void { this._onDidChange.fire(uri); }

    async provideTextDocumentContent(uri: vscode.Uri): Promise<string> {
        const parts = uri.path.replace(/^\//, "").split("/");
        let memType: string;
        let bank: number;
        let hex: string;

        if (parts.length >= 3) {
            memType = parts[0];
            bank    = parseInt(parts[1], 10);
            if (isNaN(bank)) bank = -1;
            hex = parts[2].replace(/\.z80disasm$/, "");
        } else {
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

// ─── Gutter decorations ───────────────────────────────────────────────────────

let bpDecoration: vscode.TextEditorDecorationType;
let pcDecoration: vscode.TextEditorDecorationType;

const bpAddresses = new Set<number>();
let currentPcAddress: number | undefined;

function refreshZ80BpDecorations() {
    for (const editor of vscode.window.visibleTextEditors) {
        if (editor.document.languageId !== "z80-disasm") continue;
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

function refreshPcDecoration() {
    for (const editor of vscode.window.visibleTextEditors) {
        if (editor.document.languageId !== "z80-disasm") continue;
        const ranges: vscode.Range[] = [];
        if (currentPcAddress !== undefined) {
            for (let l = 0; l < editor.document.lineCount; l++) {
                const m = editor.document.lineAt(l).text.match(/^0x([0-9a-fA-F]{4})/i);
                if (m && parseInt(m[1], 16) === currentPcAddress) {
                    ranges.push(new vscode.Range(l, 0, l, 0));
                    break;
                }
            }
        }
        editor.setDecorations(pcDecoration, ranges);
    }
}

// ─── activate ─────────────────────────────────────────────────────────────────

export function activate(context: vscode.ExtensionContext) {

    // Must be first — all t() calls depend on it
    initI18n(context.extensionPath);

    bpDecoration = vscode.window.createTextEditorDecorationType({
        gutterIconPath: vscode.Uri.joinPath(context.extensionUri, "images", "breakpoint.svg"),
        gutterIconSize: "contain"
    });
    context.subscriptions.push(bpDecoration);

    pcDecoration = vscode.window.createTextEditorDecorationType({
        isWholeLine: true,
        backgroundColor: new vscode.ThemeColor("editor.stackFrameHighlightBackground"),
        overviewRulerColor: new vscode.ThemeColor("editorOverviewRuler.stackFrameForeground"),
        overviewRulerLane: vscode.OverviewRulerLane.Left
    });
    context.subscriptions.push(pcDecoration);

    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(() => { refreshZ80BpDecorations(); refreshPcDecoration(); }),
        vscode.window.onDidChangeVisibleTextEditors(() => { refreshZ80BpDecorations(); refreshPcDecoration(); })
    );

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
    context.subscriptions.push(
        vscode.debug.registerDebugAdapterTrackerFactory("z80", {
            createDebugAdapterTracker(_session: vscode.DebugSession) {
                return {
                    onDidSendMessage(message: any) {
                        if (message.type === "response" && message.command === "stackTrace") {
                            const frame0 = message.body?.stackFrames?.[0];

                            const memRef: string | undefined = frame0?.memoryReference;
                            if (memRef) {
                                const addr = parseInt(memRef.replace(/^0x/i, ""), 16);
                                if (!isNaN(addr)) {
                                    currentPcAddress = addr & 0xFFFF;
                                    refreshPcDecoration();
                                }
                            }

                            const framePath: string | undefined = frame0?.source?.path;
                            if (framePath?.startsWith("z80disasm:")) {
                                const frameUri    = vscode.Uri.parse(framePath);
                                const frameUriStr = frameUri.toString();

                                const frameVisible = vscode.window.visibleTextEditors.some(
                                    e => e.document.uri.toString() === frameUriStr
                                );

                                if (!frameVisible) {
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
                            }
                        } else if (
                            message.type === "event" &&
                            message.event === "stopped"
                        ) {
                            HardwarePanel.refreshAll().catch(() => {});
                        } else if (
                            message.type === "event" &&
                            message.event === "mediaChanged"
                        ) {
                            FdcPanel.currentPanel?.refresh().catch(() => {});
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
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.showCrtcPanel",      () => CrtcAsicPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showGateArrayPanel",  () => GateArrayPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showPsgPanel",        () => PsgPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showFdcPanel",        () => FdcPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showPpiPanel",        () => PpiPanel.createOrShow()),
        vscode.commands.registerCommand("z80debug.showTapePanel",       () => TapePanel.createOrShow()),
    );

    // ── Command: open disassembly at address ──────────────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.openDisasmAt", async (arg?: any) => {
            const session = vscode.debug.activeDebugSession;
            if (!session) {
                vscode.window.showWarningMessage(t("cmd.openDisasmAt.noSession"));
                return;
            }

            let addr = addrFromVariableArg(arg) ?? addrFromEditor();

            if (addr === undefined) {
                const input = await vscode.window.showInputBox({
                    title: t("cmd.openDisasmAt.title"),
                    prompt: t("cmd.openDisasmAt.prompt"),
                    placeHolder: t("cmd.openDisasmAt.placeholder"),
                    validateInput: validateAddr
                });
                if (input === undefined) return;
                addr = parseAddrInput(input);
            }

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
                        title:       t("cmd.openDisasmAt.memSource"),
                        placeHolder: t("cmd.openDisasmAt.memDefault")
                    });

                    if (picked === undefined) return;
                    memType = picked.memType;
                    bank    = picked.srcBank;
                } else if (sources && sources.length === 1) {
                    memType = sources[0].type;
                    bank    = sources[0].bank;
                }
            } catch (_) { /* fall back to READ */ }

            const hex4 = (addr & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
            const uri  = vscode.Uri.parse(`z80disasm:/${memType}/${bank}/${hex4}.z80disasm`);
            const doc  = await vscode.workspace.openTextDocument(uri);
            await vscode.languages.setTextDocumentLanguage(doc, "z80-disasm");
            await vscode.window.showTextDocument(doc, { preview: false });
        })
    );

    // ── Command: open memory view at address ──────────────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.openMemoryAt", async (arg?: any) => {
            if (!vscode.debug.activeDebugSession) {
                vscode.window.showWarningMessage(t("cmd.openMemoryAt.noSession"));
                return;
            }

            let addr = addrFromVariableArg(arg) ?? addrFromEditor();

            if (addr === undefined) {
                const input = await vscode.window.showInputBox({
                    title: t("cmd.openMemoryAt.title"),
                    prompt: t("cmd.openMemoryAt.prompt"),
                    placeHolder: t("cmd.openMemoryAt.placeholder"),
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
                vscode.window.showWarningMessage(t("cmd.addBreakpoint.noSession"));
                return;
            }
            const input = await vscode.window.showInputBox({
                title: t("cmd.addBreakpoint.title"),
                prompt: t("cmd.addBreakpoint.prompt"),
                placeHolder: t("cmd.addBreakpoint.placeholder"),
            });
            if (input === undefined || input.trim() === "") return;
            try {
                const result = await session.customRequest("z80bp", { name: input.trim(), enable: true });
                if (result?.address !== undefined) {
                    bpAddresses.add(result.address & 0xFFFF);
                    refreshZ80BpDecorations();
                }
            } catch (e) {
                vscode.window.showWarningMessage(t("cmd.addBreakpoint.failed", String(e)));
            }
        })
    );

    // ── Command: toggle breakpoint on current line ────────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.toggleBreakpointAt", async () => {
            const session = vscode.debug.activeDebugSession;
            if (!session) {
                vscode.window.showWarningMessage(t("cmd.toggleBreakpoint.noSession"));
                return;
            }
            const editor = vscode.window.activeTextEditor;
            if (!editor) return;
            const lineText = editor.document.lineAt(editor.selection.active.line).text;
            const m = lineText.match(/^0x([0-9a-fA-F]{4})/i);
            if (!m) {
                vscode.window.showWarningMessage(t("cmd.toggleBreakpoint.noAddr"));
                return;
            }
            const addr   = parseInt(m[1], 16);
            const enable = !bpAddresses.has(addr);
            try {
                await session.customRequest("z80bp", { address: addr, enable });
                if (enable) bpAddresses.add(addr); else bpAddresses.delete(addr);
                refreshZ80BpDecorations();
            } catch (e) {
                vscode.window.showWarningMessage(t("cmd.toggleBreakpoint.failed", String(e)));
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

    // ── Commands: configure + project + quick launch ──────────────────────────
    context.subscriptions.push(
        vscode.commands.registerCommand("z80debug.configure",   configureWorkspace),
        vscode.commands.registerCommand("z80debug.newProject",  newProject),
        vscode.commands.registerCommand("z80debug.quickLaunch", quickLaunch)
    );

    checkConfiguration();
}

export function deactivate() {}

// ─── New Project wizard ───────────────────────────────────────────────────────

interface TemplateItem extends vscode.QuickPickItem { id: string; }

async function newProject(): Promise<void> {
    const parentPick = await vscode.window.showOpenDialog({
        title: t("np.folderPicker.title"),
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        openLabel: t("np.folderPicker.label")
    });
    if (!parentPick) return;
    const parentDir = parentPick[0].fsPath;

    const projectName = await vscode.window.showInputBox({
        title: t("np.name.title"),
        placeHolder: t("np.name.placeholder"),
        prompt: t("np.name.prompt"),
        validateInput: v => {
            const s = v.trim();
            if (!s) return t("np.name.errEmpty");
            if (!/^[a-zA-Z0-9_\-]+$/.test(s)) return t("np.name.errChars");
            if (fs.existsSync(nodePath.join(parentDir, s))) return t("np.name.errExists", s);
            return null;
        }
    });
    if (!projectName) return;

    const templateChoice = await vscode.window.showQuickPick<TemplateItem>([
        {
            label: t("np.template.hello.label"),
            description: t("np.template.hello.desc"),
            detail: t("np.template.hello.detail"),
            id: "hello"
        },
        {
            label: t("np.template.empty.label"),
            description: t("np.template.empty.desc"),
            detail: t("np.template.empty.detail"),
            id: "empty"
        }
    ], {
        title: t("np.template.title"),
        matchOnDescription: true
    });
    if (!templateChoice) return;

    const projectDir = nodePath.join(parentDir, projectName);
    const srcDir     = nodePath.join(projectDir, "src");
    const buildDir   = nodePath.join(projectDir, "build");
    const vscodeDir  = nodePath.join(projectDir, ".vscode");

    try {
        fs.mkdirSync(srcDir,    { recursive: true });
        fs.mkdirSync(buildDir,  { recursive: true });
        fs.mkdirSync(vscodeDir, { recursive: true });
    } catch (e: any) {
        vscode.window.showErrorMessage(t("np.errCreate", e.message));
        return;
    }

    const globalCfg    = vscode.workspace.getConfiguration("z80debug");
    const sugarboxPath = globalCfg.get<string>("sugarbox") || "";
    const rasmPath     = globalCfg.get<string>("rasm")     || "rasm";

    fs.writeFileSync(nodePath.join(srcDir, "main.asm"),
        templateChoice.id === "hello" ? templateHello(projectName) : templateEmpty(projectName));

    fs.writeFileSync(nodePath.join(vscodeDir, "tasks.json"),    tasksJson());
    fs.writeFileSync(nodePath.join(vscodeDir, "launch.json"),   launchJson());
    fs.writeFileSync(nodePath.join(vscodeDir, "settings.json"), settingsJson(projectName, sugarboxPath, rasmPath));
    fs.writeFileSync(nodePath.join(projectDir, ".gitignore"),   "build/\n");

    const newUri = vscode.Uri.file(projectDir);
    const choice = await vscode.window.showInformationMessage(
        t("np.created", projectName, projectDir),
        t("np.open"),
        t("np.openNew")
    );
    if (choice === t("np.open")) {
        await vscode.commands.executeCommand("vscode.openFolder", newUri, false);
    } else if (choice === t("np.openNew")) {
        await vscode.commands.executeCommand("vscode.openFolder", newUri, true);
    }
}

// ── ASM templates ─────────────────────────────────────────────────────────────

function templateHello(name: string): string {
    return `\
; ── ${name} ${"─".repeat(Math.max(0, 78 - name.length))}
; ${t("tmpl.built-with")}
; ${t("tmpl.call-from-basic")}
; ${"─".repeat(78)}

        BANKSET 0
        ORG     #8000
        RUN     start

; ── ${t("tmpl.entry-point")} ${"─".repeat(Math.max(0, 70 - t("tmpl.entry-point").length))}
start:
        ld      hl, msg_hello
        call    print_string

        ; ${t("tmpl.infinite-loop")}
loop:
        jr      loop

; ── ${t("tmpl.subroutine")} ${"─".repeat(Math.max(0, 70 - t("tmpl.subroutine").length))}
; ${t("tmpl.subroutine.input")}
print_string:
        ld      a, (hl)
        or      a
        ret     z
        call    TXT_OUTPUT
        inc     hl
        jr      print_string

; ── ${t("tmpl.firmware")} ${"─".repeat(Math.max(0, 70 - t("tmpl.firmware").length))}
TXT_OUTPUT      EQU     #BB5A

; ── ${t("tmpl.data")} ${"─".repeat(Math.max(0, 70 - t("tmpl.data").length))}
msg_hello:
        db      "Hello, World!", 13, 0
`;
}

function templateEmpty(name: string): string {
    return `\
; ── ${name} ${"─".repeat(Math.max(0, 78 - name.length))}
; ${t("tmpl.built-with")}
; ${t("tmpl.call-from-basic")}
; ${"─".repeat(78)}

        BANKSET 0
        ORG     #8000
        RUN     start

; ── ${t("tmpl.entry-point")} ${"─".repeat(Math.max(0, 70 - t("tmpl.entry-point").length))}
start:
        ; ${t("tmpl.your-code")}


        ; ${t("tmpl.bare-loop")}
loop:
        jr      loop
`;
}

function tasksJson(): string {
    return JSON.stringify({
        version: "2.0.0",
        tasks: [
            {
                label: "Create build dir",
                type: "shell",
                command: "mkdir -p '${workspaceFolder}/build'",
                windows: {
                    command: "New-Item -ItemType Directory -Force -Path '${workspaceFolder}\\\\build' | Out-Null"
                },
                presentation: { reveal: "never" },
                problemMatcher: []
            },
            {
                label: "RASM: assemble",
                type: "shell",
                command: "${config:z80debug.rasm}",
                args: [
                    "${workspaceFolder}/${config:z80debug.entryPoint}",
                    "-o",   "${workspaceFolder}/build/${config:z80debug.buildName}",
                    "-oi",  "${workspaceFolder}/build/${config:z80debug.buildName}.sna",
                    "-rasm",
                    "-sq"
                ],
                dependsOn: ["Create build dir"],
                group: { kind: "build", isDefault: true },
                presentation: { reveal: "always", panel: "shared" },
                problemMatcher: []
            }
        ]
    }, null, 2);
}

function launchJson(): string {
    return JSON.stringify({
        version: "0.2.0",
        configurations: [
            {
                type: "z80",
                request: "launch",
                name: t("launch.debugName"),
                emulator: "${config:z80debug.sugarbox}",
                snapshot:   "${workspaceFolder}/build/${config:z80debug.buildName}.sna",
                symbolFile: "${workspaceFolder}/build/${config:z80debug.buildName}.rasm",
                sourceFile: "${workspaceFolder}/${config:z80debug.entryPoint}",
                port: 1234,
                hideEmulator: false,
                preLaunchTask: "RASM: assemble"
            },
            {
                type: "z80",
                request: "attach",
                name: t("launch.attachName"),
                port: 1234,
                symbolFile: "${workspaceFolder}/build/${config:z80debug.buildName}.rasm"
            }
        ]
    }, null, 2);
}

function settingsJson(buildName: string, sugarbox: string, rasm: string): string {
    const s: Record<string, any> = {
        "z80debug.entryPoint": "src/main.asm",
        "z80debug.buildName": buildName,
        "files.associations": { "*.asm": "asm-collection" },
        "[asm-collection]": { "editor.colorDecorators": false },
        "[z80-disasm]":     { "editor.colorDecorators": false }
    };
    if (sugarbox) { s["z80debug.sugarbox"] = sugarbox; }
    if (rasm && rasm !== "rasm") { s["z80debug.rasm"] = rasm; }
    return JSON.stringify(s, null, 2);
}

// ─── Quick Launch wizard ──────────────────────────────────────────────────────

interface MediaItem extends vscode.QuickPickItem { media: string; }
interface ConfigItem extends vscode.QuickPickItem { cfg?: string; }

async function quickLaunch(): Promise<void> {
    const cfg = vscode.workspace.getConfiguration("z80debug");
    let emulatorPath = cfg.get<string>("sugarbox") || "";

    if (!emulatorPath || !fs.existsSync(emulatorPath)) {
        const picked = await vscode.window.showOpenDialog({
            title: t("ql.emulatorPicker.title"),
            canSelectMany: false,
            filters: process.platform === "win32"
                ? { [t("ql.emulatorPicker.exe")]: ["exe"] }
                : { [t("ql.emulatorPicker.all")]: ["*"] }
        });
        if (!picked) return;
        emulatorPath = picked[0].fsPath;
    }

    const folder    = vscode.workspace.workspaceFolders?.[0];
    const buildName = cfg.get<string>("buildName") || "main";
    let projectSna:     string | undefined;
    let projectSymbols: string | undefined;
    let projectDsk:     string | undefined;

    if (folder) {
        const buildDir = nodePath.join(folder.uri.fsPath, "build");
        const snaPath  = nodePath.join(buildDir, `${buildName}.sna`);
        const rasmPath = nodePath.join(buildDir, `${buildName}.rasm`);
        const dskPath  = nodePath.join(buildDir, `${buildName}.dsk`);
        if (fs.existsSync(snaPath))  projectSna     = snaPath;
        if (fs.existsSync(rasmPath)) projectSymbols = rasmPath;
        if (fs.existsSync(dskPath))  projectDsk     = dskPath;
    }

    const symSuffix = projectSymbols ? t("ql.media.symbols.suffix") : "";

    const mediaItems: MediaItem[] = [
        ...(projectSna ? [{
            label:       t("ql.media.projectSnapshot"),
            description: t("ql.media.projectSnapshot.desc", buildName, symSuffix),
            detail:      projectSna,
            media:       "projectSnapshot"
        }] : []),
        ...(projectSna ? [{
            label:       t("ql.media.projectSnapshotBuild"),
            description: t("ql.media.projectSnapshotBuild.desc", buildName),
            detail:      t("ql.media.projectSnapshotBuild.detail"),
            media:       "projectSnapshotBuild"
        }] : []),
        ...(projectDsk ? [{
            label:       t("ql.media.projectDisk"),
            description: t("ql.media.projectDisk.desc", buildName, symSuffix),
            detail:      projectDsk,
            media:       "projectDisk"
        }] : []),
        { label: t("ql.media.empty"),    description: t("ql.media.empty.desc"),    media: "empty"     },
        { label: t("ql.media.diskA"),    description: t("ql.media.diskA.desc"),    media: "disk"      },
        { label: t("ql.media.diskB"),    description: t("ql.media.diskB.desc"),    media: "diskB"     },
        { label: t("ql.media.tape"),     description: t("ql.media.tape.desc"),     media: "tape"      },
        { label: t("ql.media.snapshot"), description: t("ql.media.snapshot.desc"), media: "snapshot"  },
        { label: t("ql.media.cartridge"),description: t("ql.media.cartridge.desc"),media: "cartridge" },
    ];

    const mediaChoice = await vscode.window.showQuickPick(mediaItems, {
        title: t("ql.media.title"),
        matchOnDescription: true,
        matchOnDetail: true
    });
    if (!mediaChoice) return;

    const launchCfg: vscode.DebugConfiguration = {
        type: "z80",
        request: "launch",
        name: t("ql.launchName"),
        emulator: emulatorPath,
        port: 1234,
        hideEmulator: false,
    };

    switch (mediaChoice.media) {
        case "projectSnapshot":
            launchCfg.snapshot = projectSna;
            if (projectSymbols) launchCfg.symbolFile = projectSymbols;
            break;
        case "projectSnapshotBuild":
            launchCfg.snapshot      = projectSna;
            launchCfg.preLaunchTask = "RASM: assemble";
            if (projectSymbols) launchCfg.symbolFile = projectSymbols;
            break;
        case "projectDisk":
            launchCfg.disk = projectDsk;
            if (projectSymbols) launchCfg.symbolFile = projectSymbols;
            break;
        case "empty":
            break;
        default: {
            const filterMap: Record<string, { [name: string]: string[] }> = {
                disk:      { [t("ql.filePicker.disk")]:      ["dsk"] },
                diskB:     { [t("ql.filePicker.disk")]:      ["dsk"] },
                tape:      { [t("ql.filePicker.tape")]:      ["cdt", "wav", "tzx"] },
                snapshot:  { [t("ql.filePicker.snapshot")]:  ["sna"] },
                cartridge: { [t("ql.filePicker.cartridge")]: ["cpr"] },
            };
            const files = await vscode.window.showOpenDialog({
                title: t("ql.filePicker.title"),
                canSelectMany: false,
                filters: filterMap[mediaChoice.media] ?? { [t("ql.filePicker.all")]: ["*"] }
            });
            if (!files) return;
            (launchCfg as any)[mediaChoice.media] = files[0].fsPath;
            break;
        }
    }

    const configItems: ConfigItem[] = [
        { label: t("ql.config.cpc6128"),  description: t("ql.config.cpc6128.desc"), cfg: undefined     },
        { label: t("ql.config.cpc464"),   description: "",                            cfg: "CPC464"      },
        { label: t("ql.config.cpc664"),   description: "",                            cfg: "CPC664"      },
        { label: t("ql.config.cpcplus"),  description: "",                            cfg: "CPC+"        },
        { label: t("ql.config.custom"),   description: t("ql.config.custom.desc"),   cfg: "__custom__"  },
    ];

    const configChoice = await vscode.window.showQuickPick(configItems, {
        title: t("ql.config.title")
    });

    if (configChoice) {
        if (configChoice.cfg === "__custom__") {
            const custom = await vscode.window.showInputBox({
                title: t("ql.config.customInput.title"),
                prompt: t("ql.config.customInput.prompt"),
                placeHolder: t("ql.config.customInput.placeholder")
            });
            if (custom?.trim()) launchCfg.configuration = custom.trim();
        } else if (configChoice.cfg) {
            launchCfg.configuration = configChoice.cfg;
        }
    }

    await vscode.debug.startDebugging(folder, launchCfg);
}

// ─── Configure workspace ──────────────────────────────────────────────────────

async function configureWorkspace(): Promise<void> {
    const config = vscode.workspace.getConfiguration("z80debug");

    const sugarboxResult = await vscode.window.showOpenDialog({
        title: t("cfg.sugarboxPicker.title"),
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { [t("cfg.exe")]: ["exe"] }
            : { [t("cfg.all")]: ["*"] }
    });
    if (!sugarboxResult || sugarboxResult.length === 0) {
        vscode.window.showWarningMessage(t("cfg.warnNoSugarbox"));
        return;
    }
    const sugarboxPath = sugarboxResult[0].fsPath;

    const rasmResult = await vscode.window.showOpenDialog({
        title: t("cfg.rasmPicker.title"),
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { [t("cfg.exe")]: ["exe"] }
            : { [t("cfg.all")]: ["*"] }
    });
    if (!rasmResult || rasmResult.length === 0) {
        vscode.window.showWarningMessage(t("cfg.warnNoRasm"));
        return;
    }
    const rasmPath = rasmResult[0].fsPath;

    await config.update("sugarbox", sugarboxPath, vscode.ConfigurationTarget.Workspace);
    await config.update("rasm",     rasmPath,     vscode.ConfigurationTarget.Workspace);

    vscode.window.showInformationMessage(t("cfg.done", sugarboxPath, rasmPath));
}

// ─── Startup check ────────────────────────────────────────────────────────────

function checkConfiguration(): void {
    const config   = vscode.workspace.getConfiguration("z80debug");
    const sugarbox = config.get<string>("sugarbox", "");

    if (!sugarbox || !fs.existsSync(sugarbox)) {
        vscode.window.showWarningMessage(
            t("cfg.warnNotConfigured"),
            t("cfg.configureNow")
        ).then(choice => {
            if (choice === t("cfg.configureNow")) {
                vscode.commands.executeCommand("z80debug.configure");
            }
        });
    }
}

// ─── Address helpers ──────────────────────────────────────────────────────────

function addrFromVariableArg(arg: any): number | undefined {
    if (!arg || typeof arg !== "object") return undefined;
    const v = arg.variable ?? arg;
    const ref: string | undefined = v?.memoryReference;
    if (ref) {
        const n = parseInt(ref.replace(/^0x/i, ""), 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF) return n;
    }
    const val: string | undefined = v?.value ?? v?.variable?.value;
    if (val) {
        const digits = String(val).trim().replace(/^(?:0x|\$|#)/i, "");
        const n = parseInt(digits, 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF) return n;
    }
    return undefined;
}

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
    return (isNaN(n) || n < 0 || n > 0xFFFF) ? t("addr.validate") : null;
}

function parseAddrInput(input: string): number {
    const raw = input.trim();
    return raw.match(/^\d+$/)
        ? parseInt(raw, 10)
        : parseInt(raw.replace(/^0x/i, ""), 16);
}
