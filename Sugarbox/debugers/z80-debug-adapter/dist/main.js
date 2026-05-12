/******/ (() => { // webpackBootstrap
/******/ 	var __webpack_modules__ = ([
/* 0 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.activate = activate;
exports.deactivate = deactivate;
const vscode = __importStar(__webpack_require__(1));
const fs = __importStar(__webpack_require__(2));
const nodePath = __importStar(__webpack_require__(3));
const Z80DebugSession_1 = __webpack_require__(4);
const MemoryViewPanel_1 = __webpack_require__(30);
const HardwarePanel_1 = __webpack_require__(31);
const CrtcAsicPanel_1 = __webpack_require__(32);
const GateArrayPanel_1 = __webpack_require__(33);
const PsgPanel_1 = __webpack_require__(34);
const PpiPanel_1 = __webpack_require__(35);
const FdcPanel_1 = __webpack_require__(36);
const TapePanel_1 = __webpack_require__(37);
const HardwarePanelTreeProvider_1 = __webpack_require__(38);
const i18n_1 = __webpack_require__(39);
// ─── Disassembly virtual document provider ────────────────────────────────────
class Z80DisasmProvider {
    constructor() {
        this._onDidChange = new vscode.EventEmitter();
        this.onDidChange = this._onDidChange.event;
    }
    refresh(uri) { this._onDidChange.fire(uri); }
    async provideTextDocumentContent(uri) {
        const parts = uri.path.replace(/^\//, "").split("/");
        let memType;
        let bank;
        let hex;
        if (parts.length >= 3) {
            memType = parts[0];
            bank = parseInt(parts[1], 10);
            if (isNaN(bank))
                bank = -1;
            hex = parts[2].replace(/\.z80disasm$/, "");
        }
        else {
            memType = "read";
            bank = -1;
            hex = parts[0].replace(/\.z80disasm$/, "");
        }
        const addr = parseInt(hex, 16);
        if (isNaN(addr))
            return `; Invalid address: ${uri.path}`;
        const session = vscode.debug.activeDebugSession;
        if (!session)
            return `; No active debug session`;
        try {
            const result = await session.customRequest("getDisasmAt", { address: addr, memType, bank });
            return result?.text ?? `; No disassembly returned`;
        }
        catch (e) {
            return `; Error: ${e}`;
        }
    }
}
// ─── Gutter decorations ───────────────────────────────────────────────────────
let bpDecoration;
let pcDecoration;
const bpAddresses = new Set();
let currentPcAddress;
function refreshZ80BpDecorations() {
    for (const editor of vscode.window.visibleTextEditors) {
        if (editor.document.languageId !== "z80-disasm")
            continue;
        const ranges = [];
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
        if (editor.document.languageId !== "z80-disasm")
            continue;
        const ranges = [];
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
function activate(context) {
    // Must be first — all t() calls depend on it
    (0, i18n_1.initI18n)(context.extensionPath);
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
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(() => { refreshZ80BpDecorations(); refreshPcDecoration(); }), vscode.window.onDidChangeVisibleTextEditors(() => { refreshZ80BpDecorations(); refreshPcDecoration(); }));
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument(doc => {
        if (doc.uri.scheme === "z80disasm" && doc.languageId !== "z80-disasm") {
            vscode.languages.setTextDocumentLanguage(doc, "z80-disasm").then(() => { refreshZ80BpDecorations(); refreshPcDecoration(); }, () => { });
        }
    }));
    // ── DebugAdapterTracker ───────────────────────────────────────────────────
    context.subscriptions.push(vscode.debug.registerDebugAdapterTrackerFactory("z80", {
        createDebugAdapterTracker(_session) {
            return {
                onDidSendMessage(message) {
                    if (message.type === "response" && message.command === "stackTrace") {
                        const frame0 = message.body?.stackFrames?.[0];
                        const memRef = frame0?.memoryReference;
                        if (memRef) {
                            const addr = parseInt(memRef.replace(/^0x/i, ""), 16);
                            if (!isNaN(addr)) {
                                currentPcAddress = addr & 0xFFFF;
                                refreshPcDecoration();
                            }
                        }
                        const framePath = frame0?.source?.path;
                        if (framePath?.startsWith("z80disasm:")) {
                            const frameUri = vscode.Uri.parse(framePath);
                            const frameUriStr = frameUri.toString();
                            const frameVisible = vscode.window.visibleTextEditors.some(e => e.document.uri.toString() === frameUriStr);
                            if (!frameVisible) {
                                const existingCol = vscode.window.visibleTextEditors.find(e => e.document.uri.scheme === "z80disasm")?.viewColumn ?? vscode.ViewColumn.Active;
                                vscode.workspace.openTextDocument(frameUri)
                                    .then(doc => Promise.resolve(vscode.window.showTextDocument(doc, {
                                    viewColumn: existingCol,
                                    preview: false
                                })), () => { });
                            }
                        }
                    }
                    else if (message.type === "event" &&
                        message.event === "stopped") {
                        HardwarePanel_1.HardwarePanel.refreshAll().catch(() => { });
                    }
                    else if (message.type === "event" &&
                        message.event === "mediaChanged") {
                        FdcPanel_1.FdcPanel.currentPanel?.refresh().catch(() => { });
                    }
                    else if (message.type === "event" &&
                        (message.event === "continued" || message.event === "terminated")) {
                        currentPcAddress = undefined;
                        refreshPcDecoration();
                    }
                }
            };
        }
    }));
    // ── Register debug adapter ────────────────────────────────────────────────
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory("z80", {
        createDebugAdapterDescriptor: () => new vscode.DebugAdapterInlineImplementation(new Z80DebugSession_1.Z80DebugSession())
    }));
    // ── Register disassembly content provider ─────────────────────────────────
    const disasmProvider = new Z80DisasmProvider();
    context.subscriptions.push(vscode.workspace.registerTextDocumentContentProvider("z80disasm", disasmProvider));
    // ── Hardware panels TreeView ──────────────────────────────────────────────
    const hwTree = new HardwarePanelTreeProvider_1.HardwarePanelTreeProvider();
    context.subscriptions.push(vscode.window.registerTreeDataProvider("z80debug.hardwarePanels", hwTree));
    // ── Commands: hardware panels ─────────────────────────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.showCrtcPanel", () => CrtcAsicPanel_1.CrtcAsicPanel.createOrShow()), vscode.commands.registerCommand("z80debug.showGateArrayPanel", () => GateArrayPanel_1.GateArrayPanel.createOrShow()), vscode.commands.registerCommand("z80debug.showPsgPanel", () => PsgPanel_1.PsgPanel.createOrShow()), vscode.commands.registerCommand("z80debug.showFdcPanel", () => FdcPanel_1.FdcPanel.createOrShow()), vscode.commands.registerCommand("z80debug.showPpiPanel", () => PpiPanel_1.PpiPanel.createOrShow()), vscode.commands.registerCommand("z80debug.showTapePanel", () => TapePanel_1.TapePanel.createOrShow()));
    // ── Command: open disassembly at address ──────────────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.openDisasmAt", async (arg) => {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.openDisasmAt.noSession"));
            return;
        }
        let addr = addrFromVariableArg(arg) ?? addrFromEditor();
        if (addr === undefined) {
            const input = await vscode.window.showInputBox({
                title: (0, i18n_1.t)("cmd.openDisasmAt.title"),
                prompt: (0, i18n_1.t)("cmd.openDisasmAt.prompt"),
                placeHolder: (0, i18n_1.t)("cmd.openDisasmAt.placeholder"),
                validateInput: validateAddr
            });
            if (input === undefined)
                return;
            addr = parseAddrInput(input);
        }
        let memType = "read";
        let bank = -1;
        try {
            const result = await session.customRequest("getMemBanks");
            const sources = result?.sources ?? null;
            if (sources && sources.length > 1) {
                const items = sources.map(s => ({
                    label: s.label,
                    description: s.type + (s.bank >= 0 ? ` #${s.bank}` : ""),
                    memType: s.type,
                    srcBank: s.bank
                }));
                const picked = await vscode.window.showQuickPick(items, {
                    title: (0, i18n_1.t)("cmd.openDisasmAt.memSource"),
                    placeHolder: (0, i18n_1.t)("cmd.openDisasmAt.memDefault")
                });
                if (picked === undefined)
                    return;
                memType = picked.memType;
                bank = picked.srcBank;
            }
            else if (sources && sources.length === 1) {
                memType = sources[0].type;
                bank = sources[0].bank;
            }
        }
        catch (_) { /* fall back to READ */ }
        const hex4 = (addr & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
        const uri = vscode.Uri.parse(`z80disasm:/${memType}/${bank}/${hex4}.z80disasm`);
        const doc = await vscode.workspace.openTextDocument(uri);
        await vscode.languages.setTextDocumentLanguage(doc, "z80-disasm");
        await vscode.window.showTextDocument(doc, { preview: false });
    }));
    // ── Command: open memory view at address ──────────────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.openMemoryAt", async (arg) => {
        if (!vscode.debug.activeDebugSession) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.openMemoryAt.noSession"));
            return;
        }
        let addr = addrFromVariableArg(arg) ?? addrFromEditor();
        if (addr === undefined) {
            const input = await vscode.window.showInputBox({
                title: (0, i18n_1.t)("cmd.openMemoryAt.title"),
                prompt: (0, i18n_1.t)("cmd.openMemoryAt.prompt"),
                placeHolder: (0, i18n_1.t)("cmd.openMemoryAt.placeholder"),
                validateInput: validateAddr
            });
            if (input === undefined)
                return;
            addr = parseAddrInput(input);
        }
        MemoryViewPanel_1.MemoryViewPanel.createOrShow(addr);
    }));
    // ── Command: add breakpoint by address or label ───────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.addBreakpointAt", async () => {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.addBreakpoint.noSession"));
            return;
        }
        const input = await vscode.window.showInputBox({
            title: (0, i18n_1.t)("cmd.addBreakpoint.title"),
            prompt: (0, i18n_1.t)("cmd.addBreakpoint.prompt"),
            placeHolder: (0, i18n_1.t)("cmd.addBreakpoint.placeholder"),
        });
        if (input === undefined || input.trim() === "")
            return;
        try {
            const result = await session.customRequest("z80bp", { name: input.trim(), enable: true });
            if (result?.address !== undefined) {
                bpAddresses.add(result.address & 0xFFFF);
                refreshZ80BpDecorations();
            }
        }
        catch (e) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.addBreakpoint.failed", String(e)));
        }
    }));
    // ── Command: toggle breakpoint on current line ────────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.toggleBreakpointAt", async () => {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.toggleBreakpoint.noSession"));
            return;
        }
        const editor = vscode.window.activeTextEditor;
        if (!editor)
            return;
        const lineText = editor.document.lineAt(editor.selection.active.line).text;
        const m = lineText.match(/^0x([0-9a-fA-F]{4})/i);
        if (!m) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.toggleBreakpoint.noAddr"));
            return;
        }
        const addr = parseInt(m[1], 16);
        const enable = !bpAddresses.has(addr);
        try {
            await session.customRequest("z80bp", { address: addr, enable });
            if (enable)
                bpAddresses.add(addr);
            else
                bpAddresses.delete(addr);
            refreshZ80BpDecorations();
        }
        catch (e) {
            vscode.window.showWarningMessage((0, i18n_1.t)("cmd.toggleBreakpoint.failed", String(e)));
        }
    }));
    // ── Clear local BP/PC state when session ends ─────────────────────────────
    context.subscriptions.push(vscode.debug.onDidTerminateDebugSession(() => {
        bpAddresses.clear();
        currentPcAddress = undefined;
        refreshZ80BpDecorations();
        refreshPcDecoration();
    }));
    // ── Commands: configure + project + quick launch ──────────────────────────
    context.subscriptions.push(vscode.commands.registerCommand("z80debug.configure", configureWorkspace), vscode.commands.registerCommand("z80debug.newProject", newProject), vscode.commands.registerCommand("z80debug.quickLaunch", quickLaunch));
    checkConfiguration();
}
function deactivate() { }
async function newProject() {
    const parentPick = await vscode.window.showOpenDialog({
        title: (0, i18n_1.t)("np.folderPicker.title"),
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        openLabel: (0, i18n_1.t)("np.folderPicker.label")
    });
    if (!parentPick)
        return;
    const parentDir = parentPick[0].fsPath;
    const projectName = await vscode.window.showInputBox({
        title: (0, i18n_1.t)("np.name.title"),
        placeHolder: (0, i18n_1.t)("np.name.placeholder"),
        prompt: (0, i18n_1.t)("np.name.prompt"),
        validateInput: v => {
            const s = v.trim();
            if (!s)
                return (0, i18n_1.t)("np.name.errEmpty");
            if (!/^[a-zA-Z0-9_\-]+$/.test(s))
                return (0, i18n_1.t)("np.name.errChars");
            if (fs.existsSync(nodePath.join(parentDir, s)))
                return (0, i18n_1.t)("np.name.errExists", s);
            return null;
        }
    });
    if (!projectName)
        return;
    const templateChoice = await vscode.window.showQuickPick([
        {
            label: (0, i18n_1.t)("np.template.hello.label"),
            description: (0, i18n_1.t)("np.template.hello.desc"),
            detail: (0, i18n_1.t)("np.template.hello.detail"),
            id: "hello"
        },
        {
            label: (0, i18n_1.t)("np.template.empty.label"),
            description: (0, i18n_1.t)("np.template.empty.desc"),
            detail: (0, i18n_1.t)("np.template.empty.detail"),
            id: "empty"
        }
    ], {
        title: (0, i18n_1.t)("np.template.title"),
        matchOnDescription: true
    });
    if (!templateChoice)
        return;
    const projectDir = nodePath.join(parentDir, projectName);
    const srcDir = nodePath.join(projectDir, "src");
    const buildDir = nodePath.join(projectDir, "build");
    const vscodeDir = nodePath.join(projectDir, ".vscode");
    try {
        fs.mkdirSync(srcDir, { recursive: true });
        fs.mkdirSync(buildDir, { recursive: true });
        fs.mkdirSync(vscodeDir, { recursive: true });
    }
    catch (e) {
        vscode.window.showErrorMessage((0, i18n_1.t)("np.errCreate", e.message));
        return;
    }
    const globalCfg = vscode.workspace.getConfiguration("z80debug");
    const sugarboxPath = globalCfg.get("sugarbox") || "";
    const rasmPath = globalCfg.get("rasm") || "rasm";
    fs.writeFileSync(nodePath.join(srcDir, "main.asm"), templateChoice.id === "hello" ? templateHello(projectName) : templateEmpty(projectName));
    fs.writeFileSync(nodePath.join(vscodeDir, "tasks.json"), tasksJson());
    fs.writeFileSync(nodePath.join(vscodeDir, "launch.json"), launchJson());
    fs.writeFileSync(nodePath.join(vscodeDir, "settings.json"), settingsJson(projectName, sugarboxPath, rasmPath));
    fs.writeFileSync(nodePath.join(projectDir, ".gitignore"), "build/\n");
    const newUri = vscode.Uri.file(projectDir);
    const choice = await vscode.window.showInformationMessage((0, i18n_1.t)("np.created", projectName, projectDir), (0, i18n_1.t)("np.open"), (0, i18n_1.t)("np.openNew"));
    if (choice === (0, i18n_1.t)("np.open")) {
        await vscode.commands.executeCommand("vscode.openFolder", newUri, false);
    }
    else if (choice === (0, i18n_1.t)("np.openNew")) {
        await vscode.commands.executeCommand("vscode.openFolder", newUri, true);
    }
}
// ── ASM templates ─────────────────────────────────────────────────────────────
function templateHello(name) {
    return `\
; ── ${name} ${"─".repeat(Math.max(0, 78 - name.length))}
; ${(0, i18n_1.t)("tmpl.built-with")}
; ${(0, i18n_1.t)("tmpl.call-from-basic")}
; ${"─".repeat(78)}

        BANKSET 0
        ORG     #8000
        RUN     start

; ── ${(0, i18n_1.t)("tmpl.entry-point")} ${"─".repeat(Math.max(0, 70 - (0, i18n_1.t)("tmpl.entry-point").length))}
start:
        ld      hl, msg_hello
        call    print_string

        ; ${(0, i18n_1.t)("tmpl.infinite-loop")}
loop:
        jr      loop

; ── ${(0, i18n_1.t)("tmpl.subroutine")} ${"─".repeat(Math.max(0, 70 - (0, i18n_1.t)("tmpl.subroutine").length))}
; ${(0, i18n_1.t)("tmpl.subroutine.input")}
print_string:
        ld      a, (hl)
        or      a
        ret     z
        call    TXT_OUTPUT
        inc     hl
        jr      print_string

; ── ${(0, i18n_1.t)("tmpl.firmware")} ${"─".repeat(Math.max(0, 70 - (0, i18n_1.t)("tmpl.firmware").length))}
TXT_OUTPUT      EQU     #BB5A

; ── ${(0, i18n_1.t)("tmpl.data")} ${"─".repeat(Math.max(0, 70 - (0, i18n_1.t)("tmpl.data").length))}
msg_hello:
        db      "Hello, World!", 13, 0
`;
}
function templateEmpty(name) {
    return `\
; ── ${name} ${"─".repeat(Math.max(0, 78 - name.length))}
; ${(0, i18n_1.t)("tmpl.built-with")}
; ${(0, i18n_1.t)("tmpl.call-from-basic")}
; ${"─".repeat(78)}

        BANKSET 0
        ORG     #8000
        RUN     start

; ── ${(0, i18n_1.t)("tmpl.entry-point")} ${"─".repeat(Math.max(0, 70 - (0, i18n_1.t)("tmpl.entry-point").length))}
start:
        ; ${(0, i18n_1.t)("tmpl.your-code")}


        ; ${(0, i18n_1.t)("tmpl.bare-loop")}
loop:
        jr      loop
`;
}
function tasksJson() {
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
                    "-o", "${workspaceFolder}/build/${config:z80debug.buildName}",
                    "-oi", "${workspaceFolder}/build/${config:z80debug.buildName}.sna",
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
function launchJson() {
    return JSON.stringify({
        version: "0.2.0",
        configurations: [
            {
                type: "z80",
                request: "launch",
                name: (0, i18n_1.t)("launch.debugName"),
                emulator: "${config:z80debug.sugarbox}",
                snapshot: "${workspaceFolder}/build/${config:z80debug.buildName}.sna",
                symbolFile: "${workspaceFolder}/build/${config:z80debug.buildName}.rasm",
                sourceFile: "${workspaceFolder}/${config:z80debug.entryPoint}",
                port: 1234,
                hideEmulator: false,
                preLaunchTask: "RASM: assemble"
            },
            {
                type: "z80",
                request: "attach",
                name: (0, i18n_1.t)("launch.attachName"),
                port: 1234,
                symbolFile: "${workspaceFolder}/build/${config:z80debug.buildName}.rasm"
            }
        ]
    }, null, 2);
}
function settingsJson(buildName, sugarbox, rasm) {
    const s = {
        "z80debug.entryPoint": "src/main.asm",
        "z80debug.buildName": buildName,
        "files.associations": { "*.asm": "asm-collection" },
        "[asm-collection]": { "editor.colorDecorators": false },
        "[z80-disasm]": { "editor.colorDecorators": false }
    };
    if (sugarbox) {
        s["z80debug.sugarbox"] = sugarbox;
    }
    if (rasm && rasm !== "rasm") {
        s["z80debug.rasm"] = rasm;
    }
    return JSON.stringify(s, null, 2);
}
async function quickLaunch() {
    const cfg = vscode.workspace.getConfiguration("z80debug");
    let emulatorPath = cfg.get("sugarbox") || "";
    if (!emulatorPath || !fs.existsSync(emulatorPath)) {
        const picked = await vscode.window.showOpenDialog({
            title: (0, i18n_1.t)("ql.emulatorPicker.title"),
            canSelectMany: false,
            filters: process.platform === "win32"
                ? { [(0, i18n_1.t)("ql.emulatorPicker.exe")]: ["exe"] }
                : { [(0, i18n_1.t)("ql.emulatorPicker.all")]: ["*"] }
        });
        if (!picked)
            return;
        emulatorPath = picked[0].fsPath;
    }
    const folder = vscode.workspace.workspaceFolders?.[0];
    const buildName = cfg.get("buildName") || "main";
    let projectSna;
    let projectSymbols;
    let projectDsk;
    if (folder) {
        const buildDir = nodePath.join(folder.uri.fsPath, "build");
        const snaPath = nodePath.join(buildDir, `${buildName}.sna`);
        const rasmPath = nodePath.join(buildDir, `${buildName}.rasm`);
        const dskPath = nodePath.join(buildDir, `${buildName}.dsk`);
        if (fs.existsSync(snaPath))
            projectSna = snaPath;
        if (fs.existsSync(rasmPath))
            projectSymbols = rasmPath;
        if (fs.existsSync(dskPath))
            projectDsk = dskPath;
    }
    const symSuffix = projectSymbols ? (0, i18n_1.t)("ql.media.symbols.suffix") : "";
    const mediaItems = [
        ...(projectSna ? [{
                label: (0, i18n_1.t)("ql.media.projectSnapshot"),
                description: (0, i18n_1.t)("ql.media.projectSnapshot.desc", buildName, symSuffix),
                detail: projectSna,
                media: "projectSnapshot"
            }] : []),
        ...(projectSna ? [{
                label: (0, i18n_1.t)("ql.media.projectSnapshotBuild"),
                description: (0, i18n_1.t)("ql.media.projectSnapshotBuild.desc", buildName),
                detail: (0, i18n_1.t)("ql.media.projectSnapshotBuild.detail"),
                media: "projectSnapshotBuild"
            }] : []),
        ...(projectDsk ? [{
                label: (0, i18n_1.t)("ql.media.projectDisk"),
                description: (0, i18n_1.t)("ql.media.projectDisk.desc", buildName, symSuffix),
                detail: projectDsk,
                media: "projectDisk"
            }] : []),
        { label: (0, i18n_1.t)("ql.media.empty"), description: (0, i18n_1.t)("ql.media.empty.desc"), media: "empty" },
        { label: (0, i18n_1.t)("ql.media.diskA"), description: (0, i18n_1.t)("ql.media.diskA.desc"), media: "disk" },
        { label: (0, i18n_1.t)("ql.media.diskB"), description: (0, i18n_1.t)("ql.media.diskB.desc"), media: "diskB" },
        { label: (0, i18n_1.t)("ql.media.tape"), description: (0, i18n_1.t)("ql.media.tape.desc"), media: "tape" },
        { label: (0, i18n_1.t)("ql.media.snapshot"), description: (0, i18n_1.t)("ql.media.snapshot.desc"), media: "snapshot" },
        { label: (0, i18n_1.t)("ql.media.cartridge"), description: (0, i18n_1.t)("ql.media.cartridge.desc"), media: "cartridge" },
    ];
    const mediaChoice = await vscode.window.showQuickPick(mediaItems, {
        title: (0, i18n_1.t)("ql.media.title"),
        matchOnDescription: true,
        matchOnDetail: true
    });
    if (!mediaChoice)
        return;
    const launchCfg = {
        type: "z80",
        request: "launch",
        name: (0, i18n_1.t)("ql.launchName"),
        emulator: emulatorPath,
        port: 1234,
        hideEmulator: false,
    };
    switch (mediaChoice.media) {
        case "projectSnapshot":
            launchCfg.snapshot = projectSna;
            if (projectSymbols)
                launchCfg.symbolFile = projectSymbols;
            break;
        case "projectSnapshotBuild":
            launchCfg.snapshot = projectSna;
            launchCfg.preLaunchTask = "RASM: assemble";
            if (projectSymbols)
                launchCfg.symbolFile = projectSymbols;
            break;
        case "projectDisk":
            launchCfg.disk = projectDsk;
            if (projectSymbols)
                launchCfg.symbolFile = projectSymbols;
            break;
        case "empty":
            break;
        default: {
            const filterMap = {
                disk: { [(0, i18n_1.t)("ql.filePicker.disk")]: ["dsk"] },
                diskB: { [(0, i18n_1.t)("ql.filePicker.disk")]: ["dsk"] },
                tape: { [(0, i18n_1.t)("ql.filePicker.tape")]: ["cdt", "wav", "tzx"] },
                snapshot: { [(0, i18n_1.t)("ql.filePicker.snapshot")]: ["sna"] },
                cartridge: { [(0, i18n_1.t)("ql.filePicker.cartridge")]: ["cpr"] },
            };
            const files = await vscode.window.showOpenDialog({
                title: (0, i18n_1.t)("ql.filePicker.title"),
                canSelectMany: false,
                filters: filterMap[mediaChoice.media] ?? { [(0, i18n_1.t)("ql.filePicker.all")]: ["*"] }
            });
            if (!files)
                return;
            launchCfg[mediaChoice.media] = files[0].fsPath;
            break;
        }
    }
    const configItems = [
        { label: (0, i18n_1.t)("ql.config.cpc6128"), description: (0, i18n_1.t)("ql.config.cpc6128.desc"), cfg: undefined },
        { label: (0, i18n_1.t)("ql.config.cpc464"), description: "", cfg: "CPC464" },
        { label: (0, i18n_1.t)("ql.config.cpc664"), description: "", cfg: "CPC664" },
        { label: (0, i18n_1.t)("ql.config.cpcplus"), description: "", cfg: "CPC+" },
        { label: (0, i18n_1.t)("ql.config.custom"), description: (0, i18n_1.t)("ql.config.custom.desc"), cfg: "__custom__" },
    ];
    const configChoice = await vscode.window.showQuickPick(configItems, {
        title: (0, i18n_1.t)("ql.config.title")
    });
    if (configChoice) {
        if (configChoice.cfg === "__custom__") {
            const custom = await vscode.window.showInputBox({
                title: (0, i18n_1.t)("ql.config.customInput.title"),
                prompt: (0, i18n_1.t)("ql.config.customInput.prompt"),
                placeHolder: (0, i18n_1.t)("ql.config.customInput.placeholder")
            });
            if (custom?.trim())
                launchCfg.configuration = custom.trim();
        }
        else if (configChoice.cfg) {
            launchCfg.configuration = configChoice.cfg;
        }
    }
    await vscode.debug.startDebugging(folder, launchCfg);
}
// ─── Configure workspace ──────────────────────────────────────────────────────
async function configureWorkspace() {
    const config = vscode.workspace.getConfiguration("z80debug");
    const sugarboxResult = await vscode.window.showOpenDialog({
        title: (0, i18n_1.t)("cfg.sugarboxPicker.title"),
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { [(0, i18n_1.t)("cfg.exe")]: ["exe"] }
            : { [(0, i18n_1.t)("cfg.all")]: ["*"] }
    });
    if (!sugarboxResult || sugarboxResult.length === 0) {
        vscode.window.showWarningMessage((0, i18n_1.t)("cfg.warnNoSugarbox"));
        return;
    }
    const sugarboxPath = sugarboxResult[0].fsPath;
    const rasmResult = await vscode.window.showOpenDialog({
        title: (0, i18n_1.t)("cfg.rasmPicker.title"),
        canSelectMany: false,
        filters: process.platform === "win32"
            ? { [(0, i18n_1.t)("cfg.exe")]: ["exe"] }
            : { [(0, i18n_1.t)("cfg.all")]: ["*"] }
    });
    if (!rasmResult || rasmResult.length === 0) {
        vscode.window.showWarningMessage((0, i18n_1.t)("cfg.warnNoRasm"));
        return;
    }
    const rasmPath = rasmResult[0].fsPath;
    await config.update("sugarbox", sugarboxPath, vscode.ConfigurationTarget.Workspace);
    await config.update("rasm", rasmPath, vscode.ConfigurationTarget.Workspace);
    vscode.window.showInformationMessage((0, i18n_1.t)("cfg.done", sugarboxPath, rasmPath));
}
// ─── Startup check ────────────────────────────────────────────────────────────
function checkConfiguration() {
    const config = vscode.workspace.getConfiguration("z80debug");
    const sugarbox = config.get("sugarbox", "");
    if (!sugarbox || !fs.existsSync(sugarbox)) {
        vscode.window.showWarningMessage((0, i18n_1.t)("cfg.warnNotConfigured"), (0, i18n_1.t)("cfg.configureNow")).then(choice => {
            if (choice === (0, i18n_1.t)("cfg.configureNow")) {
                vscode.commands.executeCommand("z80debug.configure");
            }
        });
    }
}
// ─── Address helpers ──────────────────────────────────────────────────────────
function addrFromVariableArg(arg) {
    if (!arg || typeof arg !== "object")
        return undefined;
    const v = arg.variable ?? arg;
    const ref = v?.memoryReference;
    if (ref) {
        const n = parseInt(ref.replace(/^0x/i, ""), 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF)
            return n;
    }
    const val = v?.value ?? v?.variable?.value;
    if (val) {
        const digits = String(val).trim().replace(/^(?:0x|\$|#)/i, "");
        const n = parseInt(digits, 16);
        if (!isNaN(n) && n >= 0 && n <= 0xFFFF)
            return n;
    }
    return undefined;
}
function addrFromEditor() {
    const editor = vscode.window.activeTextEditor;
    if (!editor)
        return undefined;
    let text = editor.document.getText(editor.selection).trim();
    if (!text) {
        const wordRange = editor.document.getWordRangeAtPosition(editor.selection.active, /(?:0x|\$|#)?[0-9a-fA-F]{1,4}/);
        text = wordRange ? editor.document.getText(wordRange).trim() : "";
    }
    if (!text)
        return undefined;
    const digits = text.replace(/^(?:0x|\$|#)/i, "");
    const n = parseInt(digits, 16);
    return (!isNaN(n) && n >= 0 && n <= 0xFFFF) ? n : undefined;
}
function validateAddr(v) {
    const raw = v.trim();
    const n = raw.match(/^\d+$/)
        ? parseInt(raw, 10)
        : parseInt(raw.replace(/^0x/i, ""), 16);
    return (isNaN(n) || n < 0 || n > 0xFFFF) ? (0, i18n_1.t)("addr.validate") : null;
}
function parseAddrInput(input) {
    const raw = input.trim();
    return raw.match(/^\d+$/)
        ? parseInt(raw, 10)
        : parseInt(raw.replace(/^0x/i, ""), 16);
}
//# sourceMappingURL=main.js.map

/***/ }),
/* 1 */
/***/ ((module) => {

"use strict";
module.exports = require("vscode");

/***/ }),
/* 2 */
/***/ ((module) => {

"use strict";
module.exports = require("fs");

/***/ }),
/* 3 */
/***/ ((module) => {

"use strict";
module.exports = require("path");

/***/ }),
/* 4 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.Z80DebugSession = void 0;
const vscode_debugadapter_1 = __webpack_require__(5);
const vscode_debugadapter_2 = __webpack_require__(5);
const EmulatorClient_1 = __webpack_require__(25);
const SymbolTable_1 = __webpack_require__(26);
const SourceAnnotations_1 = __webpack_require__(27);
const vscode_debugadapter_3 = __webpack_require__(5);
const vscode_debugadapter_4 = __webpack_require__(5);
const vscode_debugadapter_5 = __webpack_require__(5);
const cp = __importStar(__webpack_require__(28));
const fs = __importStar(__webpack_require__(2));
const os = __importStar(__webpack_require__(29));
const nodePath = __importStar(__webpack_require__(3));
const net = __importStar(__webpack_require__(11));
// Column width for the mnemonic field in the virtual source view
const COL_INSTR = 20; // mnemonic + operands, padded to this width
// Maximum Z80 instruction size in bytes (DD CB dd op = 4 bytes)
const MAX_INSTR_BYTES = 4;
// Width of the hex field: "XX XX XX XX" = 4×2 + 3 spaces = 11 chars
const HEX_FIELD_WIDTH = MAX_INSTR_BYTES * 3 - 1; // 11
/**
 * Format raw bytes for the virtual source view.
 *
 * Returns a "; XX XX  .." style inline comment so the TextMate grammar
 * can colour it as a comment (pale/dim).
 */
function fmtHexAsciiComment(bytes) {
    if (!bytes || bytes.length === 0)
        return "";
    const hex = bytes.map(b => b.toString(16).toUpperCase().padStart(2, "0")).join(" ");
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
function fmtInstructionBytes(bytes) {
    if (!bytes || bytes.length === 0)
        return undefined;
    const hex = bytes.map(b => b.toString(16).toUpperCase().padStart(2, "0")).join(" ");
    const ascii = bytes.map(b => (b >= 0x20 && b <= 0x7E) ? String.fromCharCode(b) : ".").join("");
    return `${hex.padEnd(HEX_FIELD_WIDTH)}  ${ascii}`;
}
// 16-bit register names (for memoryReference and 4-digit hex formatting)
const REG16 = new Set(["bc", "de", "hl", "sp", "pc", "ix", "iy", "bc'", "de'", "hl'", "af", "af'"]);
// 8-bit register names (2-digit hex)
const REG8 = new Set(["i", "r"]);
class Z80DebugSession extends vscode_debugadapter_2.DebugSession {
    constructor() {
        super();
        this.emulator = new EmulatorClient_1.EmulatorClient();
        this.isAttach = false;
        this.emulatorProcess = null;
        // Disassembly cache: sourceRef → region
        this.disasmCache = new Map();
        // Reverse index: "memType:bank:startAddr" → sourceRef (avoids duplicate builds)
        this.disasmKeyToRef = new Map();
        this.disasmRefCounter = 1;
        // Symbol table (optional, loaded from symbolFile arg)
        this.symbolTable = null;
        // Source annotations (optional, loaded from sourceFile arg)
        this.sourceAnnotations = null;
        // Global breakpoint registry: key → list of addresses
        // "src:<sourceRef>" for source breakpoints, "instr" for instruction breakpoints
        this.bpRegistry = new Map();
        console.log("Z80 Debug Adapter started");
        this.setDebuggerLinesStartAt1(true);
        this.setDebuggerColumnsStartAt1(true);
        this.on("stopped", (reason) => {
            this.sendEvent(new vscode_debugadapter_3.StoppedEvent(reason, 1));
        });
    }
    onStopped(reason) {
        this.sendEvent(new vscode_debugadapter_3.StoppedEvent(reason, 1));
    }
    initializeRequest(response, args) {
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
        response.body.supportsInlineBreakpoints = true;
        this.sendResponse(response);
        // InitializedEvent is sent at the end of launchRequest / attachRequest,
        // after the TCP connection to the emulator is established. Sending it here
        // would trigger VS Code to send configurationDone before the socket exists.
    }
    loadSymbols(args) {
        console.log(`DAP: loadSymbols — symbolFile=${args.symbolFile ?? "(none)"} sourceFile=${args.sourceFile ?? "(none)"} snapshot=${args.snapshot ?? "(none)"}`);
        if (args.symbolFile) {
            this.symbolTable = SymbolTable_1.SymbolTable.fromRasm(args.symbolFile);
        }
        if (args.sourceFile) {
            this.sourceAnnotations = SourceAnnotations_1.SourceAnnotations.fromFile(args.sourceFile);
        }
        if (args.snapshot) {
            const { table, breakpoints } = SymbolTable_1.SymbolTable.fromSnapshotRemu(args.snapshot);
            if (this.symbolTable) {
                this.symbolTable.merge(table);
            }
            else if (table.size > 0) {
                this.symbolTable = table;
            }
            if (breakpoints.length > 0) {
                this.bpRegistry.set("snapshot", breakpoints);
                console.log(`DAP: ${breakpoints.length} breakpoint(s) loaded from snapshot REMU`);
            }
        }
        console.log(`DAP: loadSymbols done — symbolTable=${this.symbolTable?.size ?? "null"} symbols, sourceAnnotations=${this.sourceAnnotations ? "loaded" : "null"}`);
    }
    async launchRequest(response, args) {
        console.log("DAP: Launch...");
        this.loadSymbols(args);
        const port = args.port ?? 1234;
        // ── Pre-flight: validate emulator path ────────────────────────────────────
        if (!args.emulator) {
            const msg = "Emulator path not set — configure 'emulator' in launch.json or run Z80 Debug: Configure workspace.\n";
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = "Emulator path not configured";
            this.sendResponse(response);
            return;
        }
        if (!fs.existsSync(args.emulator)) {
            const msg = `Emulator binary not found: "${args.emulator}"\nCheck the 'emulator' field in launch.json or the z80debug.sugarbox setting.\n`;
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = `Emulator not found: ${args.emulator}`;
            this.sendResponse(response);
            return;
        }
        // Build a temporary CSL script for disk/tape (snapshot is loaded via DAP command after connect)
        let cslFile = null;
        if (args.disk || args.diskB || args.tape) {
            const lines = ["cslversion 2.0"];
            if (args.disk)
                lines.push(`disk_insert 0 '${args.disk}'`);
            if (args.diskB)
                lines.push(`disk_insert 1 '${args.diskB}'`);
            if (args.tape)
                lines.push(`tape_insert '${args.tape}'`);
            cslFile = nodePath.join(os.tmpdir(), `sugarbox_${Date.now()}.csl`);
            fs.writeFileSync(cslFile, lines.join("\n") + "\n");
            console.log("DAP: CSL script written to", cslFile);
        }
        // Build Sugarbox arguments
        const spawnArgs = ["--debug", "--debug_server", String(port)];
        if (cslFile)
            spawnArgs.push("--csl", cslFile);
        if (args.cartridge)
            spawnArgs.push("--cart", args.cartridge);
        if (args.configuration)
            spawnArgs.push("--cfg", args.configuration);
        if (args.hideEmulator)
            spawnArgs.push("--hide");
        // Check if the port is already in use before spawning
        const portInUse = await this.isPortInUse(port);
        if (portInUse) {
            const msg = `Port ${port} is already in use — a previous Sugarbox instance may still be running.\n` +
                `Run: fuser ${port}/tcp  or  ss -tlnp | grep ${port}\n`;
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = `Port ${port} already in use`;
            this.sendResponse(response);
            return;
        }
        // Set cwd to the emulator's own directory so that Sugarbox finds its
        // data files (Sugarbox.ini, ROM/, CONF/) relative to itself, regardless
        // of the VS Code workspace folder.
        const emulatorDir = nodePath.dirname(args.emulator);
        console.log("DAP: Spawning emulator:", args.emulator, spawnArgs.join(" "), "(cwd:", emulatorDir, ")");
        // Track early exit so we can give a more actionable error message.
        let emulatorExitCode = null;
        let spawnError = null;
        this.emulatorProcess = cp.spawn(args.emulator, spawnArgs, {
            stdio: ["ignore", "ignore", "pipe"],
            detached: true, // GUI process — survit si le parent Node.js est tué
            cwd: emulatorDir
        });
        // Relay emulator stderr to the Debug Console for diagnostics
        this.emulatorProcess.stderr?.on("data", (data) => {
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(`[Sugarbox] ${data.toString()}`, "stderr"));
        });
        this.emulatorProcess.on("error", err => {
            spawnError = err.message;
            const msg = `Failed to start emulator "${args.emulator}": ${err.message}\n`;
            console.error(msg);
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            this.sendEvent(new vscode_debugadapter_1.TerminatedEvent());
        });
        this.emulatorProcess.on("exit", code => {
            emulatorExitCode = code ?? -1;
            console.log("DAP: Emulator exited with code", code);
            this.sendEvent(new vscode_debugadapter_1.TerminatedEvent());
        });
        // Wait for the TCP debug port to open (up to 10 s)
        try {
            await this.waitForPort(port, 10000);
        }
        catch (e) {
            // Build a diagnostic message based on what we observed
            let reason;
            if (spawnError) {
                reason = `Emulator failed to start: ${spawnError}`;
            }
            else if (emulatorExitCode !== null) {
                reason = `Emulator exited immediately (code ${emulatorExitCode}) — check the binary and its arguments`;
            }
            else {
                reason = `Emulator did not open port ${port} within 10 s — check that Sugarbox supports --debug_server`;
            }
            const msg = `Launch failed: ${reason}\nCommand: ${args.emulator} ${spawnArgs.join(" ")}\n`;
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = reason;
            this.sendResponse(response);
            return;
        }
        try {
            await this.emulator.connect(port);
        }
        catch (e) {
            const msg = `Emulator port ${port} closed unexpectedly after opening — emulator may have crashed.\n`;
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = `Connection to port ${port} failed: ${e}`;
            this.sendResponse(response);
            return;
        }
        console.log("DAP: Connected to emulator");
        // Load snapshot via DAP command — send file content as base64 to avoid
        // path-resolution issues (relative paths, remote machines, etc.)
        if (args.snapshot) {
            console.log("DAP: Loading snapshot", args.snapshot);
            let snapshotData;
            try {
                snapshotData = fs.readFileSync(args.snapshot).toString("base64");
            }
            catch (e) {
                const msg = `Cannot read snapshot file "${args.snapshot}": ${e.message}\n`;
                this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
                response.success = false;
                response.message = msg.trim();
                this.sendResponse(response);
                return;
            }
            const r = await this.emulator.send({ cmd: "loadSnapshot", data: snapshotData });
            if (r?.status !== "ok") {
                const msg = `Failed to load snapshot: ${r?.message ?? args.snapshot}\n`;
                this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
                response.success = false;
                response.message = msg.trim();
                this.sendResponse(response);
                return;
            }
        }
        this.emulator.onEvent = (evt) => {
            if (evt.event === "stopped") {
                const reason = evt.body?.reason ?? "breakpoint";
                console.log("DAP: async stopped event:", reason);
                this.sendEvent(new vscode_debugadapter_3.StoppedEvent(reason, 1));
            }
            else if (evt.event === "mediaChanged") {
                this.sendEvent({ type: "event", event: "mediaChanged", seq: 0, body: evt.body });
            }
        };
        this.sendResponse(response);
        this.sendEvent(new vscode_debugadapter_1.InitializedEvent());
    }
    // Check once if a port already accepts connections (residual process).
    isPortInUse(port, host = "127.0.0.1") {
        return new Promise(resolve => {
            const sock = new net.Socket();
            sock.setTimeout(300);
            sock.connect(port, host, () => { sock.destroy(); resolve(true); });
            sock.on("error", () => { sock.destroy(); resolve(false); });
            sock.on("timeout", () => { sock.destroy(); resolve(false); });
        });
    }
    // Poll until the TCP port accepts connections, or timeout.
    waitForPort(port, timeoutMs, host = "127.0.0.1") {
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
                    if (Date.now() < deadline)
                        setTimeout(tryConnect, 250);
                    else
                        reject(new Error(`Port ${port} not available after ${timeoutMs}ms`));
                });
                sock.on("timeout", () => {
                    sock.destroy();
                    if (Date.now() < deadline)
                        setTimeout(tryConnect, 250);
                    else
                        reject(new Error(`Port ${port} timed out after ${timeoutMs}ms`));
                });
            };
            tryConnect();
        });
    }
    async attachRequest(response, args) {
        console.log("DAP: Attach...");
        this.isAttach = true;
        this.loadSymbols(args);
        const port = args.port ?? 1234;
        try {
            await this.emulator.connect(port);
        }
        catch (e) {
            const msg = `Cannot attach: no emulator listening on port ${port}.\n` +
                `Start Sugarbox with: --debug_server ${port}\n`;
            this.sendEvent(new vscode_debugadapter_1.OutputEvent(msg, "stderr"));
            response.success = false;
            response.message = `No emulator on port ${port}`;
            this.sendResponse(response);
            return;
        }
        console.log("DAP: Attached");
        this.emulator.onEvent = (evt) => {
            if (evt.event === "stopped") {
                const reason = evt.body?.reason ?? "breakpoint";
                console.log("DAP: async stopped event:", reason);
                this.sendEvent(new vscode_debugadapter_3.StoppedEvent(reason, 1));
            }
            else if (evt.event === "mediaChanged") {
                this.sendEvent({ type: "event", event: "mediaChanged", seq: 0, body: evt.body });
            }
        };
        this.sendEvent(new vscode_debugadapter_1.InitializedEvent());
        this.sendResponse(response);
    }
    async configurationDoneRequest(response, args) {
        console.log("DAP: configurationDone");
        this.sendResponse(response);
        // Apply all breakpoints (snapshot BPs + any VS Code BPs set during init)
        if (this.bpRegistry.size > 0) {
            await this.flushBreakpoints();
        }
        if (this.isAttach) {
            const state = await this.emulator.send({ cmd: "getState" });
            if (!state?.running) {
                this.sendEvent(new vscode_debugadapter_3.StoppedEvent("pause", 1));
            }
        }
        else {
            this.sendEvent(new vscode_debugadapter_3.StoppedEvent("entry", 1));
        }
    }
    onEmulatorConnected() {
        this.sendEvent(new vscode_debugadapter_1.ContinuedEvent(1, true));
    }
    async continueRequest(response) {
        console.log("DAP: Continue");
        await this.emulator.send({ cmd: "continue" });
        this.sendResponse(response);
    }
    async nextRequest(response) {
        console.log("DAP: Step");
        await this.emulator.send({ cmd: "step" });
        this.sendResponse(response);
        // StoppedEvent will be sent by the async onEvent handler
    }
    async pauseRequest(response, args) {
        console.log("DAP: Halt");
        await this.emulator.send({ cmd: "halt" });
        this.sendEvent(new vscode_debugadapter_3.StoppedEvent("pause", 1));
        this.sendResponse(response);
    }
    scopesRequest(response, args) {
        console.log("DAP: scopesRequest");
        response.body = {
            scopes: [
                // Variables as register, memory. Maybe memory banks ? tape/disks ? cartridge ?
                new vscode_debugadapter_5.Scope("Registers", 1, false),
                new vscode_debugadapter_5.Scope("Memory", 2, false),
                new vscode_debugadapter_5.Scope("Stack", 3, false)
            ]
        };
        this.sendResponse(response);
    }
    async variablesRequest(response, args) {
        // REGISTERS
        if (args.variablesReference == 1) {
            const regs = await this.emulator.send({
                cmd: "readRegisters"
            });
            response.body = {
                variables: Object.entries(regs).map(([name, val]) => {
                    const nameLower = name.toLowerCase();
                    const is8 = REG8.has(nameLower);
                    const is16 = REG16.has(nameLower);
                    const hex = is8
                        ? "0x" + (val & 0xFF).toString(16).padStart(2, "0").toUpperCase()
                        : "0x" + (val & 0xFFFF).toString(16).padStart(4, "0").toUpperCase();
                    const v = {
                        name,
                        value: hex,
                        variablesReference: 0
                    };
                    if (is16) {
                        v.memoryReference = hex;
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
            const sp = state.sp;
            const WORDS = 16;
            const BYTES = WORDS * 2;
            // 2) Lire la mémoire
            const mem = await this.emulator.send({
                cmd: "readMemory",
                address: sp,
                size: BYTES
            }); // tableau de bytes
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
    threadsRequest(response) {
        // Pour l'instant, un seul "CPU Z80" fictif
        console.log("DAP: threadsRequest");
        response.body = {
            threads: [new vscode_debugadapter_4.Thread(1, "Z80 CPU")]
        };
        this.sendResponse(response);
    }
    // ─── Disassembly cache helpers ────────────────────────────────────────────────
    // Fetch (or reuse) a disassembly region that contains addr at a valid boundary.
    //
    // Fast path: find a cached region that already covers addr (same memType/bank).
    // Slow path: disassemble 2048 instructions starting from addr.
    // memType defaults to "read" (used for stack-trace frames).
    async ensureRegion(addr, memType = "read", bank = -1) {
        // Reuse any cached region that already contains addr with the same source
        for (const region of this.disasmCache.values()) {
            if (region.memType === memType && region.bank === bank && region.addressToLine.has(addr)) {
                return region;
            }
        }
        // Allocate a new unique sourceRef
        const sourceRef = this.disasmRefCounter++;
        const startAddress = addr;
        const reply = await this.emulator.send({
            cmd: "disassemble",
            address: startAddress,
            count: 2048,
            memType,
            bank
        });
        const rawLines = reply.instructions ?? [];
        const addressToLine = new Map();
        const lineToAddress = new Map();
        let text = "";
        let textLineNo = 0;
        rawLines.forEach((l) => {
            const labels = this.symbolTable?.getLabelsAt(l.address) ?? [];
            if (labels.length > 0) {
                if (text.length > 0) {
                    text += "\n";
                    textLineNo++;
                }
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
        const region = { sourceRef, startAddress, memType, bank, lines: rawLines, addressToLine, lineToAddress, text };
        this.disasmCache.set(sourceRef, region);
        this.disasmKeyToRef.set(`${memType}:${bank}:${startAddress}`, sourceRef);
        return region;
    }
    // Remove all cached regions that contain addr (e.g. after a memory write).
    invalidateRegion(addr) {
        for (const [key, region] of this.disasmCache.entries()) {
            if (region.addressToLine.has(addr)) {
                this.disasmCache.delete(key);
                this.disasmKeyToRef.delete(`${region.memType}:${region.bank}:${region.startAddress}`);
            }
        }
    }
    // Return true if addr looks like a CALL/RST return address
    // (i.e. addr-3 or addr-1 contains the corresponding opcode).
    async isReturnAddress(addr) {
        if (addr < 3)
            return false;
        const mem = await this.emulator.send({ cmd: "readMemory", address: addr - 3, size: 3 });
        const bytes = mem?.bytes ?? [];
        if (bytes.length < 3)
            return false;
        return Z80DebugSession.CALL_OPCODES.has(bytes[0]) // CALL at addr-3
            || Z80DebugSession.RST_OPCODES.has(bytes[2]); // RST  at addr-1
    }
    // Build a single DAP StackFrame for a given PC and its disassembly region.
    buildStackFrame(id, pc, region) {
        const pcHex = "0x" + pc.toString(16).padStart(4, "0");
        const lineNo = region.addressToLine.get(pc) ?? 1;
        const labels = this.symbolTable?.getLabelsAt(pc);
        const name = labels?.length ? labels[0] : (id === 0 ? "PC" : `ret #${pcHex}`);
        const hex4 = region.startAddress.toString(16).padStart(4, "0").toUpperCase();
        const sourceName = `Z80 0x${region.startAddress.toString(16).padStart(4, "0")}`;
        // Use the z80disasm:/ URI as path so VS Code navigates to the already-open
        // virtual document instead of opening a new one via sourceRequest.
        const sourcePath = `z80disasm:/${region.memType}/${region.bank}/${hex4}.z80disasm`;
        const frame = {
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
        frame.memoryReference = pcHex;
        return frame;
    }
    async stackTraceRequest(response, args) {
        console.log("DAP: stackTraceRequest");
        const state = await this.emulator.send({ cmd: "getState" });
        const pc = state?.pc ?? 0;
        const sp = state?.sp ?? 0;
        // Frame 0 — current PC
        const region0 = await this.ensureRegion(pc);
        const frames = [this.buildStackFrame(0, pc, region0)];
        // Walk the Z80 stack: each word is a potential return address pushed by CALL/RST.
        const MAX_DEPTH = 15;
        const memReply = await this.emulator.send({ cmd: "readMemory", address: sp, size: MAX_DEPTH * 2 });
        const bytes = memReply?.bytes ?? [];
        for (let i = 0; i < MAX_DEPTH && i * 2 + 1 < bytes.length; i++) {
            const retAddr = (bytes[i * 2] | (bytes[i * 2 + 1] << 8)) & 0xFFFF;
            if (!await this.isReturnAddress(retAddr))
                continue;
            const region = await this.ensureRegion(retAddr);
            frames.push(this.buildStackFrame(frames.length, retAddr, region));
        }
        response.body = { stackFrames: frames, totalFrames: frames.length };
        this.sendResponse(response);
    }
    // ─── Virtual source content ───────────────────────────────────────────────────
    async sourceRequest(response, args) {
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
    async disassembleRequest(response, args) {
        // memoryReference format: "MemoryRead:0xNNNN"
        const parts = args.memoryReference.split(":");
        const type = parts[0];
        const addrPart = parts.length > 1 ? parts[parts.length - 1] : "0x0000";
        const base = parseInt(addrPart, 16) || 0;
        const instrOffset = args.instructionOffset ?? 0;
        const count = args.instructionCount ?? 64;
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
        let rawInstructions;
        if (instrOffset < 0) {
            const beforeCount = -instrOffset;
            const afterCount = Math.max(count - beforeCount, 0);
            const byteBack = Math.min(beforeCount * 2, base);
            const startBefore = (base - byteBack) & 0xFFFF;
            // Pass 1: context before PC (sequential — emulator may not support concurrent)
            const beforeReply = await this.emulator.send({
                cmd: "disassemble", address: startBefore, count: beforeCount + 10, type
            });
            let beforeRaw = (beforeReply.instructions ?? []).filter((i) => i.address < base);
            beforeRaw = beforeRaw.slice(-beforeCount); // keep closest to PC
            // Pass 2: from PC onwards
            const afterReply = await this.emulator.send({
                cmd: "disassemble", address: base, count: afterCount + 1, type
            });
            const afterRaw = (afterReply.instructions ?? []).slice(0, afterCount + 1);
            // Pad front with dummy entries if the backward disassembly didn't yield enough
            const padCount = beforeCount - beforeRaw.length;
            const padRaw = [];
            for (let i = 0; i < padCount; i++) {
                const a = beforeRaw.length > 0
                    ? (beforeRaw[0].address - (padCount - i)) & 0xFFFF
                    : (base - beforeCount + i) & 0xFFFF;
                padRaw.push({ address: a, instruction: "???" });
            }
            rawInstructions = [...padRaw, ...beforeRaw, ...afterRaw];
            console.log(`DAP: DisassembleRequest (2-pass) — base=${addrPart} instrOffset=${instrOffset} startBefore=0x${startBefore.toString(16)} before=${beforeRaw.length}(+${padCount} pad) after=${afterRaw.length}`);
        }
        else {
            const startAddress = (base + (args.offset ?? 0)) & 0xFFFF;
            console.log(`DAP: DisassembleRequest — base=${addrPart} → 0x${startAddress.toString(16).padStart(4, "0")} count=${count}`);
            const reply = await this.emulator.send({ cmd: "disassemble", address: startAddress, type, count });
            rawInstructions = reply.instructions ?? [];
        }
        if (!Array.isArray(rawInstructions)) {
            response.body = { instructions: [] };
            this.sendResponse(response);
            return;
        }
        const instructions = [];
        for (const ins of rawInstructions) {
            const addrStr = "0x" + ins.address.toString(16);
            const labels = this.symbolTable?.getLabelsAt(ins.address) ?? [];
            const entry = {
                address: addrStr,
                instruction: (ins.instruction ?? "").trimEnd(),
                instructionBytes: fmtInstructionBytes(ins.bytes)
            };
            if (labels.length > 0) {
                const label = labels[0];
                const ann = this.sourceAnnotations?.getAnnotation(label);
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
    resolveBpsInRegion(region, bps) {
        const textLines = region.text.split('\n');
        const addresses = [];
        const results = bps.map(bp => {
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
    regionFromDisasmPath(path) {
        // Full URI with scheme — new 3-part format
        let m = path.match(/z80disasm:\/([^/]+)\/(-?\d+)\/([0-9a-fA-F]+)\.z80disasm/i);
        if (!m)
            m = path.match(/z80disasm:\/([0-9a-fA-F]+)\.z80disasm/i); // full URI compat
        if (!m)
            m = path.match(/^\/([^/]+)\/(-?\d+)\/([0-9a-fA-F]+)\.z80disasm$/i); // fsPath 3-part
        if (!m)
            m = path.match(/^\/([0-9a-fA-F]+)\.z80disasm$/i); // fsPath compat
        if (!m)
            return undefined;
        let memType, bank, startAddr;
        if (m.length >= 4) {
            memType = m[1];
            bank = parseInt(m[2], 10);
            startAddr = parseInt(m[3], 16);
        }
        else {
            memType = "read";
            bank = -1;
            startAddr = parseInt(m[1], 16);
        }
        const ref = this.disasmKeyToRef.get(`${memType}:${bank}:${startAddr}`);
        return ref !== undefined ? this.disasmCache.get(ref) : undefined;
    }
    // Merge all registered breakpoints and send the unified list to the emulator.
    async flushBreakpoints() {
        const allAddresses = [];
        for (const addrs of this.bpRegistry.values()) {
            allAddresses.push(...addrs);
        }
        // Deduplicate
        const unique = [...new Set(allAddresses)].map(a => ({ address: a }));
        await this.emulator.send({ cmd: "setBreakpoints", breakpoints: unique });
    }
    // Source breakpoints (virtual disassembly sources)
    async setBreakpointsRequest(response, args) {
        const sourceRef = args.source.sourceReference ?? 0;
        const bps = args.breakpoints ?? [];
        console.log(`DAP: setBreakpointsRequest — sourceRef=${sourceRef} path=${JSON.stringify(args.source.path)} name=${JSON.stringify(args.source.name)} bps=${JSON.stringify(bps.map(b => ({ line: b.line, col: b.column })))}`);
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
    async setInstructionBreakpointsRequest(response, args) {
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
    static parseAddress(s) {
        const t = s.trim();
        let m = t.match(/^(?:0x|\$|#)([0-9a-fA-F]{1,4})$/i);
        if (m) {
            const n = parseInt(m[1], 16);
            return n <= 0xFFFF ? n : undefined;
        }
        if (/^[0-9a-fA-F]{1,4}$/.test(t) && /[a-fA-F]/.test(t)) {
            const n = parseInt(t, 16);
            return n <= 0xFFFF ? n : undefined;
        }
        if (/^\d{1,5}$/.test(t)) {
            const n = parseInt(t, 10);
            return n <= 0xFFFF ? n : undefined;
        }
        return undefined;
    }
    // Label breakpoints — VS Code "function breakpoints" panel, adapted for Z80 assembly labels.
    // Also accepts raw addresses: "0xBB5A", "$BB5A", "BB5A", "47962".
    async setFunctionBreakpointsRequest(response, args) {
        const bps = args.breakpoints ?? [];
        console.log(`DAP: setFunctionBreakpointsRequest — ${bps.length} bp(s): ${JSON.stringify(bps.map(b => b.name))}`);
        const resolved = bps.map(bp => {
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
            .map(bp => parseInt(bp.instructionReference.replace("0x", ""), 16));
        this.bpRegistry.set("func", addresses);
        await this.flushBreakpoints();
        response.body = { breakpoints: resolved };
        this.sendResponse(response);
    }
    completionsRequest(response, args) {
        const prefix = (args.text ?? "").slice(0, (args.column ?? args.text?.length ?? 0) - 1);
        const names = this.symbolTable?.getAllNames() ?? [];
        const lower = prefix.toLowerCase();
        const items = names
            .filter(n => n.toLowerCase().startsWith(lower))
            .map(n => ({ label: n, type: "function" }));
        response.body = { targets: items };
        this.sendResponse(response);
    }
    async stepInRequest(response, args) {
        await this.emulator.send({ cmd: "stepIn" });
        this.sendResponse(response);
        // StoppedEvent will be sent by the async onEvent handler
    }
    async stepOutRequest(response, args) {
        await this.emulator.send({ cmd: "stepOut" });
        this.sendResponse(response);
        // StoppedEvent will be sent by the async onEvent handler
    }
    async evaluateRequest(response, args) {
        try {
            const result = await this.emulator.send({
                cmd: "evaluate",
                expression: args.expression
            });
            response.body = {
                result: result?.text ?? "?",
                variablesReference: 0
            };
        }
        catch {
            response.body = { result: "?", variablesReference: 0 };
        }
        this.sendResponse(response);
    }
    async disconnectRequest(response, args) {
        try {
            await this.emulator.send({ cmd: "continue" });
        }
        catch (_) { }
        this.emulator.disconnect();
        // Kill the emulator process if we spawned it (launch mode only)
        if (this.emulatorProcess && !this.emulatorProcess.killed) {
            this.emulatorProcess.kill();
            this.emulatorProcess = null;
        }
        this.sendResponse(response);
    }
    async restartRequest(response, args) {
        // Invalidate all disassembly caches (memory may have changed after reset)
        this.disasmCache.clear();
        await this.emulator.send({ cmd: "reset" });
        this.sendResponse(response);
        this.sendEvent(new vscode_debugadapter_3.StoppedEvent("entry", 1));
    }
    async readMemoryRequest(response, args) {
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
        const bytes = reply.bytes ?? [];
        response.body = {
            address: "0x" + address.toString(16).padStart(4, "0"),
            data: Buffer.from(bytes).toString("base64")
        };
        this.sendResponse(response);
    }
    async writeMemoryRequest(response, args) {
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
    async setVariableRequest(response, args) {
        if (args.variablesReference !== 1) {
            // Only registers scope is editable
            response.body = { value: args.value, variablesReference: 0 };
            this.sendResponse(response);
            return;
        }
        const val = Number(args.value); // handles "0x1234" and decimal
        const key = args.name.toLowerCase(); // AF→af, AF'→af', etc.
        await this.emulator.send({ cmd: "setRegisters", [key]: val });
        response.body = {
            value: "0x" + (val & 0xFFFF).toString(16).padStart(4, "0"),
            variablesReference: 0
        };
        this.sendResponse(response);
    }
    // ─── Custom requests (called from extension via session.customRequest) ────────
    async _forwardHardwareRequest(response, cmd, errCode) {
        try {
            const result = await this.emulator.send({ cmd });
            response.body = result?.error ? { error: result.error } : result;
            this.sendResponse(response);
        }
        catch (e) {
            this.sendErrorResponse(response, errCode, `${cmd} failed: ${e}`);
        }
    }
    async customRequest(command, response, args) {
        if (command === "getDisasmAt") {
            try {
                const addr = (args?.address ?? 0) & 0xFFFF;
                const memType = args?.memType ?? "read";
                const bank = args?.bank ?? -1;
                const region = await this.ensureRegion(addr, memType, bank);
                response.body = { text: region.text, sourceRef: region.sourceRef };
                this.sendResponse(response);
            }
            catch (e) {
                this.sendErrorResponse(response, 1234, `Disassembly failed: ${e}`);
            }
        }
        else if (command === "getMemBanks") {
            try {
                const result = await this.emulator.send({ cmd: "getMemBanks" });
                // The emulator may return {"error":"unknown command"} for old binaries —
                // that is NOT a throw, so we must check explicitly.
                if (result?.error) {
                    console.log(`DAP: getMemBanks not supported by emulator (${result.error}), using defaults`);
                    response.body = { sources: null }; // signal "not supported"
                }
                else {
                    const sources = Array.isArray(result?.sources) ? result.sources : [];
                    console.log(`DAP: getMemBanks returned ${sources.length} source(s): ` +
                        sources.map((s) => s.label).join(", "));
                    response.body = { sources };
                }
                this.sendResponse(response);
            }
            catch (e) {
                console.log(`DAP: getMemBanks threw: ${e}`);
                this.sendErrorResponse(response, 1235, `getMemBanks failed: ${e}`);
            }
        }
        else if (command === "readMemoryEx") {
            try {
                const address = (args?.address ?? 0) & 0xFFFF;
                const count = args?.count ?? 256;
                const memType = args?.memType ?? "read";
                const bank = args?.bank ?? -1;
                const reply = await this.emulator.send({
                    cmd: "readMemory",
                    address,
                    size: count,
                    memType,
                    bank
                });
                const bytes = reply?.bytes ?? [];
                response.body = { address, bytes };
                this.sendResponse(response);
            }
            catch (e) {
                this.sendErrorResponse(response, 1236, `readMemoryEx failed: ${e}`);
            }
        }
        else if (command === "z80bp") {
            // Accept either a numeric address or a label/address string (from addBreakpointAt)
            let addr;
            if (args?.name !== undefined) {
                const resolved = this.symbolTable?.resolveLabel(args.name) ?? Z80DebugSession.parseAddress(args.name);
                if (resolved === undefined) {
                    this.sendErrorResponse(response, 1237, `z80bp: unknown label or address: ${args.name}`);
                    return;
                }
                addr = resolved & 0xFFFF;
            }
            else {
                addr = (args?.address ?? 0) & 0xFFFF;
            }
            const enable = args?.enable !== false;
            const current = new Set(this.bpRegistry.get("direct") ?? []);
            if (enable)
                current.add(addr);
            else
                current.delete(addr);
            this.bpRegistry.set("direct", [...current]);
            await this.flushBreakpoints();
            console.log(`DAP: z80bp addr=0x${addr.toString(16).padStart(4, "0")} enable=${enable} → direct set size=${current.size}`);
            response.body = { address: addr, enabled: enable };
            this.sendResponse(response);
        }
        else if (command === "getCrtcState") {
            await this._forwardHardwareRequest(response, "getCrtcState", 1240);
        }
        else if (command === "getGateArrayState") {
            await this._forwardHardwareRequest(response, "getGateArrayState", 1241);
        }
        else if (command === "getPsgState") {
            await this._forwardHardwareRequest(response, "getPsgState", 1242);
        }
        else if (command === "getPpiState") {
            await this._forwardHardwareRequest(response, "getPpiState", 1243);
        }
        else if (command === "getFdcState") {
            await this._forwardHardwareRequest(response, "getFdcState", 1244);
        }
        else if (command === "getTapeState") {
            await this._forwardHardwareRequest(response, "getTapeState", 1245);
        }
        else if (command === "getAsicState") {
            await this._forwardHardwareRequest(response, "getAsicState", 1246);
        }
        else if (command === "getTapeSignal") {
            await this._forwardHardwareRequest(response, "getTapeSignal", 1248);
        }
        else if (command === "getTrackRaw") {
            try {
                const result = await this.emulator.send({ cmd: "getTrackRaw", ...args });
                response.body = result?.error ? { error: result.error } : result;
                this.sendResponse(response);
            }
            catch (e) {
                this.sendErrorResponse(response, 1247, `getTrackRaw failed: ${e}`);
            }
        }
        else {
            this.sendErrorResponse(response, 1014, `Unknown custom request: ${command}`);
        }
    }
}
exports.Z80DebugSession = Z80DebugSession;
// ─── Stack trace ──────────────────────────────────────────────────────────────
// CALL opcodes (3-byte instructions → return address is pushed as PC+3)
Z80DebugSession.CALL_OPCODES = new Set([
    0xCD, // CALL nn
    0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC // CALL cc,nn
]);
// RST opcodes (1-byte instructions → return address is pushed as PC+1)
Z80DebugSession.RST_OPCODES = new Set([
    0xC7, 0xCF, 0xD7, 0xDF, 0xE7, 0xEF, 0xF7, 0xFF
]);
//# sourceMappingURL=Z80DebugSession.js.map

/***/ }),
/* 5 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */

Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.Handles = exports.Response = exports.Event = exports.ErrorDestination = exports.CompletionItem = exports.Module = exports.Source = exports.Breakpoint = exports.Variable = exports.Scope = exports.StackFrame = exports.Thread = exports.InvalidatedEvent = exports.ProgressEndEvent = exports.ProgressUpdateEvent = exports.ProgressStartEvent = exports.CapabilitiesEvent = exports.LoadedSourceEvent = exports.ModuleEvent = exports.BreakpointEvent = exports.ThreadEvent = exports.OutputEvent = exports.ContinuedEvent = exports.StoppedEvent = exports.ExitedEvent = exports.TerminatedEvent = exports.InitializedEvent = exports.logger = exports.Logger = exports.LoggingDebugSession = exports.DebugSession = void 0;
const debugSession_1 = __webpack_require__(6);
Object.defineProperty(exports, "DebugSession", ({ enumerable: true, get: function () { return debugSession_1.DebugSession; } }));
Object.defineProperty(exports, "InitializedEvent", ({ enumerable: true, get: function () { return debugSession_1.InitializedEvent; } }));
Object.defineProperty(exports, "TerminatedEvent", ({ enumerable: true, get: function () { return debugSession_1.TerminatedEvent; } }));
Object.defineProperty(exports, "ExitedEvent", ({ enumerable: true, get: function () { return debugSession_1.ExitedEvent; } }));
Object.defineProperty(exports, "StoppedEvent", ({ enumerable: true, get: function () { return debugSession_1.StoppedEvent; } }));
Object.defineProperty(exports, "ContinuedEvent", ({ enumerable: true, get: function () { return debugSession_1.ContinuedEvent; } }));
Object.defineProperty(exports, "OutputEvent", ({ enumerable: true, get: function () { return debugSession_1.OutputEvent; } }));
Object.defineProperty(exports, "ThreadEvent", ({ enumerable: true, get: function () { return debugSession_1.ThreadEvent; } }));
Object.defineProperty(exports, "BreakpointEvent", ({ enumerable: true, get: function () { return debugSession_1.BreakpointEvent; } }));
Object.defineProperty(exports, "ModuleEvent", ({ enumerable: true, get: function () { return debugSession_1.ModuleEvent; } }));
Object.defineProperty(exports, "LoadedSourceEvent", ({ enumerable: true, get: function () { return debugSession_1.LoadedSourceEvent; } }));
Object.defineProperty(exports, "CapabilitiesEvent", ({ enumerable: true, get: function () { return debugSession_1.CapabilitiesEvent; } }));
Object.defineProperty(exports, "ProgressStartEvent", ({ enumerable: true, get: function () { return debugSession_1.ProgressStartEvent; } }));
Object.defineProperty(exports, "ProgressUpdateEvent", ({ enumerable: true, get: function () { return debugSession_1.ProgressUpdateEvent; } }));
Object.defineProperty(exports, "ProgressEndEvent", ({ enumerable: true, get: function () { return debugSession_1.ProgressEndEvent; } }));
Object.defineProperty(exports, "InvalidatedEvent", ({ enumerable: true, get: function () { return debugSession_1.InvalidatedEvent; } }));
Object.defineProperty(exports, "Thread", ({ enumerable: true, get: function () { return debugSession_1.Thread; } }));
Object.defineProperty(exports, "StackFrame", ({ enumerable: true, get: function () { return debugSession_1.StackFrame; } }));
Object.defineProperty(exports, "Scope", ({ enumerable: true, get: function () { return debugSession_1.Scope; } }));
Object.defineProperty(exports, "Variable", ({ enumerable: true, get: function () { return debugSession_1.Variable; } }));
Object.defineProperty(exports, "Breakpoint", ({ enumerable: true, get: function () { return debugSession_1.Breakpoint; } }));
Object.defineProperty(exports, "Source", ({ enumerable: true, get: function () { return debugSession_1.Source; } }));
Object.defineProperty(exports, "Module", ({ enumerable: true, get: function () { return debugSession_1.Module; } }));
Object.defineProperty(exports, "CompletionItem", ({ enumerable: true, get: function () { return debugSession_1.CompletionItem; } }));
Object.defineProperty(exports, "ErrorDestination", ({ enumerable: true, get: function () { return debugSession_1.ErrorDestination; } }));
const loggingDebugSession_1 = __webpack_require__(13);
Object.defineProperty(exports, "LoggingDebugSession", ({ enumerable: true, get: function () { return loggingDebugSession_1.LoggingDebugSession; } }));
const Logger = __webpack_require__(14);
exports.Logger = Logger;
const messages_1 = __webpack_require__(9);
Object.defineProperty(exports, "Event", ({ enumerable: true, get: function () { return messages_1.Event; } }));
Object.defineProperty(exports, "Response", ({ enumerable: true, get: function () { return messages_1.Response; } }));
const handles_1 = __webpack_require__(24);
Object.defineProperty(exports, "Handles", ({ enumerable: true, get: function () { return handles_1.Handles; } }));
const logger = Logger.logger;
exports.logger = logger;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoibWFpbi5qcyIsInNvdXJjZVJvb3QiOiIiLCJzb3VyY2VzIjpbIi4uL3NyYy9tYWluLnRzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiJBQUFBOzs7Z0dBR2dHO0FBQ2hHLFlBQVksQ0FBQzs7O0FBRWIsaURBT3dCO0FBU3ZCLDZGQWZBLDJCQUFZLE9BZUE7QUFJWixpR0FsQkEsK0JBQWdCLE9Ba0JBO0FBQUUsZ0dBbEJBLDhCQUFlLE9Ba0JBO0FBQUUsNEZBbEJBLDBCQUFXLE9Ba0JBO0FBQUUsNkZBbEJBLDJCQUFZLE9Ba0JBO0FBQUUsK0ZBbEJBLDZCQUFjLE9Ba0JBO0FBQUUsNEZBbEJBLDBCQUFXLE9Ba0JBO0FBQUUsNEZBbEJBLDBCQUFXLE9Ba0JBO0FBQUUsZ0dBbEJBLDhCQUFlLE9Ba0JBO0FBQUUsNEZBbEJBLDBCQUFXLE9Ba0JBO0FBQ25JLGtHQWxCQSxnQ0FBaUIsT0FrQkE7QUFBRSxrR0FsQkEsZ0NBQWlCLE9Ba0JBO0FBQUUsbUdBbEJBLGlDQUFrQixPQWtCQTtBQUFFLG9HQWxCQSxrQ0FBbUIsT0FrQkE7QUFBRSxpR0FsQkEsK0JBQWdCLE9Ba0JBO0FBQUUsaUdBbEJBLCtCQUFnQixPQWtCQTtBQUNsSCx1RkFsQkEscUJBQU0sT0FrQkE7QUFBRSwyRkFsQkEseUJBQVUsT0FrQkE7QUFBRSxzRkFsQkEsb0JBQUssT0FrQkE7QUFBRSx5RkFsQkEsdUJBQVEsT0FrQkE7QUFDbkMsMkZBbEJBLHlCQUFVLE9Ba0JBO0FBQUUsdUZBbEJBLHFCQUFNLE9Ba0JBO0FBQUUsdUZBbEJBLHFCQUFNLE9Ba0JBO0FBQUUsK0ZBbEJBLDZCQUFjLE9Ba0JBO0FBQzFDLGlHQWxCQSwrQkFBZ0IsT0FrQkE7QUFoQmpCLCtEQUEwRDtBQVN6RCxvR0FUTyx5Q0FBbUIsT0FTUDtBQVJwQixtQ0FBbUM7QUFTbEMsd0JBQU07QUFSUCx5Q0FBNkM7QUFlNUMsc0ZBZlEsZ0JBQUssT0FlUjtBQUFFLHlGQWZRLG1CQUFRLE9BZVI7QUFkaEIsdUNBQW9DO0FBZW5DLHdGQWZRLGlCQUFPLE9BZVI7QUFiUixNQUFNLE1BQU0sR0FBRyxNQUFNLENBQUMsTUFBTSxDQUFDO0FBTTVCLHdCQUFNIiwic291cmNlc0NvbnRlbnQiOlsiLyogLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cbiAqIENvcHlyaWdodCAoYykgTWljcm9zb2Z0IENvcnBvcmF0aW9uLiBBbGwgcmlnaHRzIHJlc2VydmVkLlxuICogTGljZW5zZWQgdW5kZXIgdGhlIE1JVCBMaWNlbnNlLiBTZWUgTGljZW5zZS50eHQgaW4gdGhlIHByb2plY3Qgcm9vdCBmb3IgbGljZW5zZSBpbmZvcm1hdGlvbi5cbiAqIC0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSAqL1xuJ3VzZSBzdHJpY3QnO1xuXG5pbXBvcnQge1xuXHREZWJ1Z1Nlc3Npb24sXG5cdEluaXRpYWxpemVkRXZlbnQsIFRlcm1pbmF0ZWRFdmVudCwgRXhpdGVkRXZlbnQsIFN0b3BwZWRFdmVudCwgQ29udGludWVkRXZlbnQsIE91dHB1dEV2ZW50LCBUaHJlYWRFdmVudCwgQnJlYWtwb2ludEV2ZW50LCBNb2R1bGVFdmVudCxcblx0XHRMb2FkZWRTb3VyY2VFdmVudCwgQ2FwYWJpbGl0aWVzRXZlbnQsIFByb2dyZXNzU3RhcnRFdmVudCwgUHJvZ3Jlc3NVcGRhdGVFdmVudCwgUHJvZ3Jlc3NFbmRFdmVudCwgSW52YWxpZGF0ZWRFdmVudCxcblx0VGhyZWFkLCBTdGFja0ZyYW1lLCBTY29wZSwgVmFyaWFibGUsXG5cdEJyZWFrcG9pbnQsIFNvdXJjZSwgTW9kdWxlLCBDb21wbGV0aW9uSXRlbSxcblx0RXJyb3JEZXN0aW5hdGlvblxufSBmcm9tICcuL2RlYnVnU2Vzc2lvbic7XG5pbXBvcnQge0xvZ2dpbmdEZWJ1Z1Nlc3Npb259IGZyb20gJy4vbG9nZ2luZ0RlYnVnU2Vzc2lvbic7XG5pbXBvcnQgKiBhcyBMb2dnZXIgZnJvbSAnLi9sb2dnZXInO1xuaW1wb3J0IHsgRXZlbnQsIFJlc3BvbnNlIH0gZnJvbSAnLi9tZXNzYWdlcyc7XG5pbXBvcnQgeyBIYW5kbGVzIH0gZnJvbSAnLi9oYW5kbGVzJztcblxuY29uc3QgbG9nZ2VyID0gTG9nZ2VyLmxvZ2dlcjtcblxuZXhwb3J0IHtcblx0RGVidWdTZXNzaW9uLFxuXHRMb2dnaW5nRGVidWdTZXNzaW9uLFxuXHRMb2dnZXIsXG5cdGxvZ2dlcixcblx0SW5pdGlhbGl6ZWRFdmVudCwgVGVybWluYXRlZEV2ZW50LCBFeGl0ZWRFdmVudCwgU3RvcHBlZEV2ZW50LCBDb250aW51ZWRFdmVudCwgT3V0cHV0RXZlbnQsIFRocmVhZEV2ZW50LCBCcmVha3BvaW50RXZlbnQsIE1vZHVsZUV2ZW50LFxuXHRcdExvYWRlZFNvdXJjZUV2ZW50LCBDYXBhYmlsaXRpZXNFdmVudCwgUHJvZ3Jlc3NTdGFydEV2ZW50LCBQcm9ncmVzc1VwZGF0ZUV2ZW50LCBQcm9ncmVzc0VuZEV2ZW50LCBJbnZhbGlkYXRlZEV2ZW50LFxuXHRUaHJlYWQsIFN0YWNrRnJhbWUsIFNjb3BlLCBWYXJpYWJsZSxcblx0QnJlYWtwb2ludCwgU291cmNlLCBNb2R1bGUsIENvbXBsZXRpb25JdGVtLFxuXHRFcnJvckRlc3RpbmF0aW9uLFxuXHRFdmVudCwgUmVzcG9uc2UsXG5cdEhhbmRsZXNcbn1cbiJdfQ==

/***/ }),
/* 6 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.DebugSession = exports.ErrorDestination = exports.InvalidatedEvent = exports.ProgressEndEvent = exports.ProgressUpdateEvent = exports.ProgressStartEvent = exports.CapabilitiesEvent = exports.LoadedSourceEvent = exports.ModuleEvent = exports.BreakpointEvent = exports.ThreadEvent = exports.OutputEvent = exports.ExitedEvent = exports.TerminatedEvent = exports.InitializedEvent = exports.ContinuedEvent = exports.StoppedEvent = exports.CompletionItem = exports.Module = exports.Breakpoint = exports.Variable = exports.Thread = exports.StackFrame = exports.Scope = exports.Source = void 0;
const protocol_1 = __webpack_require__(7);
const messages_1 = __webpack_require__(9);
const runDebugAdapter_1 = __webpack_require__(10);
const url_1 = __webpack_require__(12);
class Source {
    constructor(name, path, id = 0, origin, data) {
        this.name = name;
        this.path = path;
        this.sourceReference = id;
        if (origin) {
            this.origin = origin;
        }
        if (data) {
            this.adapterData = data;
        }
    }
}
exports.Source = Source;
class Scope {
    constructor(name, reference, expensive = false) {
        this.name = name;
        this.variablesReference = reference;
        this.expensive = expensive;
    }
}
exports.Scope = Scope;
class StackFrame {
    constructor(i, nm, src, ln = 0, col = 0) {
        this.id = i;
        this.source = src;
        this.line = ln;
        this.column = col;
        this.name = nm;
    }
}
exports.StackFrame = StackFrame;
class Thread {
    constructor(id, name) {
        this.id = id;
        if (name) {
            this.name = name;
        }
        else {
            this.name = 'Thread #' + id;
        }
    }
}
exports.Thread = Thread;
class Variable {
    constructor(name, value, ref = 0, indexedVariables, namedVariables) {
        this.name = name;
        this.value = value;
        this.variablesReference = ref;
        if (typeof namedVariables === 'number') {
            this.namedVariables = namedVariables;
        }
        if (typeof indexedVariables === 'number') {
            this.indexedVariables = indexedVariables;
        }
    }
}
exports.Variable = Variable;
class Breakpoint {
    constructor(verified, line, column, source) {
        this.verified = verified;
        const e = this;
        if (typeof line === 'number') {
            e.line = line;
        }
        if (typeof column === 'number') {
            e.column = column;
        }
        if (source) {
            e.source = source;
        }
    }
    setId(id) {
        this.id = id;
    }
}
exports.Breakpoint = Breakpoint;
class Module {
    constructor(id, name) {
        this.id = id;
        this.name = name;
    }
}
exports.Module = Module;
class CompletionItem {
    constructor(label, start, length = 0) {
        this.label = label;
        this.start = start;
        this.length = length;
    }
}
exports.CompletionItem = CompletionItem;
class StoppedEvent extends messages_1.Event {
    constructor(reason, threadId, exceptionText) {
        super('stopped');
        this.body = {
            reason: reason
        };
        if (typeof threadId === 'number') {
            this.body.threadId = threadId;
        }
        if (typeof exceptionText === 'string') {
            this.body.text = exceptionText;
        }
    }
}
exports.StoppedEvent = StoppedEvent;
class ContinuedEvent extends messages_1.Event {
    constructor(threadId, allThreadsContinued) {
        super('continued');
        this.body = {
            threadId: threadId
        };
        if (typeof allThreadsContinued === 'boolean') {
            this.body.allThreadsContinued = allThreadsContinued;
        }
    }
}
exports.ContinuedEvent = ContinuedEvent;
class InitializedEvent extends messages_1.Event {
    constructor() {
        super('initialized');
    }
}
exports.InitializedEvent = InitializedEvent;
class TerminatedEvent extends messages_1.Event {
    constructor(restart) {
        super('terminated');
        if (typeof restart === 'boolean' || restart) {
            const e = this;
            e.body = {
                restart: restart
            };
        }
    }
}
exports.TerminatedEvent = TerminatedEvent;
class ExitedEvent extends messages_1.Event {
    constructor(exitCode) {
        super('exited');
        this.body = {
            exitCode: exitCode
        };
    }
}
exports.ExitedEvent = ExitedEvent;
class OutputEvent extends messages_1.Event {
    constructor(output, category = 'console', data) {
        super('output');
        this.body = {
            category: category,
            output: output
        };
        if (data !== undefined) {
            this.body.data = data;
        }
    }
}
exports.OutputEvent = OutputEvent;
class ThreadEvent extends messages_1.Event {
    constructor(reason, threadId) {
        super('thread');
        this.body = {
            reason: reason,
            threadId: threadId
        };
    }
}
exports.ThreadEvent = ThreadEvent;
class BreakpointEvent extends messages_1.Event {
    constructor(reason, breakpoint) {
        super('breakpoint');
        this.body = {
            reason: reason,
            breakpoint: breakpoint
        };
    }
}
exports.BreakpointEvent = BreakpointEvent;
class ModuleEvent extends messages_1.Event {
    constructor(reason, module) {
        super('module');
        this.body = {
            reason: reason,
            module: module
        };
    }
}
exports.ModuleEvent = ModuleEvent;
class LoadedSourceEvent extends messages_1.Event {
    constructor(reason, source) {
        super('loadedSource');
        this.body = {
            reason: reason,
            source: source
        };
    }
}
exports.LoadedSourceEvent = LoadedSourceEvent;
class CapabilitiesEvent extends messages_1.Event {
    constructor(capabilities) {
        super('capabilities');
        this.body = {
            capabilities: capabilities
        };
    }
}
exports.CapabilitiesEvent = CapabilitiesEvent;
class ProgressStartEvent extends messages_1.Event {
    constructor(progressId, title, message) {
        super('progressStart');
        this.body = {
            progressId: progressId,
            title: title
        };
        if (typeof message === 'string') {
            this.body.message = message;
        }
    }
}
exports.ProgressStartEvent = ProgressStartEvent;
class ProgressUpdateEvent extends messages_1.Event {
    constructor(progressId, message) {
        super('progressUpdate');
        this.body = {
            progressId: progressId
        };
        if (typeof message === 'string') {
            this.body.message = message;
        }
    }
}
exports.ProgressUpdateEvent = ProgressUpdateEvent;
class ProgressEndEvent extends messages_1.Event {
    constructor(progressId, message) {
        super('progressEnd');
        this.body = {
            progressId: progressId
        };
        if (typeof message === 'string') {
            this.body.message = message;
        }
    }
}
exports.ProgressEndEvent = ProgressEndEvent;
class InvalidatedEvent extends messages_1.Event {
    constructor(areas, threadId, stackFrameId) {
        super('invalidated');
        this.body = {};
        if (areas) {
            this.body.areas = areas;
        }
        if (threadId) {
            this.body.threadId = threadId;
        }
        if (stackFrameId) {
            this.body.stackFrameId = stackFrameId;
        }
    }
}
exports.InvalidatedEvent = InvalidatedEvent;
var ErrorDestination;
(function (ErrorDestination) {
    ErrorDestination[ErrorDestination["User"] = 1] = "User";
    ErrorDestination[ErrorDestination["Telemetry"] = 2] = "Telemetry";
})(ErrorDestination = exports.ErrorDestination || (exports.ErrorDestination = {}));
;
class DebugSession extends protocol_1.ProtocolServer {
    constructor(obsolete_debuggerLinesAndColumnsStartAt1, obsolete_isServer) {
        super();
        const linesAndColumnsStartAt1 = typeof obsolete_debuggerLinesAndColumnsStartAt1 === 'boolean' ? obsolete_debuggerLinesAndColumnsStartAt1 : false;
        this._debuggerLinesStartAt1 = linesAndColumnsStartAt1;
        this._debuggerColumnsStartAt1 = linesAndColumnsStartAt1;
        this._debuggerPathsAreURIs = false;
        this._clientLinesStartAt1 = true;
        this._clientColumnsStartAt1 = true;
        this._clientPathsAreURIs = false;
        this._isServer = typeof obsolete_isServer === 'boolean' ? obsolete_isServer : false;
        this.on('close', () => {
            this.shutdown();
        });
        this.on('error', (error) => {
            this.shutdown();
        });
    }
    setDebuggerPathFormat(format) {
        this._debuggerPathsAreURIs = format !== 'path';
    }
    setDebuggerLinesStartAt1(enable) {
        this._debuggerLinesStartAt1 = enable;
    }
    setDebuggerColumnsStartAt1(enable) {
        this._debuggerColumnsStartAt1 = enable;
    }
    setRunAsServer(enable) {
        this._isServer = enable;
    }
    /**
     * A virtual constructor...
     */
    static run(debugSession) {
        (0, runDebugAdapter_1.runDebugAdapter)(debugSession);
    }
    shutdown() {
        if (this._isServer || this._isRunningInline()) {
            // shutdown ignored in server mode
        }
        else {
            // wait a bit before shutting down
            setTimeout(() => {
                process.exit(0);
            }, 100);
        }
    }
    sendErrorResponse(response, codeOrMessage, format, variables, dest = ErrorDestination.User) {
        let msg;
        if (typeof codeOrMessage === 'number') {
            msg = {
                id: codeOrMessage,
                format: format
            };
            if (variables) {
                msg.variables = variables;
            }
            if (dest & ErrorDestination.User) {
                msg.showUser = true;
            }
            if (dest & ErrorDestination.Telemetry) {
                msg.sendTelemetry = true;
            }
        }
        else {
            msg = codeOrMessage;
        }
        response.success = false;
        response.message = DebugSession.formatPII(msg.format, true, msg.variables);
        if (!response.body) {
            response.body = {};
        }
        response.body.error = msg;
        this.sendResponse(response);
    }
    runInTerminalRequest(args, timeout, cb) {
        this.sendRequest('runInTerminal', args, timeout, cb);
    }
    dispatchRequest(request) {
        const response = new messages_1.Response(request);
        try {
            if (request.command === 'initialize') {
                var args = request.arguments;
                if (typeof args.linesStartAt1 === 'boolean') {
                    this._clientLinesStartAt1 = args.linesStartAt1;
                }
                if (typeof args.columnsStartAt1 === 'boolean') {
                    this._clientColumnsStartAt1 = args.columnsStartAt1;
                }
                if (args.pathFormat !== 'path') {
                    this.sendErrorResponse(response, 2018, 'debug adapter only supports native paths', null, ErrorDestination.Telemetry);
                }
                else {
                    const initializeResponse = response;
                    initializeResponse.body = {};
                    this.initializeRequest(initializeResponse, args);
                }
            }
            else if (request.command === 'launch') {
                this.launchRequest(response, request.arguments, request);
            }
            else if (request.command === 'attach') {
                this.attachRequest(response, request.arguments, request);
            }
            else if (request.command === 'disconnect') {
                this.disconnectRequest(response, request.arguments, request);
            }
            else if (request.command === 'terminate') {
                this.terminateRequest(response, request.arguments, request);
            }
            else if (request.command === 'restart') {
                this.restartRequest(response, request.arguments, request);
            }
            else if (request.command === 'setBreakpoints') {
                this.setBreakPointsRequest(response, request.arguments, request);
            }
            else if (request.command === 'setFunctionBreakpoints') {
                this.setFunctionBreakPointsRequest(response, request.arguments, request);
            }
            else if (request.command === 'setExceptionBreakpoints') {
                this.setExceptionBreakPointsRequest(response, request.arguments, request);
            }
            else if (request.command === 'configurationDone') {
                this.configurationDoneRequest(response, request.arguments, request);
            }
            else if (request.command === 'continue') {
                this.continueRequest(response, request.arguments, request);
            }
            else if (request.command === 'next') {
                this.nextRequest(response, request.arguments, request);
            }
            else if (request.command === 'stepIn') {
                this.stepInRequest(response, request.arguments, request);
            }
            else if (request.command === 'stepOut') {
                this.stepOutRequest(response, request.arguments, request);
            }
            else if (request.command === 'stepBack') {
                this.stepBackRequest(response, request.arguments, request);
            }
            else if (request.command === 'reverseContinue') {
                this.reverseContinueRequest(response, request.arguments, request);
            }
            else if (request.command === 'restartFrame') {
                this.restartFrameRequest(response, request.arguments, request);
            }
            else if (request.command === 'goto') {
                this.gotoRequest(response, request.arguments, request);
            }
            else if (request.command === 'pause') {
                this.pauseRequest(response, request.arguments, request);
            }
            else if (request.command === 'stackTrace') {
                this.stackTraceRequest(response, request.arguments, request);
            }
            else if (request.command === 'scopes') {
                this.scopesRequest(response, request.arguments, request);
            }
            else if (request.command === 'variables') {
                this.variablesRequest(response, request.arguments, request);
            }
            else if (request.command === 'setVariable') {
                this.setVariableRequest(response, request.arguments, request);
            }
            else if (request.command === 'setExpression') {
                this.setExpressionRequest(response, request.arguments, request);
            }
            else if (request.command === 'source') {
                this.sourceRequest(response, request.arguments, request);
            }
            else if (request.command === 'threads') {
                this.threadsRequest(response, request);
            }
            else if (request.command === 'terminateThreads') {
                this.terminateThreadsRequest(response, request.arguments, request);
            }
            else if (request.command === 'evaluate') {
                this.evaluateRequest(response, request.arguments, request);
            }
            else if (request.command === 'stepInTargets') {
                this.stepInTargetsRequest(response, request.arguments, request);
            }
            else if (request.command === 'gotoTargets') {
                this.gotoTargetsRequest(response, request.arguments, request);
            }
            else if (request.command === 'completions') {
                this.completionsRequest(response, request.arguments, request);
            }
            else if (request.command === 'exceptionInfo') {
                this.exceptionInfoRequest(response, request.arguments, request);
            }
            else if (request.command === 'loadedSources') {
                this.loadedSourcesRequest(response, request.arguments, request);
            }
            else if (request.command === 'dataBreakpointInfo') {
                this.dataBreakpointInfoRequest(response, request.arguments, request);
            }
            else if (request.command === 'setDataBreakpoints') {
                this.setDataBreakpointsRequest(response, request.arguments, request);
            }
            else if (request.command === 'readMemory') {
                this.readMemoryRequest(response, request.arguments, request);
            }
            else if (request.command === 'writeMemory') {
                this.writeMemoryRequest(response, request.arguments, request);
            }
            else if (request.command === 'disassemble') {
                this.disassembleRequest(response, request.arguments, request);
            }
            else if (request.command === 'cancel') {
                this.cancelRequest(response, request.arguments, request);
            }
            else if (request.command === 'breakpointLocations') {
                this.breakpointLocationsRequest(response, request.arguments, request);
            }
            else if (request.command === 'setInstructionBreakpoints') {
                this.setInstructionBreakpointsRequest(response, request.arguments, request);
            }
            else {
                this.customRequest(request.command, response, request.arguments, request);
            }
        }
        catch (e) {
            this.sendErrorResponse(response, 1104, '{_stack}', { _exception: e.message, _stack: e.stack }, ErrorDestination.Telemetry);
        }
    }
    initializeRequest(response, args) {
        // This default debug adapter does not support conditional breakpoints.
        response.body.supportsConditionalBreakpoints = false;
        // This default debug adapter does not support hit conditional breakpoints.
        response.body.supportsHitConditionalBreakpoints = false;
        // This default debug adapter does not support function breakpoints.
        response.body.supportsFunctionBreakpoints = false;
        // This default debug adapter implements the 'configurationDone' request.
        response.body.supportsConfigurationDoneRequest = true;
        // This default debug adapter does not support hovers based on the 'evaluate' request.
        response.body.supportsEvaluateForHovers = false;
        // This default debug adapter does not support the 'stepBack' request.
        response.body.supportsStepBack = false;
        // This default debug adapter does not support the 'setVariable' request.
        response.body.supportsSetVariable = false;
        // This default debug adapter does not support the 'restartFrame' request.
        response.body.supportsRestartFrame = false;
        // This default debug adapter does not support the 'stepInTargets' request.
        response.body.supportsStepInTargetsRequest = false;
        // This default debug adapter does not support the 'gotoTargets' request.
        response.body.supportsGotoTargetsRequest = false;
        // This default debug adapter does not support the 'completions' request.
        response.body.supportsCompletionsRequest = false;
        // This default debug adapter does not support the 'restart' request.
        response.body.supportsRestartRequest = false;
        // This default debug adapter does not support the 'exceptionOptions' attribute on the 'setExceptionBreakpoints' request.
        response.body.supportsExceptionOptions = false;
        // This default debug adapter does not support the 'format' attribute on the 'variables', 'evaluate', and 'stackTrace' request.
        response.body.supportsValueFormattingOptions = false;
        // This debug adapter does not support the 'exceptionInfo' request.
        response.body.supportsExceptionInfoRequest = false;
        // This debug adapter does not support the 'TerminateDebuggee' attribute on the 'disconnect' request.
        response.body.supportTerminateDebuggee = false;
        // This debug adapter does not support delayed loading of stack frames.
        response.body.supportsDelayedStackTraceLoading = false;
        // This debug adapter does not support the 'loadedSources' request.
        response.body.supportsLoadedSourcesRequest = false;
        // This debug adapter does not support the 'logMessage' attribute of the SourceBreakpoint.
        response.body.supportsLogPoints = false;
        // This debug adapter does not support the 'terminateThreads' request.
        response.body.supportsTerminateThreadsRequest = false;
        // This debug adapter does not support the 'setExpression' request.
        response.body.supportsSetExpression = false;
        // This debug adapter does not support the 'terminate' request.
        response.body.supportsTerminateRequest = false;
        // This debug adapter does not support data breakpoints.
        response.body.supportsDataBreakpoints = false;
        /** This debug adapter does not support the 'readMemory' request. */
        response.body.supportsReadMemoryRequest = false;
        /** The debug adapter does not support the 'disassemble' request. */
        response.body.supportsDisassembleRequest = false;
        /** The debug adapter does not support the 'cancel' request. */
        response.body.supportsCancelRequest = false;
        /** The debug adapter does not support the 'breakpointLocations' request. */
        response.body.supportsBreakpointLocationsRequest = false;
        /** The debug adapter does not support the 'clipboard' context value in the 'evaluate' request. */
        response.body.supportsClipboardContext = false;
        /** The debug adapter does not support stepping granularities for the stepping requests. */
        response.body.supportsSteppingGranularity = false;
        /** The debug adapter does not support the 'setInstructionBreakpoints' request. */
        response.body.supportsInstructionBreakpoints = false;
        /** The debug adapter does not support 'filterOptions' on the 'setExceptionBreakpoints' request. */
        response.body.supportsExceptionFilterOptions = false;
        this.sendResponse(response);
    }
    disconnectRequest(response, args, request) {
        this.sendResponse(response);
        this.shutdown();
    }
    launchRequest(response, args, request) {
        this.sendResponse(response);
    }
    attachRequest(response, args, request) {
        this.sendResponse(response);
    }
    terminateRequest(response, args, request) {
        this.sendResponse(response);
    }
    restartRequest(response, args, request) {
        this.sendResponse(response);
    }
    setBreakPointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    setFunctionBreakPointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    setExceptionBreakPointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    configurationDoneRequest(response, args, request) {
        this.sendResponse(response);
    }
    continueRequest(response, args, request) {
        this.sendResponse(response);
    }
    nextRequest(response, args, request) {
        this.sendResponse(response);
    }
    stepInRequest(response, args, request) {
        this.sendResponse(response);
    }
    stepOutRequest(response, args, request) {
        this.sendResponse(response);
    }
    stepBackRequest(response, args, request) {
        this.sendResponse(response);
    }
    reverseContinueRequest(response, args, request) {
        this.sendResponse(response);
    }
    restartFrameRequest(response, args, request) {
        this.sendResponse(response);
    }
    gotoRequest(response, args, request) {
        this.sendResponse(response);
    }
    pauseRequest(response, args, request) {
        this.sendResponse(response);
    }
    sourceRequest(response, args, request) {
        this.sendResponse(response);
    }
    threadsRequest(response, request) {
        this.sendResponse(response);
    }
    terminateThreadsRequest(response, args, request) {
        this.sendResponse(response);
    }
    stackTraceRequest(response, args, request) {
        this.sendResponse(response);
    }
    scopesRequest(response, args, request) {
        this.sendResponse(response);
    }
    variablesRequest(response, args, request) {
        this.sendResponse(response);
    }
    setVariableRequest(response, args, request) {
        this.sendResponse(response);
    }
    setExpressionRequest(response, args, request) {
        this.sendResponse(response);
    }
    evaluateRequest(response, args, request) {
        this.sendResponse(response);
    }
    stepInTargetsRequest(response, args, request) {
        this.sendResponse(response);
    }
    gotoTargetsRequest(response, args, request) {
        this.sendResponse(response);
    }
    completionsRequest(response, args, request) {
        this.sendResponse(response);
    }
    exceptionInfoRequest(response, args, request) {
        this.sendResponse(response);
    }
    loadedSourcesRequest(response, args, request) {
        this.sendResponse(response);
    }
    dataBreakpointInfoRequest(response, args, request) {
        this.sendResponse(response);
    }
    setDataBreakpointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    readMemoryRequest(response, args, request) {
        this.sendResponse(response);
    }
    writeMemoryRequest(response, args, request) {
        this.sendResponse(response);
    }
    disassembleRequest(response, args, request) {
        this.sendResponse(response);
    }
    cancelRequest(response, args, request) {
        this.sendResponse(response);
    }
    breakpointLocationsRequest(response, args, request) {
        this.sendResponse(response);
    }
    setInstructionBreakpointsRequest(response, args, request) {
        this.sendResponse(response);
    }
    /**
     * Override this hook to implement custom requests.
     */
    customRequest(command, response, args, request) {
        this.sendErrorResponse(response, 1014, 'unrecognized request', null, ErrorDestination.Telemetry);
    }
    //---- protected -------------------------------------------------------------------------------------------------
    convertClientLineToDebugger(line) {
        if (this._debuggerLinesStartAt1) {
            return this._clientLinesStartAt1 ? line : line + 1;
        }
        return this._clientLinesStartAt1 ? line - 1 : line;
    }
    convertDebuggerLineToClient(line) {
        if (this._debuggerLinesStartAt1) {
            return this._clientLinesStartAt1 ? line : line - 1;
        }
        return this._clientLinesStartAt1 ? line + 1 : line;
    }
    convertClientColumnToDebugger(column) {
        if (this._debuggerColumnsStartAt1) {
            return this._clientColumnsStartAt1 ? column : column + 1;
        }
        return this._clientColumnsStartAt1 ? column - 1 : column;
    }
    convertDebuggerColumnToClient(column) {
        if (this._debuggerColumnsStartAt1) {
            return this._clientColumnsStartAt1 ? column : column - 1;
        }
        return this._clientColumnsStartAt1 ? column + 1 : column;
    }
    convertClientPathToDebugger(clientPath) {
        if (this._clientPathsAreURIs !== this._debuggerPathsAreURIs) {
            if (this._clientPathsAreURIs) {
                return DebugSession.uri2path(clientPath);
            }
            else {
                return DebugSession.path2uri(clientPath);
            }
        }
        return clientPath;
    }
    convertDebuggerPathToClient(debuggerPath) {
        if (this._debuggerPathsAreURIs !== this._clientPathsAreURIs) {
            if (this._debuggerPathsAreURIs) {
                return DebugSession.uri2path(debuggerPath);
            }
            else {
                return DebugSession.path2uri(debuggerPath);
            }
        }
        return debuggerPath;
    }
    //---- private -------------------------------------------------------------------------------
    static path2uri(path) {
        if (process.platform === 'win32') {
            if (/^[A-Z]:/.test(path)) {
                path = path[0].toLowerCase() + path.substr(1);
            }
            path = path.replace(/\\/g, '/');
        }
        path = encodeURI(path);
        let uri = new url_1.URL(`file:`); // ignore 'path' for now
        uri.pathname = path; // now use 'path' to get the correct percent encoding (see https://url.spec.whatwg.org)
        return uri.toString();
    }
    static uri2path(sourceUri) {
        let uri = new url_1.URL(sourceUri);
        let s = decodeURIComponent(uri.pathname);
        if (process.platform === 'win32') {
            if (/^\/[a-zA-Z]:/.test(s)) {
                s = s[1].toLowerCase() + s.substr(2);
            }
            s = s.replace(/\//g, '\\');
        }
        return s;
    }
    /*
    * If argument starts with '_' it is OK to send its value to telemetry.
    */
    static formatPII(format, excludePII, args) {
        return format.replace(DebugSession._formatPIIRegexp, function (match, paramName) {
            if (excludePII && paramName.length > 0 && paramName[0] !== '_') {
                return match;
            }
            return args[paramName] && args.hasOwnProperty(paramName) ?
                args[paramName] :
                match;
        });
    }
}
exports.DebugSession = DebugSession;
DebugSession._formatPIIRegexp = /{([^}]+)}/g;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiZGVidWdTZXNzaW9uLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXMiOlsiLi4vc3JjL2RlYnVnU2Vzc2lvbi50cyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiO0FBQUE7OztnR0FHZ0c7OztBQUdoRyx5Q0FBNEM7QUFDNUMseUNBQTZDO0FBQzdDLHVEQUFvRDtBQUNwRCw2QkFBMEI7QUFHMUIsTUFBYSxNQUFNO0lBS2xCLFlBQW1CLElBQVksRUFBRSxJQUFhLEVBQUUsS0FBYSxDQUFDLEVBQUUsTUFBZSxFQUFFLElBQVU7UUFDMUYsSUFBSSxDQUFDLElBQUksR0FBRyxJQUFJLENBQUM7UUFDakIsSUFBSSxDQUFDLElBQUksR0FBRyxJQUFJLENBQUM7UUFDakIsSUFBSSxDQUFDLGVBQWUsR0FBRyxFQUFFLENBQUM7UUFDMUIsSUFBSSxNQUFNLEVBQUU7WUFDTCxJQUFLLENBQUMsTUFBTSxHQUFHLE1BQU0sQ0FBQztTQUM1QjtRQUNELElBQUksSUFBSSxFQUFFO1lBQ0gsSUFBSyxDQUFDLFdBQVcsR0FBRyxJQUFJLENBQUM7U0FDL0I7SUFDRixDQUFDO0NBQ0Q7QUFoQkQsd0JBZ0JDO0FBRUQsTUFBYSxLQUFLO0lBS2pCLFlBQW1CLElBQVksRUFBRSxTQUFpQixFQUFFLFlBQXFCLEtBQUs7UUFDN0UsSUFBSSxDQUFDLElBQUksR0FBRyxJQUFJLENBQUM7UUFDakIsSUFBSSxDQUFDLGtCQUFrQixHQUFHLFNBQVMsQ0FBQztRQUNwQyxJQUFJLENBQUMsU0FBUyxHQUFHLFNBQVMsQ0FBQztJQUM1QixDQUFDO0NBQ0Q7QUFWRCxzQkFVQztBQUVELE1BQWEsVUFBVTtJQWF0QixZQUFtQixDQUFTLEVBQUUsRUFBVSxFQUFFLEdBQVksRUFBRSxLQUFhLENBQUMsRUFBRSxNQUFjLENBQUM7UUFDdEYsSUFBSSxDQUFDLEVBQUUsR0FBRyxDQUFDLENBQUM7UUFDWixJQUFJLENBQUMsTUFBTSxHQUFHLEdBQUcsQ0FBQztRQUNsQixJQUFJLENBQUMsSUFBSSxHQUFHLEVBQUUs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RUFBRSxVQUFtQixFQUFFLElBQTZCO1FBQ3pGLE9BQU8sTUFBTSxDQUFDLE9BQU8sQ0FBQyxZQUFZLENBQUMsZ0JBQWdCLEVBQUUsVUFBUyxLQUFLLEVBQUUsU0FBUztZQUM3RSxJQUFJLFVBQVUsSUFBSSxTQUFTLENBQUMsTUFBTSxHQUFHLENBQUMsSUFBSSxTQUFTLENBQUMsQ0FBQyxDQUFDLEtBQUssR0FBRyxFQUFFO2dCQUMvRCxPQUFPLEtBQUssQ0FBQzthQUNiO1lBQ0QsT0FBTyxJQUFJLENBQUMsU0FBUyxDQUFDLElBQUksSUFBSSxDQUFDLGNBQWMsQ0FBQyxTQUFTLENBQUMsQ0FBQyxDQUFDO2dCQUN6RCxJQUFJLENBQUMsU0FBUyxDQUFDLENBQUMsQ0FBQztnQkFDakIsS0FBSyxDQUFDO1FBQ1IsQ0FBQyxDQUFDLENBQUE7SUFDSCxDQUFDOztBQXhtQkYsb0NBeW1CQztBQWZlLDZCQUFnQixHQUFHLFlBQVksQ0FBQyIsInNvdXJjZXNDb250ZW50IjpbIi8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tXG4gKiAgQ29weXJpZ2h0IChjKSBNaWNyb3NvZnQgQ29ycG9yYXRpb24uIEFsbCByaWdodHMgcmVzZXJ2ZWQuXG4gKiAgTGljZW5zZWQgdW5kZXIgdGhlIE1JVCBMaWNlbnNlLiBTZWUgTGljZW5zZS50eHQgaW4gdGhlIHByb2plY3Qgcm9vdCBmb3IgbGljZW5zZSBpbmZvcm1hdGlvbi5cbiAqLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG5pbXBvcnQgeyBEZWJ1Z1Byb3RvY29sIH0gZnJvbSAndnNjb2RlLWRlYnVncHJvdG9jb2wnO1xuaW1wb3J0IHsgUHJvdG9jb2xTZXJ2ZXIgfSBmcm9tICcuL3Byb3RvY29sJztcbmltcG9ydCB7IFJlc3BvbnNlLCBFdmVudCB9IGZyb20gJy4vbWVzc2FnZXMnO1xuaW1wb3J0IHsgcnVuRGVidWdBZGFwdGVyIH0gZnJvbSAnLi9ydW5EZWJ1Z0FkYXB0ZXInO1xuaW1wb3J0IHsgVVJMIH0gZnJvbSAndXJsJztcblxuXG5leHBvcnQgY2xhc3MgU291cmNlIGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Tb3VyY2Uge1xuXHRuYW1lOiBzdHJpbmc7XG5cdHBhdGg6IHN0cmluZztcblx0c291cmNlUmVmZXJlbmNlOiBudW1iZXI7XG5cblx0cHVibGljIGNvbnN0cnVjdG9yKG5hbWU6IHN0cmluZywgcGF0aD86IHN0cmluZywgaWQ6IG51bWJlciA9IDAsIG9yaWdpbj86IHN0cmluZywgZGF0YT86IGFueSkge1xuXHRcdHRoaXMubmFtZSA9IG5hbWU7XG5cdFx0dGhpcy5wYXRoID0gcGF0aDtcblx0XHR0aGlzLnNvdXJjZVJlZmVyZW5jZSA9IGlkO1xuXHRcdGlmIChvcmlnaW4pIHtcblx0XHRcdCg8YW55PnRoaXMpLm9yaWdpbiA9IG9yaWdpbjtcblx0XHR9XG5cdFx0aWYgKGRhdGEpIHtcblx0XHRcdCg8YW55PnRoaXMpLmFkYXB0ZXJEYXRhID0gZGF0YTtcblx0XHR9XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIFNjb3BlIGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5TY29wZSB7XG5cdG5hbWU6IHN0cmluZztcblx0dmFyaWFibGVzUmVmZXJlbmNlOiBudW1iZXI7XG5cdGV4cGVuc2l2ZTogYm9vbGVhbjtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IobmFtZTogc3RyaW5nLCByZWZlcmVuY2U6IG51bWJlciwgZXhwZW5zaXZlOiBib29sZWFuID0gZmFsc2UpIHtcblx0XHR0aGlzLm5hbWUgPSBuYW1lO1xuXHRcdHRoaXMudmFyaWFibGVzUmVmZXJlbmNlID0gcmVmZXJlbmNlO1xuXHRcdHRoaXMuZXhwZW5zaXZlID0gZXhwZW5zaXZlO1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBTdGFja0ZyYW1lIGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5TdGFja0ZyYW1lIHtcblx0aWQ6IG51bWJlcjtcblx0bmFtZTogc3RyaW5nO1xuXHRzb3VyY2U/OiBEZWJ1Z1Byb3RvY29sLlNvdXJjZTtcblx0bGluZTogbnVtYmVyO1xuXHRjb2x1bW46IG51bWJlcjtcblx0ZW5kTGluZT86IG51bWJlcjtcblx0ZW5kQ29sdW1uPzogbnVtYmVyO1xuXHRjYW5SZXN0YXJ0PzogYm9vbGVhbjtcblx0aW5zdHJ1Y3Rpb25Qb2ludGVyUmVmZXJlbmNlPzogc3RyaW5nO1xuXHRtb2R1bGVJZD86IG51bWJlciB8IHN0cmluZztcblx0cHJlc2VudGF0aW9uSGludD86ICdub3JtYWwnIHwgJ2xhYmVsJyB8ICdzdWJ0bGUnO1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihpOiBudW1iZXIsIG5tOiBzdHJpbmcsIHNyYz86IFNvdXJjZSwgbG46IG51bWJlciA9IDAsIGNvbDogbnVtYmVyID0gMCkge1xuXHRcdHRoaXMuaWQgPSBpO1xuXHRcdHRoaXMuc291cmNlID0gc3JjO1xuXHRcdHRoaXMubGluZSA9IGxuO1xuXHRcdHRoaXMuY29sdW1uID0gY29sO1xuXHRcdHRoaXMubmFtZSA9IG5tO1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBUaHJlYWQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLlRocmVhZCB7XG5cdGlkOiBudW1iZXI7XG5cdG5hbWU6IHN0cmluZztcblxuXHRwdWJsaWMgY29uc3RydWN0b3IoaWQ6IG51bWJlciwgbmFtZTogc3RyaW5nKSB7XG5cdFx0dGhpcy5pZCA9IGlkO1xuXHRcdGlmIChuYW1lKSB7XG5cdFx0XHR0aGlzLm5hbWUgPSBuYW1lO1xuXHRcdH0gZWxzZSB7XG5cdFx0XHR0aGlzLm5hbWUgPSAnVGhyZWFkICMnICsgaWQ7XG5cdFx0fVxuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBWYXJpYWJsZSBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuVmFyaWFibGUge1xuXHRuYW1lOiBzdHJpbmc7XG5cdHZhbHVlOiBzdHJpbmc7XG5cdHZhcmlhYmxlc1JlZmVyZW5jZTogbnVtYmVyO1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihuYW1lOiBzdHJpbmcsIHZhbHVlOiBzdHJpbmcsIHJlZjogbnVtYmVyID0gMCwgaW5kZXhlZFZhcmlhYmxlcz86IG51bWJlciwgbmFtZWRWYXJpYWJsZXM/OiBudW1iZXIpIHtcblx0XHR0aGlzLm5hbWUgPSBuYW1lO1xuXHRcdHRoaXMudmFsdWUgPSB2YWx1ZTtcblx0XHR0aGlzLnZhcmlhYmxlc1JlZmVyZW5jZSA9IHJlZjtcblx0XHRpZiAodHlwZW9mIG5hbWVkVmFyaWFibGVzID09PSAnbnVtYmVyJykge1xuXHRcdFx0KDxEZWJ1Z1Byb3RvY29sLlZhcmlhYmxlPnRoaXMpLm5hbWVkVmFyaWFibGVzID0gbmFtZWRWYXJpYWJsZXM7XG5cdFx0fVxuXHRcdGlmICh0eXBlb2YgaW5kZXhlZFZhcmlhYmxlcyA9PT0gJ251bWJlcicpIHtcblx0XHRcdCg8RGVidWdQcm90b2NvbC5WYXJpYWJsZT50aGlzKS5pbmRleGVkVmFyaWFibGVzID0gaW5kZXhlZFZhcmlhYmxlcztcblx0XHR9XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIEJyZWFrcG9pbnQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLkJyZWFrcG9pbnQge1xuXHR2ZXJpZmllZDogYm9vbGVhbjtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IodmVyaWZpZWQ6IGJvb2xlYW4sIGxpbmU/OiBudW1iZXIsIGNvbHVtbj86IG51bWJlciwgc291cmNlPzogU291cmNlKSB7XG5cdFx0dGhpcy52ZXJpZmllZCA9IHZlcmlmaWVkO1xuXHRcdGNvbnN0IGU6IERlYnVnUHJvdG9jb2wuQnJlYWtwb2ludCA9IHRoaXM7XG5cdFx0aWYgKHR5cGVvZiBsaW5lID09PSAnbnVtYmVyJykge1xuXHRcdFx0ZS5saW5lID0gbGluZTtcblx0XHR9XG5cdFx0aWYgKHR5cGVvZiBjb2x1bW4gPT09ICdudW1iZXInKSB7XG5cdFx0XHRlLmNvbHVtbiA9IGNvbHVtbjtcblx0XHR9XG5cdFx0aWYgKHNvdXJjZSkge1xuXHRcdFx0ZS5zb3VyY2UgPSBzb3VyY2U7XG5cdFx0fVxuXHR9XG5cblx0cHVibGljIHNldElkKGlkOiBudW1iZXIpIHtcblx0XHQodGhpcyBhcyBEZWJ1Z1Byb3RvY29sLkJyZWFrcG9pbnQpLmlkID0gaWQ7XG4gXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBNb2R1bGUgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLk1vZHVsZSB7XG5cdGlkOiBudW1iZXIgfCBzdHJpbmc7XG5cdG5hbWU6IHN0cmluZztcblxuXHRwdWJsaWMgY29uc3RydWN0b3IoaWQ6IG51bWJlciB8IHN0cmluZywgbmFtZTogc3RyaW5nKSB7XG5cdFx0dGhpcy5pZCA9IGlkO1xuXHRcdHRoaXMubmFtZSA9IG5hbWU7XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIENvbXBsZXRpb25JdGVtIGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Db21wbGV0aW9uSXRlbSB7XG5cdGxhYmVsOiBzdHJpbmc7XG5cdHN0YXJ0OiBudW1iZXI7XG5cdGxlbmd0aDogbnVtYmVyO1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihsYWJlbDogc3RyaW5nLCBzdGFydDogbnVtYmVyLCBsZW5ndGg6IG51bWJlciA9IDApIHtcblx0XHR0aGlzLmxhYmVsID0gbGFiZWw7XG5cdFx0dGhpcy5zdGFydCA9IHN0YXJ0O1xuXHRcdHRoaXMubGVuZ3RoID0gbGVuZ3RoO1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBTdG9wcGVkRXZlbnQgZXh0ZW5kcyBFdmVudCBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuU3RvcHBlZEV2ZW50IHtcblx0Ym9keToge1xuXHRcdHJlYXNvbjogc3RyaW5nO1xuXHR9O1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihyZWFzb246IHN0cmluZywgdGhyZWFkSWQ/OiBudW1iZXIsIGV4Y2VwdGlvblRleHQ/OiBzdHJpbmcpIHtcblx0XHRzdXBlcignc3RvcHBlZCcpO1xuXHRcdHRoaXMuYm9keSA9IHtcblx0XHRcdHJlYXNvbjogcmVhc29uXG5cdFx0fTtcblx0XHRpZiAodHlwZW9mIHRocmVhZElkID09PSAnbnVtYmVyJykge1xuXHRcdFx0KHRoaXMgYXMgRGVidWdQcm90b2NvbC5TdG9wcGVkRXZlbnQpLmJvZHkudGhyZWFkSWQgPSB0aHJlYWRJZDtcblx0XHR9XG5cdFx0aWYgKHR5cGVvZiBleGNlcHRpb25UZXh0ID09PSAnc3RyaW5nJykge1xuXHRcdFx0KHRoaXMgYXMgRGVidWdQcm90b2NvbC5TdG9wcGVkRXZlbnQpLmJvZHkudGV4dCA9IGV4Y2VwdGlvblRleHQ7XG5cdFx0fVxuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBDb250aW51ZWRFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Db250aW51ZWRFdmVudCB7XG5cdGJvZHk6IHtcblx0XHR0aHJlYWRJZDogbnVtYmVyO1xuXHR9O1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3Rvcih0aHJlYWRJZDogbnVtYmVyLCBhbGxUaHJlYWRzQ29udGludWVkPzogYm9vbGVhbikge1xuXHRcdHN1cGVyKCdjb250aW51ZWQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHR0aHJlYWRJZDogdGhyZWFkSWRcblx0XHR9O1xuXG5cdFx0aWYgKHR5cGVvZiBhbGxUaHJlYWRzQ29udGludWVkID09PSAnYm9vbGVhbicpIHtcblx0XHRcdCg8RGVidWdQcm90b2NvbC5Db250aW51ZWRFdmVudD50aGlzKS5ib2R5LmFsbFRocmVhZHNDb250aW51ZWQgPSBhbGxUaHJlYWRzQ29udGludWVkO1xuXHRcdH1cblx0fVxufVxuXG5leHBvcnQgY2xhc3MgSW5pdGlhbGl6ZWRFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Jbml0aWFsaXplZEV2ZW50IHtcblx0cHVibGljIGNvbnN0cnVjdG9yKCkge1xuXHRcdHN1cGVyKCdpbml0aWFsaXplZCcpO1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBUZXJtaW5hdGVkRXZlbnQgZXh0ZW5kcyBFdmVudCBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuVGVybWluYXRlZEV2ZW50IHtcblx0cHVibGljIGNvbnN0cnVjdG9yKHJlc3RhcnQ/OiBhbnkpIHtcblx0XHRzdXBlcigndGVybWluYXRlZCcpO1xuXHRcdGlmICh0eXBlb2YgcmVzdGFydCA9PT0gJ2Jvb2xlYW4nIHx8IHJlc3RhcnQpIHtcblx0XHRcdGNvbnN0IGU6IERlYnVnUHJvdG9jb2wuVGVybWluYXRlZEV2ZW50ID0gdGhpcztcblx0XHRcdGUuYm9keSA9IHtcblx0XHRcdFx0cmVzdGFydDogcmVzdGFydFxuXHRcdFx0fTtcblx0XHR9XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIEV4aXRlZEV2ZW50IGV4dGVuZHMgRXZlbnQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLkV4aXRlZEV2ZW50IHtcblx0Ym9keToge1xuXHRcdGV4aXRDb2RlOiBudW1iZXJcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IoZXhpdENvZGU6IG51bWJlcikge1xuXHRcdHN1cGVyKCdleGl0ZWQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRleGl0Q29kZTogZXhpdENvZGVcblx0XHR9O1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBPdXRwdXRFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5PdXRwdXRFdmVudCB7XG5cdGJvZHk6IHtcblx0XHRjYXRlZ29yeTogc3RyaW5nLFxuXHRcdG91dHB1dDogc3RyaW5nLFxuXHRcdGRhdGE/OiBhbnlcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3Iob3V0cHV0OiBzdHJpbmcsIGNhdGVnb3J5OiBzdHJpbmcgPSAnY29uc29sZScsIGRhdGE/OiBhbnkpIHtcblx0XHRzdXBlcignb3V0cHV0Jyk7XG5cdFx0dGhpcy5ib2R5ID0ge1xuXHRcdFx0Y2F0ZWdvcnk6IGNhdGVnb3J5LFxuXHRcdFx0b3V0cHV0OiBvdXRwdXRcblx0XHR9O1xuXHRcdGlmIChkYXRhICE9PSB1bmRlZmluZWQpIHtcblx0XHRcdHRoaXMuYm9keS5kYXRhID0gZGF0YTtcblx0XHR9XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIFRocmVhZEV2ZW50IGV4dGVuZHMgRXZlbnQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLlRocmVhZEV2ZW50IHtcblx0Ym9keToge1xuXHRcdHJlYXNvbjogc3RyaW5nLFxuXHRcdHRocmVhZElkOiBudW1iZXJcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IocmVhc29uOiBzdHJpbmcsIHRocmVhZElkOiBudW1iZXIpIHtcblx0XHRzdXBlcigndGhyZWFkJyk7XG5cdFx0dGhpcy5ib2R5ID0ge1xuXHRcdFx0cmVhc29uOiByZWFzb24sXG5cdFx0XHR0aHJlYWRJZDogdGhyZWFkSWRcblx0XHR9O1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBCcmVha3BvaW50RXZlbnQgZXh0ZW5kcyBFdmVudCBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuQnJlYWtwb2ludEV2ZW50IHtcblx0Ym9keToge1xuXHRcdHJlYXNvbjogc3RyaW5nLFxuXHRcdGJyZWFrcG9pbnQ6IERlYnVnUHJvdG9jb2wuQnJlYWtwb2ludFxuXHR9O1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihyZWFzb246IHN0cmluZywgYnJlYWtwb2ludDogRGVidWdQcm90b2NvbC5CcmVha3BvaW50KSB7XG5cdFx0c3VwZXIoJ2JyZWFrcG9pbnQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRyZWFzb246IHJlYXNvbixcblx0XHRcdGJyZWFrcG9pbnQ6IGJyZWFrcG9pbnRcblx0XHR9O1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBNb2R1bGVFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Nb2R1bGVFdmVudCB7XG5cdGJvZHk6IHtcblx0XHRyZWFzb246ICduZXcnIHwgJ2NoYW5nZWQnIHwgJ3JlbW92ZWQnLFxuXHRcdG1vZHVsZTogRGVidWdQcm90b2NvbC5Nb2R1bGVcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IocmVhc29uOiAnbmV3JyB8ICdjaGFuZ2VkJyB8ICdyZW1vdmVkJywgbW9kdWxlOiBEZWJ1Z1Byb3RvY29sLk1vZHVsZSkge1xuXHRcdHN1cGVyKCdtb2R1bGUnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRyZWFzb246IHJlYXNvbixcblx0XHRcdG1vZHVsZTogbW9kdWxlXG5cdFx0fTtcblx0fVxufVxuXG5leHBvcnQgY2xhc3MgTG9hZGVkU291cmNlRXZlbnQgZXh0ZW5kcyBFdmVudCBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuTG9hZGVkU291cmNlRXZlbnQge1xuXHRib2R5OiB7XG5cdFx0cmVhc29uOiAnbmV3JyB8ICdjaGFuZ2VkJyB8ICdyZW1vdmVkJyxcblx0XHRzb3VyY2U6IERlYnVnUHJvdG9jb2wuU291cmNlXG5cdH07XG5cblx0cHVibGljIGNvbnN0cnVjdG9yKHJlYXNvbjogJ25ldycgfCAnY2hhbmdlZCcgfCAncmVtb3ZlZCcsIHNvdXJjZTogRGVidWdQcm90b2NvbC5Tb3VyY2UpIHtcblx0XHRzdXBlcignbG9hZGVkU291cmNlJyk7XG5cdFx0dGhpcy5ib2R5ID0ge1xuXHRcdFx0cmVhc29uOiByZWFzb24sXG5cdFx0XHRzb3VyY2U6IHNvdXJjZVxuXHRcdH07XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIENhcGFiaWxpdGllc0V2ZW50IGV4dGVuZHMgRXZlbnQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLkNhcGFiaWxpdGllc0V2ZW50IHtcblx0Ym9keToge1xuXHRcdGNhcGFiaWxpdGllczogRGVidWdQcm90b2NvbC5DYXBhYmlsaXRpZXNcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IoY2FwYWJpbGl0aWVzOiBEZWJ1Z1Byb3RvY29sLkNhcGFiaWxpdGllcykge1xuXHRcdHN1cGVyKCdjYXBhYmlsaXRpZXMnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRjYXBhYmlsaXRpZXM6IGNhcGFiaWxpdGllc1xuXHRcdH07XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIFByb2dyZXNzU3RhcnRFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Qcm9ncmVzc1N0YXJ0RXZlbnQge1xuXHRib2R5OiB7XG5cdFx0cHJvZ3Jlc3NJZDogc3RyaW5nLFxuXHRcdHRpdGxlOiBzdHJpbmdcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IocHJvZ3Jlc3NJZDogc3RyaW5nLCB0aXRsZTogc3RyaW5nLCBtZXNzYWdlPzogc3RyaW5nKSB7XG5cdFx0c3VwZXIoJ3Byb2dyZXNzU3RhcnQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRwcm9ncmVzc0lkOiBwcm9ncmVzc0lkLFxuXHRcdFx0dGl0bGU6IHRpdGxlXG5cdFx0fTtcblx0XHRpZiAodHlwZW9mIG1lc3NhZ2UgPT09ICdzdHJpbmcnKSB7XG5cdFx0XHQodGhpcyBhcyBEZWJ1Z1Byb3RvY29sLlByb2dyZXNzU3RhcnRFdmVudCkuYm9keS5tZXNzYWdlID0gbWVzc2FnZTtcblx0XHR9XG5cdH1cbn1cblxuZXhwb3J0IGNsYXNzIFByb2dyZXNzVXBkYXRlRXZlbnQgZXh0ZW5kcyBFdmVudCBpbXBsZW1lbnRzIERlYnVnUHJvdG9jb2wuUHJvZ3Jlc3NVcGRhdGVFdmVudCB7XG5cdGJvZHk6IHtcblx0XHRwcm9ncmVzc0lkOiBzdHJpbmdcblx0fTtcblxuXHRwdWJsaWMgY29uc3RydWN0b3IocHJvZ3Jlc3NJZDogc3RyaW5nLCBtZXNzYWdlPzogc3RyaW5nKSB7XG5cdFx0c3VwZXIoJ3Byb2dyZXNzVXBkYXRlJyk7XG5cdFx0dGhpcy5ib2R5ID0ge1xuXHRcdFx0cHJvZ3Jlc3NJZDogcHJvZ3Jlc3NJZFxuXHRcdH07XG5cdFx0aWYgKHR5cGVvZiBtZXNzYWdlID09PSAnc3RyaW5nJykge1xuXHRcdFx0KHRoaXMgYXMgRGVidWdQcm90b2NvbC5Qcm9ncmVzc1VwZGF0ZUV2ZW50KS5ib2R5Lm1lc3NhZ2UgPSBtZXNzYWdlO1xuXHRcdH1cblx0fVxufVxuXG5leHBvcnQgY2xhc3MgUHJvZ3Jlc3NFbmRFdmVudCBleHRlbmRzIEV2ZW50IGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5Qcm9ncmVzc0VuZEV2ZW50IHtcblx0Ym9keToge1xuXHRcdHByb2dyZXNzSWQ6IHN0cmluZ1xuXHR9O1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3Rvcihwcm9ncmVzc0lkOiBzdHJpbmcsIG1lc3NhZ2U/OiBzdHJpbmcpIHtcblx0XHRzdXBlcigncHJvZ3Jlc3NFbmQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0XHRwcm9ncmVzc0lkOiBwcm9ncmVzc0lkXG5cdFx0fTtcblx0XHRpZiAodHlwZW9mIG1lc3NhZ2UgPT09ICdzdHJpbmcnKSB7XG5cdFx0XHQodGhpcyBhcyBEZWJ1Z1Byb3RvY29sLlByb2dyZXNzRW5kRXZlbnQpLmJvZHkubWVzc2FnZSA9IG1lc3NhZ2U7XG5cdFx0fVxuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBJbnZhbGlkYXRlZEV2ZW50IGV4dGVuZHMgRXZlbnQgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLkludmFsaWRhdGVkRXZlbnQge1xuXHRib2R5OiB7XG5cdFx0YXJlYXM/OiBEZWJ1Z1Byb3RvY29sLkludmFsaWRhdGVkQXJlYXNbXTtcblx0XHR0aHJlYWRJZD86IG51bWJlcjtcblx0XHRzdGFja0ZyYW1lSWQ/OiBudW1iZXI7XG5cdH07XG5cblx0cHVibGljIGNvbnN0cnVjdG9yKGFyZWFzPzogRGVidWdQcm90b2NvbC5JbnZhbGlkYXRlZEFyZWFzW10sIHRocmVhZElkPzogbnVtYmVyLCBzdGFja0ZyYW1lSWQ/OiBudW1iZXIpIHtcblx0XHRzdXBlcignaW52YWxpZGF0ZWQnKTtcblx0XHR0aGlzLmJvZHkgPSB7XG5cdFx0fTtcblx0XHRpZiAoYXJlYXMpIHtcblx0XHRcdHRoaXMuYm9keS5hcmVhcyA9IGFyZWFzO1xuXHRcdH1cblx0XHRpZiAodGhyZWFkSWQpIHtcblx0XHRcdHRoaXMuYm9keS50aHJlYWRJZCA9IHRocmVhZElkO1xuXHRcdH1cblx0XHRpZiAoc3RhY2tGcmFtZUlkKSB7XG5cdFx0XHR0aGlzLmJvZHkuc3RhY2tGcmFtZUlkID0gc3RhY2tGcmFtZUlkO1xuXHRcdH1cblx0fVxufVxuXG5leHBvcnQgZW51bSBFcnJvckRlc3RpbmF0aW9uIHtcblx0VXNlciA9IDEsXG5cdFRlbGVtZXRyeSA9IDJcbn07XG5cbmV4cG9ydCBjbGFzcyBEZWJ1Z1Nlc3Npb24gZXh0ZW5kcyBQcm90b2NvbFNlcnZlciB7XG5cblx0cHJpdmF0ZSBfZGVidWdnZXJMaW5lc1N0YXJ0QXQxOiBib29sZWFuO1xuXHRwcml2YXRlIF9kZWJ1Z2dlckNvbHVtbnNTdGFydEF0MTogYm9vbGVhbjtcblx0cHJpdmF0ZSBfZGVidWdnZXJQYXRoc0FyZVVSSXM6IGJvb2xlYW47XG5cblx0cHJpdmF0ZSBfY2xpZW50TGluZXNTdGFydEF0MTogYm9vbGVhbjtcblx0cHJpdmF0ZSBfY2xpZW50Q29sdW1uc1N0YXJ0QXQxOiBib29sZWFuO1xuXHRwcml2YXRlIF9jbGllbnRQYXRoc0FyZVVSSXM6IGJvb2xlYW47XG5cblx0cHJvdGVjdGVkIF9pc1NlcnZlcjogYm9vbGVhbjtcblxuXHRwdWJsaWMgY29uc3RydWN0b3Iob2Jzb2xldGVfZGVidWdnZXJMaW5lc0FuZENvbHVtbnNTdGFydEF0MT86IGJvb2xlYW4sIG9ic29sZXRlX2lzU2VydmVyPzogYm9vbGVhbikge1xuXHRcdHN1cGVyKCk7XG5cblx0XHRjb25zdCBsaW5lc0FuZENvbHVtbnNTdGFydEF0MSA9IHR5cGVvZiBvYnNvbGV0ZV9kZWJ1Z2dlckxpbmVzQW5kQ29sdW1uc1N0YXJ0QXQxID09PSAnYm9vbGVhbicgPyBvYnNvbGV0ZV9kZWJ1Z2dlckxpbmVzQW5kQ29sdW1uc1N0YXJ0QXQxIDogZmFsc2U7XG5cdFx0dGhpcy5fZGVidWdnZXJMaW5lc1N0YXJ0QXQxID0gbGluZXNBbmRDb2x1bW5zU3RhcnRBdDE7XG5cdFx0dGhpcy5fZGVidWdnZXJDb2x1bW5zU3RhcnRBdDEgPSBsaW5lc0FuZENvbHVtbnNTdGFydEF0MTtcblx0XHR0aGlzLl9kZWJ1Z2dlclBhdGhzQXJlVVJJcyA9IGZhbHNlO1xuXG5cdFx0dGhpcy5fY2xpZW50TGluZXNTdGFydEF0MSA9IHRydWU7XG5cdFx0dGhpcy5fY2xpZW50Q29sdW1uc1N0YXJ0QXQxID0gdHJ1ZTtcblx0XHR0aGlzLl9jbGllbnRQYXRoc0FyZVVSSXMgPSBmYWxzZTtcblxuXHRcdHRoaXMuX2lzU2VydmVyID0gdHlwZW9mIG9ic29sZXRlX2lzU2VydmVyID09PSAnYm9vbGVhbicgPyBvYnNvbGV0ZV9pc1NlcnZlciA6IGZhbHNlO1xuXG5cdFx0dGhpcy5vbignY2xvc2UnLCAoKSA9PiB7XG5cdFx0XHR0aGlzLnNodXRkb3duKCk7XG5cdFx0fSk7XG5cdFx0dGhpcy5vbignZXJyb3InLCAoZXJyb3IpID0+IHtcblx0XHRcdHRoaXMuc2h1dGRvd24oKTtcblx0XHR9KTtcblx0fVxuXG5cdHB1YmxpYyBzZXREZWJ1Z2dlclBhdGhGb3JtYXQoZm9ybWF0OiBzdHJpbmcpIHtcblx0XHR0aGlzLl9kZWJ1Z2dlclBhdGhzQXJlVVJJcyA9IGZvcm1hdCAhPT0gJ3BhdGgnO1xuXHR9XG5cblx0cHVibGljIHNldERlYnVnZ2VyTGluZXNTdGFydEF0MShlbmFibGU6IGJvb2xlYW4pIHtcblx0XHR0aGlzLl9kZWJ1Z2dlckxpbmVzU3RhcnRBdDEgPSBlbmFibGU7XG5cdH1cblxuXHRwdWJsaWMgc2V0RGVidWdnZXJDb2x1bW5zU3RhcnRBdDEoZW5hYmxlOiBib29sZWFuKSB7XG5cdFx0dGhpcy5fZGVidWdnZXJDb2x1bW5zU3RhcnRBdDEgPSBlbmFibGU7XG5cdH1cblxuXHRwdWJsaWMgc2V0UnVuQXNTZXJ2ZXIoZW5hYmxlOiBib29sZWFuKSB7XG5cdFx0dGhpcy5faXNTZXJ2ZXIgPSBlbmFibGU7XG5cdH1cblxuXHQvKipcblx0ICogQSB2aXJ0dWFsIGNvbnN0cnVjdG9yLi4uXG5cdCAqL1xuXHRwdWJsaWMgc3RhdGljIHJ1bihkZWJ1Z1Nlc3Npb246IHR5cGVvZiBEZWJ1Z1Nlc3Npb24pIHtcblx0XHRydW5EZWJ1Z0FkYXB0ZXIoZGVidWdTZXNzaW9uKTtcblx0fVxuXG5cdHB1YmxpYyBzaHV0ZG93bigpOiB2b2lkIHtcblx0XHRpZiAodGhpcy5faXNTZXJ2ZXIgfHwgdGhpcy5faXNSdW5uaW5nSW5saW5lKCkpIHtcblx0XHRcdC8vIHNodXRkb3duIGlnbm9yZWQgaW4gc2VydmVyIG1vZGVcblx0XHR9IGVsc2Uge1xuXHRcdFx0Ly8gd2FpdCBhIGJpdCBiZWZvcmUgc2h1dHRpbmcgZG93blxuXHRcdFx0c2V0VGltZW91dCgoKSA9PiB7XG5cdFx0XHRcdHByb2Nlc3MuZXhpdCgwKTtcblx0XHRcdH0sIDEwMCk7XG5cdFx0fVxuXHR9XG5cblx0cHJvdGVjdGVkIHNlbmRFcnJvclJlc3BvbnNlKHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlJlc3BvbnNlLCBjb2RlT3JNZXNzYWdlOiBudW1iZXIgfCBEZWJ1Z1Byb3RvY29sLk1lc3NhZ2UsIGZvcm1hdD86IHN0cmluZywgdmFyaWFibGVzPzogYW55LCBkZXN0OiBFcnJvckRlc3RpbmF0aW9uID0gRXJyb3JEZXN0aW5hdGlvbi5Vc2VyKTogdm9pZCB7XG5cblx0XHRsZXQgbXNnIDogRGVidWdQcm90b2NvbC5NZXNzYWdlO1xuXHRcdGlmICh0eXBlb2YgY29kZU9yTWVzc2FnZSA9PT0gJ251bWJlcicpIHtcblx0XHRcdG1zZyA9IDxEZWJ1Z1Byb3RvY29sLk1lc3NhZ2U+IHtcblx0XHRcdFx0aWQ6IDxudW1iZXI+IGNvZGVPck1lc3NhZ2UsXG5cdFx0XHRcdGZvcm1hdDogZm9ybWF0XG5cdFx0XHR9O1xuXHRcdFx0aWYgKHZhcmlhYmxlcykge1xuXHRcdFx0XHRtc2cudmFyaWFibGVzID0gdmFyaWFibGVzO1xuXHRcdFx0fVxuXHRcdFx0aWYgKGRlc3QgJiBFcnJvckRlc3RpbmF0aW9uLlVzZXIpIHtcblx0XHRcdFx0bXNnLnNob3dVc2VyID0gdHJ1ZTtcblx0XHRcdH1cblx0XHRcdGlmIChkZXN0ICYgRXJyb3JEZXN0aW5hdGlvbi5UZWxlbWV0cnkpIHtcblx0XHRcdFx0bXNnLnNlbmRUZWxlbWV0cnkgPSB0cnVlO1xuXHRcdFx0fVxuXHRcdH0gZWxzZSB7XG5cdFx0XHRtc2cgPSBjb2RlT3JNZXNzYWdlO1xuXHRcdH1cblxuXHRcdHJlc3BvbnNlLnN1Y2Nlc3MgPSBmYWxzZTtcblx0XHRyZXNwb25zZS5tZXNzYWdlID0gRGVidWdTZXNzaW9uLmZvcm1hdFBJSShtc2cuZm9ybWF0LCB0cnVlLCBtc2cudmFyaWFibGVzKTtcblx0XHRpZiAoIXJlc3BvbnNlLmJvZHkpIHtcblx0XHRcdHJlc3BvbnNlLmJvZHkgPSB7IH07XG5cdFx0fVxuXHRcdHJlc3BvbnNlLmJvZHkuZXJyb3IgPSBtc2c7XG5cblx0XHR0aGlzLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwdWJsaWMgcnVuSW5UZXJtaW5hbFJlcXVlc3QoYXJnczogRGVidWdQcm90b2NvbC5SdW5JblRlcm1pbmFsUmVxdWVzdEFyZ3VtZW50cywgdGltZW91dDogbnVtYmVyLCBjYjogKHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlJ1bkluVGVybWluYWxSZXNwb25zZSkgPT4gdm9pZCkge1xuXHRcdHRoaXMuc2VuZFJlcXVlc3QoJ3J1bkluVGVybWluYWwnLCBhcmdzLCB0aW1lb3V0LCBjYik7XG5cdH1cblxuXHRwcm90ZWN0ZWQgZGlzcGF0Y2hSZXF1ZXN0KHJlcXVlc3Q6IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXG5cdFx0Y29uc3QgcmVzcG9uc2UgPSBuZXcgUmVzcG9uc2UocmVxdWVzdCk7XG5cblx0XHR0cnkge1xuXHRcdFx0aWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2luaXRpYWxpemUnKSB7XG5cdFx0XHRcdHZhciBhcmdzID0gPERlYnVnUHJvdG9jb2wuSW5pdGlhbGl6ZVJlcXVlc3RBcmd1bWVudHM+IHJlcXVlc3QuYXJndW1lbnRzO1xuXG5cdFx0XHRcdGlmICh0eXBlb2YgYXJncy5saW5lc1N0YXJ0QXQxID09PSAnYm9vbGVhbicpIHtcblx0XHRcdFx0XHR0aGlzLl9jbGllbnRMaW5lc1N0YXJ0QXQxID0gYXJncy5saW5lc1N0YXJ0QXQxO1xuXHRcdFx0XHR9XG5cdFx0XHRcdGlmICh0eXBlb2YgYXJncy5jb2x1bW5zU3RhcnRBdDEgPT09ICdib29sZWFuJykge1xuXHRcdFx0XHRcdHRoaXMuX2NsaWVudENvbHVtbnNTdGFydEF0MSA9IGFyZ3MuY29sdW1uc1N0YXJ0QXQxO1xuXHRcdFx0XHR9XG5cblx0XHRcdFx0aWYgKGFyZ3MucGF0aEZvcm1hdCAhPT0gJ3BhdGgnKSB7XG5cdFx0XHRcdFx0dGhpcy5zZW5kRXJyb3JSZXNwb25zZShyZXNwb25zZSwgMjAxOCwgJ2RlYnVnIGFkYXB0ZXIgb25seSBzdXBwb3J0cyBuYXRpdmUgcGF0aHMnLCBudWxsLCBFcnJvckRlc3RpbmF0aW9uLlRlbGVtZXRyeSk7XG5cdFx0XHRcdH0gZWxzZSB7XG5cdFx0XHRcdFx0Y29uc3QgaW5pdGlhbGl6ZVJlc3BvbnNlID0gPERlYnVnUHJvdG9jb2wuSW5pdGlhbGl6ZVJlc3BvbnNlPiByZXNwb25zZTtcblx0XHRcdFx0XHRpbml0aWFsaXplUmVzcG9uc2UuYm9keSA9IHt9O1xuXHRcdFx0XHRcdHRoaXMuaW5pdGlhbGl6ZVJlcXVlc3QoaW5pdGlhbGl6ZVJlc3BvbnNlLCBhcmdzKTtcblx0XHRcdFx0fVxuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2xhdW5jaCcpIHtcblx0XHRcdFx0dGhpcy5sYXVuY2hSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkxhdW5jaFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2F0dGFjaCcpIHtcblx0XHRcdFx0dGhpcy5hdHRhY2hSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkF0dGFjaFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2Rpc2Nvbm5lY3QnKSB7XG5cdFx0XHRcdHRoaXMuZGlzY29ubmVjdFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuRGlzY29ubmVjdFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3Rlcm1pbmF0ZScpIHtcblx0XHRcdFx0dGhpcy50ZXJtaW5hdGVSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlRlcm1pbmF0ZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3Jlc3RhcnQnKSB7XG5cdFx0XHRcdHRoaXMucmVzdGFydFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuUmVzdGFydFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NldEJyZWFrcG9pbnRzJykge1xuXHRcdFx0XHR0aGlzLnNldEJyZWFrUG9pbnRzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TZXRCcmVha3BvaW50c1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NldEZ1bmN0aW9uQnJlYWtwb2ludHMnKSB7XG5cdFx0XHRcdHRoaXMuc2V0RnVuY3Rpb25CcmVha1BvaW50c1JlcXVlc3QoPERlYnVnUHJvdG9jb2wuU2V0RnVuY3Rpb25CcmVha3BvaW50c1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NldEV4Y2VwdGlvbkJyZWFrcG9pbnRzJykge1xuXHRcdFx0XHR0aGlzLnNldEV4Y2VwdGlvbkJyZWFrUG9pbnRzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TZXRFeGNlcHRpb25CcmVha3BvaW50c1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2NvbmZpZ3VyYXRpb25Eb25lJykge1xuXHRcdFx0XHR0aGlzLmNvbmZpZ3VyYXRpb25Eb25lUmVxdWVzdCg8RGVidWdQcm90b2NvbC5Db25maWd1cmF0aW9uRG9uZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2NvbnRpbnVlJykge1xuXHRcdFx0XHR0aGlzLmNvbnRpbnVlUmVxdWVzdCg8RGVidWdQcm90b2NvbC5Db250aW51ZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ25leHQnKSB7XG5cdFx0XHRcdHRoaXMubmV4dFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuTmV4dFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3N0ZXBJbicpIHtcblx0XHRcdFx0dGhpcy5zdGVwSW5SZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlN0ZXBJblJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3N0ZXBPdXQnKSB7XG5cdFx0XHRcdHRoaXMuc3RlcE91dFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuU3RlcE91dFJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3N0ZXBCYWNrJykge1xuXHRcdFx0XHR0aGlzLnN0ZXBCYWNrUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TdGVwQmFja1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3JldmVyc2VDb250aW51ZScpIHtcblx0XHRcdFx0dGhpcy5yZXZlcnNlQ29udGludWVSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlJldmVyc2VDb250aW51ZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3Jlc3RhcnRGcmFtZScpIHtcblx0XHRcdFx0dGhpcy5yZXN0YXJ0RnJhbWVSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlJlc3RhcnRGcmFtZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ2dvdG8nKSB7XG5cdFx0XHRcdHRoaXMuZ290b1JlcXVlc3QoPERlYnVnUHJvdG9jb2wuR290b1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3BhdXNlJykge1xuXHRcdFx0XHR0aGlzLnBhdXNlUmVxdWVzdCg8RGVidWdQcm90b2NvbC5QYXVzZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3N0YWNrVHJhY2UnKSB7XG5cdFx0XHRcdHRoaXMuc3RhY2tUcmFjZVJlcXVlc3QoPERlYnVnUHJvdG9jb2wuU3RhY2tUcmFjZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3Njb3BlcycpIHtcblx0XHRcdFx0dGhpcy5zY29wZXNSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlNjb3Blc1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3ZhcmlhYmxlcycpIHtcblx0XHRcdFx0dGhpcy52YXJpYWJsZXNSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlZhcmlhYmxlc1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NldFZhcmlhYmxlJykge1xuXHRcdFx0XHR0aGlzLnNldFZhcmlhYmxlUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TZXRWYXJpYWJsZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NldEV4cHJlc3Npb24nKSB7XG5cdFx0XHRcdHRoaXMuc2V0RXhwcmVzc2lvblJlcXVlc3QoPERlYnVnUHJvdG9jb2wuU2V0RXhwcmVzc2lvblJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3NvdXJjZScpIHtcblx0XHRcdFx0dGhpcy5zb3VyY2VSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLlNvdXJjZVJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXG5cdFx0XHR9IGVsc2UgaWYgKHJlcXVlc3QuY29tbWFuZCA9PT0gJ3RocmVhZHMnKSB7XG5cdFx0XHRcdHRoaXMudGhyZWFkc1JlcXVlc3QoPERlYnVnUHJvdG9jb2wuVGhyZWFkc1Jlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAndGVybWluYXRlVGhyZWFkcycpIHtcblx0XHRcdFx0dGhpcy50ZXJtaW5hdGVUaHJlYWRzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5UZXJtaW5hdGVUaHJlYWRzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnZXZhbHVhdGUnKSB7XG5cdFx0XHRcdHRoaXMuZXZhbHVhdGVSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkV2YWx1YXRlUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnc3RlcEluVGFyZ2V0cycpIHtcblx0XHRcdFx0dGhpcy5zdGVwSW5UYXJnZXRzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TdGVwSW5UYXJnZXRzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnZ290b1RhcmdldHMnKSB7XG5cdFx0XHRcdHRoaXMuZ290b1RhcmdldHNSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkdvdG9UYXJnZXRzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnY29tcGxldGlvbnMnKSB7XG5cdFx0XHRcdHRoaXMuY29tcGxldGlvbnNSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkNvbXBsZXRpb25zUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnZXhjZXB0aW9uSW5mbycpIHtcblx0XHRcdFx0dGhpcy5leGNlcHRpb25JbmZvUmVxdWVzdCg8RGVidWdQcm90b2NvbC5FeGNlcHRpb25JbmZvUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnbG9hZGVkU291cmNlcycpIHtcblx0XHRcdFx0dGhpcy5sb2FkZWRTb3VyY2VzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5Mb2FkZWRTb3VyY2VzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnZGF0YUJyZWFrcG9pbnRJbmZvJykge1xuXHRcdFx0XHR0aGlzLmRhdGFCcmVha3BvaW50SW5mb1JlcXVlc3QoPERlYnVnUHJvdG9jb2wuRGF0YUJyZWFrcG9pbnRJbmZvUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnc2V0RGF0YUJyZWFrcG9pbnRzJykge1xuXHRcdFx0XHR0aGlzLnNldERhdGFCcmVha3BvaW50c1JlcXVlc3QoPERlYnVnUHJvdG9jb2wuU2V0RGF0YUJyZWFrcG9pbnRzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAncmVhZE1lbW9yeScpIHtcblx0XHRcdFx0dGhpcy5yZWFkTWVtb3J5UmVxdWVzdCg8RGVidWdQcm90b2NvbC5SZWFkTWVtb3J5UmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnd3JpdGVNZW1vcnknKSB7XG5cdFx0XHRcdHRoaXMud3JpdGVNZW1vcnlSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLldyaXRlTWVtb3J5UmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnZGlzYXNzZW1ibGUnKSB7XG5cdFx0XHRcdHRoaXMuZGlzYXNzZW1ibGVSZXF1ZXN0KDxEZWJ1Z1Byb3RvY29sLkRpc2Fzc2VtYmxlUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnY2FuY2VsJykge1xuXHRcdFx0XHR0aGlzLmNhbmNlbFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuQ2FuY2VsUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnYnJlYWtwb2ludExvY2F0aW9ucycpIHtcblx0XHRcdFx0dGhpcy5icmVha3BvaW50TG9jYXRpb25zUmVxdWVzdCg8RGVidWdQcm90b2NvbC5CcmVha3BvaW50TG9jYXRpb25zUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSBpZiAocmVxdWVzdC5jb21tYW5kID09PSAnc2V0SW5zdHJ1Y3Rpb25CcmVha3BvaW50cycpIHtcblx0XHRcdFx0dGhpcy5zZXRJbnN0cnVjdGlvbkJyZWFrcG9pbnRzUmVxdWVzdCg8RGVidWdQcm90b2NvbC5TZXRJbnN0cnVjdGlvbkJyZWFrcG9pbnRzUmVzcG9uc2U+IHJlc3BvbnNlLCByZXF1ZXN0LmFyZ3VtZW50cywgcmVxdWVzdCk7XG5cblx0XHRcdH0gZWxzZSB7XG5cdFx0XHRcdHRoaXMuY3VzdG9tUmVxdWVzdChyZXF1ZXN0LmNvbW1hbmQsIDxEZWJ1Z1Byb3RvY29sLlJlc3BvbnNlPiByZXNwb25zZSwgcmVxdWVzdC5hcmd1bWVudHMsIHJlcXVlc3QpO1xuXHRcdFx0fVxuXHRcdH0gY2F0Y2ggKGUpIHtcblx0XHRcdHRoaXMuc2VuZEVycm9yUmVzcG9uc2UocmVzcG9uc2UsIDExMDQsICd7X3N0YWNrfScsIHsgX2V4Y2VwdGlvbjogZS5tZXNzYWdlLCBfc3RhY2s6IGUuc3RhY2sgfSwgRXJyb3JEZXN0aW5hdGlvbi5UZWxlbWV0cnkpO1xuXHRcdH1cblx0fVxuXG5cdHByb3RlY3RlZCBpbml0aWFsaXplUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5Jbml0aWFsaXplUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuSW5pdGlhbGl6ZVJlcXVlc3RBcmd1bWVudHMpOiB2b2lkIHtcblxuXHRcdC8vIFRoaXMgZGVmYXVsdCBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgY29uZGl0aW9uYWwgYnJlYWtwb2ludHMuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0NvbmRpdGlvbmFsQnJlYWtwb2ludHMgPSBmYWxzZTtcblxuXHRcdC8vIFRoaXMgZGVmYXVsdCBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgaGl0IGNvbmRpdGlvbmFsIGJyZWFrcG9pbnRzLlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNIaXRDb25kaXRpb25hbEJyZWFrcG9pbnRzID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IGZ1bmN0aW9uIGJyZWFrcG9pbnRzLlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNGdW5jdGlvbkJyZWFrcG9pbnRzID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBpbXBsZW1lbnRzIHRoZSAnY29uZmlndXJhdGlvbkRvbmUnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0NvbmZpZ3VyYXRpb25Eb25lUmVxdWVzdCA9IHRydWU7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IGhvdmVycyBiYXNlZCBvbiB0aGUgJ2V2YWx1YXRlJyByZXF1ZXN0LlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNFdmFsdWF0ZUZvckhvdmVycyA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWZhdWx0IGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3N0ZXBCYWNrJyByZXF1ZXN0LlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNTdGVwQmFjayA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWZhdWx0IGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3NldFZhcmlhYmxlJyByZXF1ZXN0LlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNTZXRWYXJpYWJsZSA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWZhdWx0IGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3Jlc3RhcnRGcmFtZScgcmVxdWVzdC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzUmVzdGFydEZyYW1lID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAnc3RlcEluVGFyZ2V0cycgcmVxdWVzdC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzU3RlcEluVGFyZ2V0c1JlcXVlc3QgPSBmYWxzZTtcblxuXHRcdC8vIFRoaXMgZGVmYXVsdCBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgdGhlICdnb3RvVGFyZ2V0cycgcmVxdWVzdC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzR290b1RhcmdldHNSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAnY29tcGxldGlvbnMnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0NvbXBsZXRpb25zUmVxdWVzdCA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWZhdWx0IGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3Jlc3RhcnQnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c1Jlc3RhcnRSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlZmF1bHQgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAnZXhjZXB0aW9uT3B0aW9ucycgYXR0cmlidXRlIG9uIHRoZSAnc2V0RXhjZXB0aW9uQnJlYWtwb2ludHMnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0V4Y2VwdGlvbk9wdGlvbnMgPSBmYWxzZTtcblxuXHRcdC8vIFRoaXMgZGVmYXVsdCBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgdGhlICdmb3JtYXQnIGF0dHJpYnV0ZSBvbiB0aGUgJ3ZhcmlhYmxlcycsICdldmFsdWF0ZScsIGFuZCAnc3RhY2tUcmFjZScgcmVxdWVzdC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzVmFsdWVGb3JtYXR0aW5nT3B0aW9ucyA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgdGhlICdleGNlcHRpb25JbmZvJyByZXF1ZXN0LlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNFeGNlcHRpb25JbmZvUmVxdWVzdCA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgdGhlICdUZXJtaW5hdGVEZWJ1Z2dlZScgYXR0cmlidXRlIG9uIHRoZSAnZGlzY29ubmVjdCcgcmVxdWVzdC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRUZXJtaW5hdGVEZWJ1Z2dlZSA9IGZhbHNlO1xuXG5cdFx0Ly8gVGhpcyBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgZGVsYXllZCBsb2FkaW5nIG9mIHN0YWNrIGZyYW1lcy5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzRGVsYXllZFN0YWNrVHJhY2VMb2FkaW5nID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ2xvYWRlZFNvdXJjZXMnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0xvYWRlZFNvdXJjZXNSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ2xvZ01lc3NhZ2UnIGF0dHJpYnV0ZSBvZiB0aGUgU291cmNlQnJlYWtwb2ludC5cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzTG9nUG9pbnRzID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3Rlcm1pbmF0ZVRocmVhZHMnIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c1Rlcm1pbmF0ZVRocmVhZHNSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3NldEV4cHJlc3Npb24nIHJlcXVlc3QuXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c1NldEV4cHJlc3Npb24gPSBmYWxzZTtcblxuXHRcdC8vIFRoaXMgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAndGVybWluYXRlJyByZXF1ZXN0LlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNUZXJtaW5hdGVSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvLyBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCBkYXRhIGJyZWFrcG9pbnRzLlxuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNEYXRhQnJlYWtwb2ludHMgPSBmYWxzZTtcblxuXHRcdC8qKiBUaGlzIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ3JlYWRNZW1vcnknIHJlcXVlc3QuICovXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c1JlYWRNZW1vcnlSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvKiogVGhlIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ2Rpc2Fzc2VtYmxlJyByZXF1ZXN0LiAqL1xuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNEaXNhc3NlbWJsZVJlcXVlc3QgPSBmYWxzZTtcblxuXHRcdC8qKiBUaGUgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAnY2FuY2VsJyByZXF1ZXN0LiAqL1xuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNDYW5jZWxSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvKiogVGhlIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ2JyZWFrcG9pbnRMb2NhdGlvbnMnIHJlcXVlc3QuICovXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0JyZWFrcG9pbnRMb2NhdGlvbnNSZXF1ZXN0ID0gZmFsc2U7XG5cblx0XHQvKiogVGhlIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCB0aGUgJ2NsaXBib2FyZCcgY29udGV4dCB2YWx1ZSBpbiB0aGUgJ2V2YWx1YXRlJyByZXF1ZXN0LiAqL1xuXHRcdHJlc3BvbnNlLmJvZHkuc3VwcG9ydHNDbGlwYm9hcmRDb250ZXh0ID0gZmFsc2U7XG5cblx0XHQvKiogVGhlIGRlYnVnIGFkYXB0ZXIgZG9lcyBub3Qgc3VwcG9ydCBzdGVwcGluZyBncmFudWxhcml0aWVzIGZvciB0aGUgc3RlcHBpbmcgcmVxdWVzdHMuICovXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c1N0ZXBwaW5nR3JhbnVsYXJpdHkgPSBmYWxzZTtcblxuXHRcdC8qKiBUaGUgZGVidWcgYWRhcHRlciBkb2VzIG5vdCBzdXBwb3J0IHRoZSAnc2V0SW5zdHJ1Y3Rpb25CcmVha3BvaW50cycgcmVxdWVzdC4gKi9cblx0XHRyZXNwb25zZS5ib2R5LnN1cHBvcnRzSW5zdHJ1Y3Rpb25CcmVha3BvaW50cyA9IGZhbHNlO1xuXG5cdFx0LyoqIFRoZSBkZWJ1ZyBhZGFwdGVyIGRvZXMgbm90IHN1cHBvcnQgJ2ZpbHRlck9wdGlvbnMnIG9uIHRoZSAnc2V0RXhjZXB0aW9uQnJlYWtwb2ludHMnIHJlcXVlc3QuICovXG5cdFx0cmVzcG9uc2UuYm9keS5zdXBwb3J0c0V4Y2VwdGlvbkZpbHRlck9wdGlvbnMgPSBmYWxzZTtcblxuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBkaXNjb25uZWN0UmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5EaXNjb25uZWN0UmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuRGlzY29ubmVjdEFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0XHR0aGlzLnNodXRkb3duKCk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgbGF1bmNoUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5MYXVuY2hSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5MYXVuY2hSZXF1ZXN0QXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGF0dGFjaFJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuQXR0YWNoUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuQXR0YWNoUmVxdWVzdEFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCB0ZXJtaW5hdGVSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlRlcm1pbmF0ZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLlRlcm1pbmF0ZUFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCByZXN0YXJ0UmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5SZXN0YXJ0UmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuUmVzdGFydEFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBzZXRCcmVha1BvaW50c1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuU2V0QnJlYWtwb2ludHNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TZXRCcmVha3BvaW50c0FyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBzZXRGdW5jdGlvbkJyZWFrUG9pbnRzUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5TZXRGdW5jdGlvbkJyZWFrcG9pbnRzUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU2V0RnVuY3Rpb25CcmVha3BvaW50c0FyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBzZXRFeGNlcHRpb25CcmVha1BvaW50c1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuU2V0RXhjZXB0aW9uQnJlYWtwb2ludHNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TZXRFeGNlcHRpb25CcmVha3BvaW50c0FyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBjb25maWd1cmF0aW9uRG9uZVJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuQ29uZmlndXJhdGlvbkRvbmVSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5Db25maWd1cmF0aW9uRG9uZUFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBjb250aW51ZVJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuQ29udGludWVSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5Db250aW51ZUFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCkgOiB2b2lkIHtcblx0XHR0aGlzLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgbmV4dFJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuTmV4dFJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLk5leHRBcmd1bWVudHMsIHJlcXVlc3Q/OiBEZWJ1Z1Byb3RvY29sLlJlcXVlc3QpIDogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHN0ZXBJblJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuU3RlcEluUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU3RlcEluQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KSA6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBzdGVwT3V0UmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5TdGVwT3V0UmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU3RlcE91dEFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCkgOiB2b2lkIHtcblx0XHR0aGlzLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgc3RlcEJhY2tSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlN0ZXBCYWNrUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU3RlcEJhY2tBcmd1bWVudHMsIHJlcXVlc3Q/OiBEZWJ1Z1Byb3RvY29sLlJlcXVlc3QpIDogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHJldmVyc2VDb250aW51ZVJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuUmV2ZXJzZUNvbnRpbnVlUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuUmV2ZXJzZUNvbnRpbnVlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KSA6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCByZXN0YXJ0RnJhbWVSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlJlc3RhcnRGcmFtZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLlJlc3RhcnRGcmFtZUFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCkgOiB2b2lkIHtcblx0XHR0aGlzLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgZ290b1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuR290b1Jlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLkdvdG9Bcmd1bWVudHMsIHJlcXVlc3Q/OiBEZWJ1Z1Byb3RvY29sLlJlcXVlc3QpIDogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHBhdXNlUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5QYXVzZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLlBhdXNlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KSA6IHZvaWQge1xuXHRcdHRoaXMuc2VuZFJlc3BvbnNlKHJlc3BvbnNlKTtcblx0fVxuXG5cdHByb3RlY3RlZCBzb3VyY2VSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlNvdXJjZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLlNvdXJjZUFyZ3VtZW50cywgcmVxdWVzdD86IERlYnVnUHJvdG9jb2wuUmVxdWVzdCkgOiB2b2lkIHtcblx0XHR0aGlzLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgdGhyZWFkc1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuVGhyZWFkc1Jlc3BvbnNlLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHRlcm1pbmF0ZVRocmVhZHNSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlRlcm1pbmF0ZVRocmVhZHNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5UZXJtaW5hdGVUaHJlYWRzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHN0YWNrVHJhY2VSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlN0YWNrVHJhY2VSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TdGFja1RyYWNlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHNjb3Blc1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuU2NvcGVzUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU2NvcGVzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHZhcmlhYmxlc1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuVmFyaWFibGVzUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuVmFyaWFibGVzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHNldFZhcmlhYmxlUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5TZXRWYXJpYWJsZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLlNldFZhcmlhYmxlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHNldEV4cHJlc3Npb25SZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlNldEV4cHJlc3Npb25SZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TZXRFeHByZXNzaW9uQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGV2YWx1YXRlUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5FdmFsdWF0ZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLkV2YWx1YXRlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHN0ZXBJblRhcmdldHNSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlN0ZXBJblRhcmdldHNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TdGVwSW5UYXJnZXRzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGdvdG9UYXJnZXRzUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5Hb3RvVGFyZ2V0c1Jlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLkdvdG9UYXJnZXRzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGNvbXBsZXRpb25zUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5Db21wbGV0aW9uc1Jlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLkNvbXBsZXRpb25zQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGV4Y2VwdGlvbkluZm9SZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLkV4Y2VwdGlvbkluZm9SZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5FeGNlcHRpb25JbmZvQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGxvYWRlZFNvdXJjZXNSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLkxvYWRlZFNvdXJjZXNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5Mb2FkZWRTb3VyY2VzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGRhdGFCcmVha3BvaW50SW5mb1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuRGF0YUJyZWFrcG9pbnRJbmZvUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuRGF0YUJyZWFrcG9pbnRJbmZvQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHNldERhdGFCcmVha3BvaW50c1JlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuU2V0RGF0YUJyZWFrcG9pbnRzUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuU2V0RGF0YUJyZWFrcG9pbnRzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHJlYWRNZW1vcnlSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlJlYWRNZW1vcnlSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5SZWFkTWVtb3J5QXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHdyaXRlTWVtb3J5UmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5Xcml0ZU1lbW9yeVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLldyaXRlTWVtb3J5QXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGRpc2Fzc2VtYmxlUmVxdWVzdChyZXNwb25zZTogRGVidWdQcm90b2NvbC5EaXNhc3NlbWJsZVJlc3BvbnNlLCBhcmdzOiBEZWJ1Z1Byb3RvY29sLkRpc2Fzc2VtYmxlQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGNhbmNlbFJlcXVlc3QocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuQ2FuY2VsUmVzcG9uc2UsIGFyZ3M6IERlYnVnUHJvdG9jb2wuQ2FuY2VsQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIGJyZWFrcG9pbnRMb2NhdGlvbnNSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLkJyZWFrcG9pbnRMb2NhdGlvbnNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5CcmVha3BvaW50TG9jYXRpb25zQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0cHJvdGVjdGVkIHNldEluc3RydWN0aW9uQnJlYWtwb2ludHNSZXF1ZXN0KHJlc3BvbnNlOiBEZWJ1Z1Byb3RvY29sLlNldEluc3RydWN0aW9uQnJlYWtwb2ludHNSZXNwb25zZSwgYXJnczogRGVidWdQcm90b2NvbC5TZXRJbnN0cnVjdGlvbkJyZWFrcG9pbnRzQXJndW1lbnRzLCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kUmVzcG9uc2UocmVzcG9uc2UpO1xuXHR9XG5cblx0LyoqXG5cdCAqIE92ZXJyaWRlIHRoaXMgaG9vayB0byBpbXBsZW1lbnQgY3VzdG9tIHJlcXVlc3RzLlxuXHQgKi9cblx0cHJvdGVjdGVkIGN1c3RvbVJlcXVlc3QoY29tbWFuZDogc3RyaW5nLCByZXNwb25zZTogRGVidWdQcm90b2NvbC5SZXNwb25zZSwgYXJnczogYW55LCByZXF1ZXN0PzogRGVidWdQcm90b2NvbC5SZXF1ZXN0KTogdm9pZCB7XG5cdFx0dGhpcy5zZW5kRXJyb3JSZXNwb25zZShyZXNwb25zZSwgMTAxNCwgJ3VucmVjb2duaXplZCByZXF1ZXN0JywgbnVsbCwgRXJyb3JEZXN0aW5hdGlvbi5UZWxlbWV0cnkpO1xuXHR9XG5cblx0Ly8tLS0tIHByb3RlY3RlZCAtLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tXG5cblx0cHJvdGVjdGVkIGNvbnZlcnRDbGllbnRMaW5lVG9EZWJ1Z2dlcihsaW5lOiBudW1iZXIpOiBudW1iZXIge1xuXHRcdGlmICh0aGlzLl9kZWJ1Z2dlckxpbmVzU3RhcnRBdDEpIHtcblx0XHRcdHJldHVybiB0aGlzLl9jbGllbnRMaW5lc1N0YXJ0QXQxID8gbGluZSA6IGxpbmUgKyAxO1xuXHRcdH1cblx0XHRyZXR1cm4gdGhpcy5fY2xpZW50TGluZXNTdGFydEF0MSA/IGxpbmUgLSAxIDogbGluZTtcblx0fVxuXG5cdHByb3RlY3RlZCBjb252ZXJ0RGVidWdnZXJMaW5lVG9DbGllbnQobGluZTogbnVtYmVyKTogbnVtYmVyIHtcblx0XHRpZiAodGhpcy5fZGVidWdnZXJMaW5lc1N0YXJ0QXQxKSB7XG5cdFx0XHRyZXR1cm4gdGhpcy5fY2xpZW50TGluZXNTdGFydEF0MSA/IGxpbmUgOiBsaW5lIC0gMTtcblx0XHR9XG5cdFx0cmV0dXJuIHRoaXMuX2NsaWVudExpbmVzU3RhcnRBdDEgPyBsaW5lICsgMSA6IGxpbmU7XG5cdH1cblxuXHRwcm90ZWN0ZWQgY29udmVydENsaWVudENvbHVtblRvRGVidWdnZXIoY29sdW1uOiBudW1iZXIpOiBudW1iZXIge1xuXHRcdGlmICh0aGlzLl9kZWJ1Z2dlckNvbHVtbnNTdGFydEF0MSkge1xuXHRcdFx0cmV0dXJuIHRoaXMuX2NsaWVudENvbHVtbnNTdGFydEF0MSA/IGNvbHVtbiA6IGNvbHVtbiArIDE7XG5cdFx0fVxuXHRcdHJldHVybiB0aGlzLl9jbGllbnRDb2x1bW5zU3RhcnRBdDEgPyBjb2x1bW4gLSAxIDogY29sdW1uO1xuXHR9XG5cblx0cHJvdGVjdGVkIGNvbnZlcnREZWJ1Z2dlckNvbHVtblRvQ2xpZW50KGNvbHVtbjogbnVtYmVyKTogbnVtYmVyIHtcblx0XHRpZiAodGhpcy5fZGVidWdnZXJDb2x1bW5zU3RhcnRBdDEpIHtcblx0XHRcdHJldHVybiB0aGlzLl9jbGllbnRDb2x1bW5zU3RhcnRBdDEgPyBjb2x1bW4gOiBjb2x1bW4gLSAxO1xuXHRcdH1cblx0XHRyZXR1cm4gdGhpcy5fY2xpZW50Q29sdW1uc1N0YXJ0QXQxID8gY29sdW1uICsgMSA6IGNvbHVtbjtcblx0fVxuXG5cdHByb3RlY3RlZCBjb252ZXJ0Q2xpZW50UGF0aFRvRGVidWdnZXIoY2xpZW50UGF0aDogc3RyaW5nKTogc3RyaW5nIHtcblx0XHRpZiAodGhpcy5fY2xpZW50UGF0aHNBcmVVUklzICE9PSB0aGlzLl9kZWJ1Z2dlclBhdGhzQXJlVVJJcykge1xuXHRcdFx0aWYgKHRoaXMuX2NsaWVudFBhdGhzQXJlVVJJcykge1xuXHRcdFx0XHRyZXR1cm4gRGVidWdTZXNzaW9uLnVyaTJwYXRoKGNsaWVudFBhdGgpO1xuXHRcdFx0fSBlbHNlIHtcblx0XHRcdFx0cmV0dXJuIERlYnVnU2Vzc2lvbi5wYXRoMnVyaShjbGllbnRQYXRoKTtcblx0XHRcdH1cblx0XHR9XG5cdFx0cmV0dXJuIGNsaWVudFBhdGg7XG5cdH1cblxuXHRwcm90ZWN0ZWQgY29udmVydERlYnVnZ2VyUGF0aFRvQ2xpZW50KGRlYnVnZ2VyUGF0aDogc3RyaW5nKTogc3RyaW5nIHtcblx0XHRpZiAodGhpcy5fZGVidWdnZXJQYXRoc0FyZVVSSXMgIT09IHRoaXMuX2NsaWVudFBhdGhzQXJlVVJJcykge1xuXHRcdFx0aWYgKHRoaXMuX2RlYnVnZ2VyUGF0aHNBcmVVUklzKSB7XG5cdFx0XHRcdHJldHVybiBEZWJ1Z1Nlc3Npb24udXJpMnBhdGgoZGVidWdnZXJQYXRoKTtcblx0XHRcdH0gZWxzZSB7XG5cdFx0XHRcdHJldHVybiBEZWJ1Z1Nlc3Npb24ucGF0aDJ1cmkoZGVidWdnZXJQYXRoKTtcblx0XHRcdH1cblx0XHR9XG5cdFx0cmV0dXJuIGRlYnVnZ2VyUGF0aDtcblx0fVxuXG5cdC8vLS0tLSBwcml2YXRlIC0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cblxuXHRwcml2YXRlIHN0YXRpYyBwYXRoMnVyaShwYXRoOiBzdHJpbmcpOiBzdHJpbmcge1xuXG5cdFx0aWYgKHByb2Nlc3MucGxhdGZvcm0gPT09ICd3aW4zMicpIHtcblx0XHRcdGlmICgvXltBLVpdOi8udGVzdChwYXRoKSkge1xuXHRcdFx0XHRwYXRoID0gcGF0aFswXS50b0xvd2VyQ2FzZSgpICsgcGF0aC5zdWJzdHIoMSk7XG5cdFx0XHR9XG5cdFx0XHRwYXRoID0gcGF0aC5yZXBsYWNlKC9cXFxcL2csICcvJyk7XG5cdFx0fVxuXHRcdHBhdGggPSBlbmNvZGVVUkkocGF0aCk7XG5cblx0XHRsZXQgdXJpID0gbmV3IFVSTChgZmlsZTpgKTtcdC8vIGlnbm9yZSAncGF0aCcgZm9yIG5vd1xuXHRcdHVyaS5wYXRobmFtZSA9IHBhdGg7XHQvLyBub3cgdXNlICdwYXRoJyB0byBnZXQgdGhlIGNvcnJlY3QgcGVyY2VudCBlbmNvZGluZyAoc2VlIGh0dHBzOi8vdXJsLnNwZWMud2hhdHdnLm9yZylcblx0XHRyZXR1cm4gdXJpLnRvU3RyaW5nKCk7XG5cdH1cblxuXHRwcml2YXRlIHN0YXRpYyB1cmkycGF0aChzb3VyY2VVcmk6IHN0cmluZyk6IHN0cmluZyB7XG5cblx0XHRsZXQgdXJpID0gbmV3IFVSTChzb3VyY2VVcmkpO1xuXHRcdGxldCBzID0gZGVjb2RlVVJJQ29tcG9uZW50KHVyaS5wYXRobmFtZSk7XG5cdFx0aWYgKHByb2Nlc3MucGxhdGZvcm0gPT09ICd3aW4zMicpIHtcblx0XHRcdGlmICgvXlxcL1thLXpBLVpdOi8udGVzdChzKSkge1xuXHRcdFx0XHRzID0gc1sxXS50b0xvd2VyQ2FzZSgpICsgcy5zdWJzdHIoMik7XG5cdFx0XHR9XG5cdFx0XHRzID0gcy5yZXBsYWNlKC9cXC8vZywgJ1xcXFwnKTtcblx0XHR9XG5cdFx0cmV0dXJuIHM7XG5cdH1cblxuXHRwcml2YXRlIHN0YXRpYyBfZm9ybWF0UElJUmVnZXhwID0gL3soW159XSspfS9nO1xuXG5cdC8qXG5cdCogSWYgYXJndW1lbnQgc3RhcnRzIHdpdGggJ18nIGl0IGlzIE9LIHRvIHNlbmQgaXRzIHZhbHVlIHRvIHRlbGVtZXRyeS5cblx0Ki9cblx0cHJpdmF0ZSBzdGF0aWMgZm9ybWF0UElJKGZvcm1hdDpzdHJpbmcsIGV4Y2x1ZGVQSUk6IGJvb2xlYW4sIGFyZ3M6IHtba2V5OiBzdHJpbmddOiBzdHJpbmd9KTogc3RyaW5nIHtcblx0XHRyZXR1cm4gZm9ybWF0LnJlcGxhY2UoRGVidWdTZXNzaW9uLl9mb3JtYXRQSUlSZWdleHAsIGZ1bmN0aW9uKG1hdGNoLCBwYXJhbU5hbWUpIHtcblx0XHRcdGlmIChleGNsdWRlUElJICYmIHBhcmFtTmFtZS5sZW5ndGggPiAwICYmIHBhcmFtTmFtZVswXSAhPT0gJ18nKSB7XG5cdFx0XHRcdHJldHVybiBtYXRjaDtcblx0XHRcdH1cblx0XHRcdHJldHVybiBhcmdzW3BhcmFtTmFtZV0gJiYgYXJncy5oYXNPd25Qcm9wZXJ0eShwYXJhbU5hbWUpID9cblx0XHRcdFx0YXJnc1twYXJhbU5hbWVdIDpcblx0XHRcdFx0bWF0Y2g7XG5cdFx0fSlcblx0fVxufVxuIl19

/***/ }),
/* 7 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.ProtocolServer = void 0;
const ee = __webpack_require__(8);
const messages_1 = __webpack_require__(9);
class Disposable0 {
    dispose() {
    }
}
class Emitter {
    get event() {
        if (!this._event) {
            this._event = (listener, thisArg) => {
                this._listener = listener;
                this._this = thisArg;
                let result;
                result = {
                    dispose: () => {
                        this._listener = undefined;
                        this._this = undefined;
                    }
                };
                return result;
            };
        }
        return this._event;
    }
    fire(event) {
        if (this._listener) {
            try {
                this._listener.call(this._this, event);
            }
            catch (e) {
            }
        }
    }
    hasListener() {
        return !!this._listener;
    }
    dispose() {
        this._listener = undefined;
        this._this = undefined;
    }
}
class ProtocolServer extends ee.EventEmitter {
    constructor() {
        super();
        this._sendMessage = new Emitter();
        this._pendingRequests = new Map();
        this.onDidSendMessage = this._sendMessage.event;
    }
    // ---- implements vscode.Debugadapter interface ---------------------------
    dispose() {
    }
    handleMessage(msg) {
        if (msg.type === 'request') {
            this.dispatchRequest(msg);
        }
        else if (msg.type === 'response') {
            const response = msg;
            const clb = this._pendingRequests.get(response.request_seq);
            if (clb) {
                this._pendingRequests.delete(response.request_seq);
                clb(response);
            }
        }
    }
    _isRunningInline() {
        return this._sendMessage && this._sendMessage.hasListener();
    }
    //--------------------------------------------------------------------------
    start(inStream, outStream) {
        this._sequence = 1;
        this._writableStream = outStream;
        this._rawData = Buffer.alloc(0);
        inStream.on('data', (data) => this._handleData(data));
        inStream.on('close', () => {
            this._emitEvent(new messages_1.Event('close'));
        });
        inStream.on('error', (error) => {
            this._emitEvent(new messages_1.Event('error', 'inStream error: ' + (error && error.message)));
        });
        outStream.on('error', (error) => {
            this._emitEvent(new messages_1.Event('error', 'outStream error: ' + (error && error.message)));
        });
        inStream.resume();
    }
    stop() {
        if (this._writableStream) {
            this._writableStream.end();
        }
    }
    sendEvent(event) {
        this._send('event', event);
    }
    sendResponse(response) {
        if (response.seq > 0) {
            console.error(`attempt to send more than one response for command ${response.command}`);
        }
        else {
            this._send('response', response);
        }
    }
    sendRequest(command, args, timeout, cb) {
        const request = {
            command: command
        };
        if (args && Object.keys(args).length > 0) {
            request.arguments = args;
        }
        this._send('request', request);
        if (cb) {
            this._pendingRequests.set(request.seq, cb);
            const timer = setTimeout(() => {
                clearTimeout(timer);
                const clb = this._pendingRequests.get(request.seq);
                if (clb) {
                    this._pendingRequests.delete(request.seq);
                    clb(new messages_1.Response(request, 'timeout'));
                }
            }, timeout);
        }
    }
    // ---- protected ----------------------------------------------------------
    dispatchRequest(request) {
    }
    // ---- private ------------------------------------------------------------
    _emitEvent(event) {
        this.emit(event.event, event);
    }
    _send(typ, message) {
        message.type = typ;
        message.seq = this._sequence++;
        if (this._writableStream) {
            const json = JSON.stringify(message);
            this._writableStream.write(`Content-Length: ${Buffer.byteLength(json, 'utf8')}\r\n\r\n${json}`, 'utf8');
        }
        this._sendMessage.fire(message);
    }
    _handleData(data) {
        this._rawData = Buffer.concat([this._rawData, data]);
        while (true) {
            if (this._contentLength >= 0) {
                if (this._rawData.length >= this._contentLength) {
                    const message = this._rawData.toString('utf8', 0, this._contentLength);
                    this._rawData = this._rawData.slice(this._contentLength);
                    this._contentLength = -1;
                    if (message.length > 0) {
                        try {
                            let msg = JSON.parse(message);
                            this.handleMessage(msg);
                        }
                        catch (e) {
                            this._emitEvent(new messages_1.Event('error', 'Error handling data: ' + (e && e.message)));
                        }
                    }
                    continue; // there may be more complete messages to process
                }
            }
            else {
                const idx = this._rawData.indexOf(ProtocolServer.TWO_CRLF);
                if (idx !== -1) {
                    const header = this._rawData.toString('utf8', 0, idx);
                    const lines = header.split('\r\n');
                    for (let i = 0; i < lines.length; i++) {
                        const pair = lines[i].split(/: +/);
                        if (pair[0] == 'Content-Length') {
                            this._contentLength = +pair[1];
                        }
                    }
                    this._rawData = this._rawData.slice(idx + ProtocolServer.TWO_CRLF.length);
                    continue;
                }
            }
            break;
        }
    }
}
exports.ProtocolServer = ProtocolServer;
ProtocolServer.TWO_CRLF = '\r\n\r\n';
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoicHJvdG9jb2wuanMiLCJzb3VyY2VSb290IjoiIiwic291cmNlcyI6WyIuLi9zcmMvcHJvdG9jb2wudHMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IjtBQUFBOzs7Z0dBR2dHOzs7QUFFaEcsNkJBQTZCO0FBRTdCLHlDQUE2QztBQVM3QyxNQUFNLFdBQVc7SUFDaEIsT0FBTztJQUNQLENBQUM7Q0FDRDtBQU1ELE1BQU0sT0FBTztJQU1aLElBQUksS0FBSztRQUNSLElBQUksQ0FBQyxJQUFJLENBQUMsTUFBTSxFQUFFO1lBQ2pCLElBQUksQ0FBQyxNQUFNLEdBQUcsQ0FBQyxRQUF1QixFQUFFLE9BQWEsRUFBRSxFQUFFO2dCQUV4RCxJQUFJLENBQUMsU0FBUyxHQUFHLFFBQVEsQ0FBQztnQkFDMUIsSUFBSSxDQUFDLEtBQUssR0FBRyxPQUFPLENBQUM7Z0JBRXJCLElBQUksTUFBbUIsQ0FBQztnQkFDeEIsTUFBTSxHQUFHO29CQUNSLE9BQU8sRUFBRSxHQUFHLEVBQUU7d0JBQ2IsSUFBSSxDQUFDLFNBQVMsR0FBRyxTQUFTLENBQUM7d0JBQzNCLElBQUksQ0FBQyxLQUFLLEdBQUcsU0FBUyxDQUFDO29CQUN4QixDQUFDO2lCQUNELENBQUM7Z0JBQ0YsT0FBTyxNQUFNLENBQUM7WUFDZixDQUFDLENBQUM7U0FDRjtRQUNELE9BQU8sSUFBSSxDQUFDLE1BQU0sQ0FBQztJQUNwQixDQUFDO0lBRUQsSUFBSSxDQUFDLEtBQVE7UUFDWixJQUFJLElBQUksQ0FBQyxTQUFTLEVBQUU7WUFDbkIsSUFBSTtnQkFDSCxJQUFJLENBQUMsU0FBUyxDQUFDLElBQUksQ0FBQyxJQUFJLENBQUMsS0FBSyxFQUFFLEtBQUssQ0FBQyxDQUFDO2FBQ3ZDO1lBQUMsT0FBTyxDQUFDLEVBQUU7YUFDWDtTQUNEO0lBQ0YsQ0FBQztJQUVELFdBQVc7UUFDVixPQUFPLENBQUMsQ0FBQyxJQUFJLENBQUMsU0FBUyxDQUFDO0lBQ3pCLENBQUM7SUFFRCxPQUFPO1FBQ04sSUFBSSxDQUFDLFNBQVMs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SSxnQkFBZ0IsRUFBRTs0QkFDaEMsSUFBSSxDQUFDLGNBQWMsR0FBRyxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsQ0FBQzt5QkFDL0I7cUJBQ0Q7b0JBQ0QsSUFBSSxDQUFDLFFBQVEsR0FBRyxJQUFJLENBQUMsUUFBUSxDQUFDLEtBQUssQ0FBQyxHQUFHLEdBQUcsY0FBYyxDQUFDLFFBQVEsQ0FBQyxNQUFNLENBQUMsQ0FBQztvQkFDMUUsU0FBUztpQkFDVDthQUNEO1lBQ0QsTUFBTTtTQUNOO0lBQ0YsQ0FBQzs7QUF2S0Ysd0NBd0tDO0FBdEtlLHVCQUFRLEdBQUcsVUFBVSxDQUFDIiwic291cmNlc0NvbnRlbnQiOlsiLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cbiAqICBDb3B5cmlnaHQgKGMpIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbi4gQWxsIHJpZ2h0cyByZXNlcnZlZC5cbiAqICBMaWNlbnNlZCB1bmRlciB0aGUgTUlUIExpY2Vuc2UuIFNlZSBMaWNlbnNlLnR4dCBpbiB0aGUgcHJvamVjdCByb290IGZvciBsaWNlbnNlIGluZm9ybWF0aW9uLlxuICotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbmltcG9ydCAqIGFzIGVlIGZyb20gJ2V2ZW50cyc7XG5pbXBvcnQgeyBEZWJ1Z1Byb3RvY29sIH0gZnJvbSAndnNjb2RlLWRlYnVncHJvdG9jb2wnO1xuaW1wb3J0IHsgUmVzcG9uc2UsIEV2ZW50IH0gZnJvbSAnLi9tZXNzYWdlcyc7XG5cbmludGVyZmFjZSBEZWJ1Z1Byb3RvY29sTWVzc2FnZSB7XG59XG5cbmludGVyZmFjZSBJRGlzcG9zYWJsZSB7XG5cdGRpc3Bvc2UoKTogdm9pZDtcbn1cblxuY2xhc3MgRGlzcG9zYWJsZTAgaW1wbGVtZW50cyBJRGlzcG9zYWJsZSB7XG5cdGRpc3Bvc2UoKTogYW55IHtcblx0fVxufVxuXG5pbnRlcmZhY2UgRXZlbnQwPFQ+IHtcblx0KGxpc3RlbmVyOiAoZTogVCkgPT4gYW55LCB0aGlzQXJnPzogYW55KTogRGlzcG9zYWJsZTA7XG59XG5cbmNsYXNzIEVtaXR0ZXI8VD4ge1xuXG5cdHByaXZhdGUgX2V2ZW50PzogRXZlbnQwPFQ+O1xuXHRwcml2YXRlIF9saXN0ZW5lcj86IChlOiBUKSA9PiB2b2lkO1xuXHRwcml2YXRlIF90aGlzPzogYW55O1xuXG5cdGdldCBldmVudCgpOiBFdmVudDA8VD4ge1xuXHRcdGlmICghdGhpcy5fZXZlbnQpIHtcblx0XHRcdHRoaXMuX2V2ZW50ID0gKGxpc3RlbmVyOiAoZTogVCkgPT4gYW55LCB0aGlzQXJnPzogYW55KSA9PiB7XG5cblx0XHRcdFx0dGhpcy5fbGlzdGVuZXIgPSBsaXN0ZW5lcjtcblx0XHRcdFx0dGhpcy5fdGhpcyA9IHRoaXNBcmc7XG5cblx0XHRcdFx0bGV0IHJlc3VsdDogSURpc3Bvc2FibGU7XG5cdFx0XHRcdHJlc3VsdCA9IHtcblx0XHRcdFx0XHRkaXNwb3NlOiAoKSA9PiB7XG5cdFx0XHRcdFx0XHR0aGlzLl9saXN0ZW5lciA9IHVuZGVmaW5lZDtcblx0XHRcdFx0XHRcdHRoaXMuX3RoaXMgPSB1bmRlZmluZWQ7XG5cdFx0XHRcdFx0fVxuXHRcdFx0XHR9O1xuXHRcdFx0XHRyZXR1cm4gcmVzdWx0O1xuXHRcdFx0fTtcblx0XHR9XG5cdFx0cmV0dXJuIHRoaXMuX2V2ZW50O1xuXHR9XG5cblx0ZmlyZShldmVudDogVCk6IHZvaWQge1xuXHRcdGlmICh0aGlzLl9saXN0ZW5lcikge1xuXHRcdFx0dHJ5IHtcblx0XHRcdFx0dGhpcy5fbGlzdGVuZXIuY2FsbCh0aGlzLl90aGlzLCBldmVudCk7XG5cdFx0XHR9IGNhdGNoIChlKSB7XG5cdFx0XHR9XG5cdFx0fVxuXHR9XG5cblx0aGFzTGlzdGVuZXIoKSA6IGJvb2xlYW4ge1xuXHRcdHJldHVybiAhIXRoaXMuX2xpc3RlbmVyO1xuXHR9XG5cblx0ZGlzcG9zZSgpIHtcblx0XHR0aGlzLl9saXN0ZW5lciA9IHVuZGVmaW5lZDtcblx0XHR0aGlzLl90aGlzID0gdW5kZWZpbmVkO1xuXHR9XG59XG5cbi8qKlxuICogQSBzdHJ1Y3R1cmFsbHkgZXF1aXZhbGVudCBjb3B5IG9mIHZzY29kZS5EZWJ1Z0FkYXB0ZXJcbiAqL1xuaW50ZXJmYWNlIFZTQ29kZURlYnVnQWRhcHRlciBleHRlbmRzIERpc3Bvc2FibGUwIHtcblxuXHRyZWFkb25seSBvbkRpZFNlbmRNZXNzYWdlOiBFdmVudDA8RGVidWdQcm90b2NvbE1lc3NhZ2U+O1xuXG5cdGhhbmRsZU1lc3NhZ2UobWVzc2FnZTogRGVidWdQcm90b2NvbC5Qcm90b2NvbE1lc3NhZ2UpOiB2b2lkO1xufVxuXG5leHBvcnQgY2xhc3MgUHJvdG9jb2xTZXJ2ZXIgZXh0ZW5kcyBlZS5FdmVudEVtaXR0ZXIgaW1wbGVtZW50cyBWU0NvZGVEZWJ1Z0FkYXB0ZXIge1xuXG5cdHByaXZhdGUgc3RhdGljIFRXT19DUkxGID0gJ1xcclxcblxcclxcbic7XG5cblx0cHJpdmF0ZSBfc2VuZE1lc3NhZ2UgPSBuZXcgRW1pdHRlcjxEZWJ1Z1Byb3RvY29sTWVzc2FnZT4oKTtcblxuXHRwcml2YXRlIF9yYXdEYXRhOiBCdWZmZXI7XG5cdHByaXZhdGUgX2NvbnRlbnRMZW5ndGg6IG51bWJlcjtcblx0cHJpdmF0ZSBfc2VxdWVuY2U6IG51bWJlcjtcblx0cHJpdmF0ZSBfd3JpdGFibGVTdHJlYW06IE5vZGVKUy5Xcml0YWJsZVN0cmVhbTtcblx0cHJpdmF0ZSBfcGVuZGluZ1JlcXVlc3RzID0gbmV3IE1hcDxudW1iZXIsIChyZXNwb25zZTogRGVidWdQcm90b2NvbC5SZXNwb25zZSkgPT4gdm9pZD4oKTtcblxuXHRjb25zdHJ1Y3RvcigpIHtcblx0XHRzdXBlcigpO1xuXHR9XG5cblx0Ly8gLS0tLSBpbXBsZW1lbnRzIHZzY29kZS5EZWJ1Z2FkYXB0ZXIgaW50ZXJmYWNlIC0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLVxuXG5cdHB1YmxpYyBkaXNwb3NlKCk6IGFueSB7XG5cdH1cblxuXHRwdWJsaWMgb25EaWRTZW5kTWVzc2FnZTogRXZlbnQwPERlYnVnUHJvdG9jb2xNZXNzYWdlPiA9IHRoaXMuX3NlbmRNZXNzYWdlLmV2ZW50O1xuXG5cdHB1YmxpYyBoYW5kbGVNZXNzYWdlKG1zZzogRGVidWdQcm90b2NvbC5Qcm90b2NvbE1lc3NhZ2UpOiB2b2lkIHtcblx0XHRpZiAobXNnLnR5cGUgPT09ICdyZXF1ZXN0Jykge1xuXHRcdFx0dGhpcy5kaXNwYXRjaFJlcXVlc3QoPERlYnVnUHJvdG9jb2wuUmVxdWVzdD5tc2cpO1xuXHRcdH0gZWxzZSBpZiAobXNnLnR5cGUgPT09ICdyZXNwb25zZScpIHtcblx0XHRcdGNvbnN0IHJlc3BvbnNlID0gPERlYnVnUHJvdG9jb2wuUmVzcG9uc2U+bXNnO1xuXHRcdFx0Y29uc3QgY2xiID0gdGhpcy5fcGVuZGluZ1JlcXVlc3RzLmdldChyZXNwb25zZS5yZXF1ZXN0X3NlcSk7XG5cdFx0XHRpZiAoY2xiKSB7XG5cdFx0XHRcdHRoaXMuX3BlbmRpbmdSZXF1ZXN0cy5kZWxldGUocmVzcG9uc2UucmVxdWVzdF9zZXEpO1xuXHRcdFx0XHRjbGIocmVzcG9uc2UpO1xuXHRcdFx0fVxuXHRcdH1cblx0fVxuXG5cdHByb3RlY3RlZCBfaXNSdW5uaW5nSW5saW5lKCkge1xuXHRcdHJldHVybiB0aGlzLl9zZW5kTWVzc2FnZSAmJiB0aGlzLl9zZW5kTWVzc2FnZS5oYXNMaXN0ZW5lcigpO1xuXHR9XG5cblx0Ly8tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLVxuXG5cdHB1YmxpYyBzdGFydChpblN0cmVhbTogTm9kZUpTLlJlYWRhYmxlU3RyZWFtLCBvdXRTdHJlYW06IE5vZGVKUy5Xcml0YWJsZVN0cmVhbSk6IHZvaWQge1xuXHRcdHRoaXMuX3NlcXVlbmNlID0gMTtcblx0XHR0aGlzLl93cml0YWJsZVN0cmVhbSA9IG91dFN0cmVhbTtcblx0XHR0aGlzLl9yYXdEYXRhID0gQnVmZmVyLmFsbG9jKDApO1xuXG5cdFx0aW5TdHJlYW0ub24oJ2RhdGEnLCAoZGF0YTogQnVmZmVyKSA9PiB0aGlzLl9oYW5kbGVEYXRhKGRhdGEpKTtcblxuXHRcdGluU3RyZWFtLm9uKCdjbG9zZScsICgpID0+IHtcblx0XHRcdHRoaXMuX2VtaXRFdmVudChuZXcgRXZlbnQoJ2Nsb3NlJykpO1xuXHRcdH0pO1xuXHRcdGluU3RyZWFtLm9uKCdlcnJvcicsIChlcnJvcikgPT4ge1xuXHRcdFx0dGhpcy5fZW1pdEV2ZW50KG5ldyBFdmVudCgnZXJyb3InLCAnaW5TdHJlYW0gZXJyb3I6ICcgKyAoZXJyb3IgJiYgZXJyb3IubWVzc2FnZSkpKTtcblx0XHR9KTtcblxuXHRcdG91dFN0cmVhbS5vbignZXJyb3InLCAoZXJyb3IpID0+IHtcblx0XHRcdHRoaXMuX2VtaXRFdmVudChuZXcgRXZlbnQoJ2Vycm9yJywgJ291dFN0cmVhbSBlcnJvcjogJyArIChlcnJvciAmJiBlcnJvci5tZXNzYWdlKSkpO1xuXHRcdH0pO1xuXG5cdFx0aW5TdHJlYW0ucmVzdW1lKCk7XG5cdH1cblxuXHRwdWJsaWMgc3RvcCgpOiB2b2lkIHtcblx0XHRpZiAodGhpcy5fd3JpdGFibGVTdHJlYW0pIHtcblx0XHRcdHRoaXMuX3dyaXRhYmxlU3RyZWFtLmVuZCgpO1xuXHRcdH1cblx0fVxuXG5cdHB1YmxpYyBzZW5kRXZlbnQoZXZlbnQ6IERlYnVnUHJvdG9jb2wuRXZlbnQpOiB2b2lkIHtcblx0XHR0aGlzLl9zZW5kKCdldmVudCcsIGV2ZW50KTtcblx0fVxuXG5cdHB1YmxpYyBzZW5kUmVzcG9uc2UocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuUmVzcG9uc2UpOiB2b2lkIHtcblx0XHRpZiAocmVzcG9uc2Uuc2VxID4gMCkge1xuXHRcdFx0Y29uc29sZS5lcnJvcihgYXR0ZW1wdCB0byBzZW5kIG1vcmUgdGhhbiBvbmUgcmVzcG9uc2UgZm9yIGNvbW1hbmQgJHtyZXNwb25zZS5jb21tYW5kfWApO1xuXHRcdH0gZWxzZSB7XG5cdFx0XHR0aGlzLl9zZW5kKCdyZXNwb25zZScsIHJlc3BvbnNlKTtcblx0XHR9XG5cdH1cblxuXHRwdWJsaWMgc2VuZFJlcXVlc3QoY29tbWFuZDogc3RyaW5nLCBhcmdzOiBhbnksIHRpbWVvdXQ6IG51bWJlciwgY2I6IChyZXNwb25zZTogRGVidWdQcm90b2NvbC5SZXNwb25zZSkgPT4gdm9pZCkgOiB2b2lkIHtcblxuXHRcdGNvbnN0IHJlcXVlc3Q6IGFueSA9IHtcblx0XHRcdGNvbW1hbmQ6IGNvbW1hbmRcblx0XHR9O1xuXHRcdGlmIChhcmdzICYmIE9iamVjdC5rZXlzKGFyZ3MpLmxlbmd0aCA+IDApIHtcblx0XHRcdHJlcXVlc3QuYXJndW1lbnRzID0gYXJncztcblx0XHR9XG5cblx0XHR0aGlzLl9zZW5kKCdyZXF1ZXN0JywgcmVxdWVzdCk7XG5cblx0XHRpZiAoY2IpIHtcblx0XHRcdHRoaXMuX3BlbmRpbmdSZXF1ZXN0cy5zZXQocmVxdWVzdC5zZXEsIGNiKTtcblxuXHRcdFx0Y29uc3QgdGltZXIgPSBzZXRUaW1lb3V0KCgpID0+IHtcblx0XHRcdFx0Y2xlYXJUaW1lb3V0KHRpbWVyKTtcblx0XHRcdFx0Y29uc3QgY2xiID0gdGhpcy5fcGVuZGluZ1JlcXVlc3RzLmdldChyZXF1ZXN0LnNlcSk7XG5cdFx0XHRcdGlmIChjbGIpIHtcblx0XHRcdFx0XHR0aGlzLl9wZW5kaW5nUmVxdWVzdHMuZGVsZXRlKHJlcXVlc3Quc2VxKTtcblx0XHRcdFx0XHRjbGIobmV3IFJlc3BvbnNlKHJlcXVlc3QsICd0aW1lb3V0JykpO1xuXHRcdFx0XHR9XG5cdFx0XHR9LCB0aW1lb3V0KTtcblx0XHR9XG5cdH1cblxuXHQvLyAtLS0tIHByb3RlY3RlZCAtLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tXG5cblx0cHJvdGVjdGVkIGRpc3BhdGNoUmVxdWVzdChyZXF1ZXN0OiBEZWJ1Z1Byb3RvY29sLlJlcXVlc3QpOiB2b2lkIHtcblx0fVxuXG5cdC8vIC0tLS0gcHJpdmF0ZSAtLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cblxuXHRwcml2YXRlIF9lbWl0RXZlbnQoZXZlbnQ6IERlYnVnUHJvdG9jb2wuRXZlbnQpIHtcblx0XHR0aGlzLmVtaXQoZXZlbnQuZXZlbnQsIGV2ZW50KTtcblx0fVxuXG5cdHByaXZhdGUgX3NlbmQodHlwOiAncmVxdWVzdCcgfCAncmVzcG9uc2UnIHwgJ2V2ZW50JywgbWVzc2FnZTogRGVidWdQcm90b2NvbC5Qcm90b2NvbE1lc3NhZ2UpOiB2b2lkIHtcblxuXHRcdG1lc3NhZ2UudHlwZSA9IHR5cDtcblx0XHRtZXNzYWdlLnNlcSA9IHRoaXMuX3NlcXVlbmNlKys7XG5cblx0XHRpZiAodGhpcy5fd3JpdGFibGVTdHJlYW0pIHtcblx0XHRcdGNvbnN0IGpzb24gPSBKU09OLnN0cmluZ2lmeShtZXNzYWdlKTtcblx0XHRcdHRoaXMuX3dyaXRhYmxlU3RyZWFtLndyaXRlKGBDb250ZW50LUxlbmd0aDogJHtCdWZmZXIuYnl0ZUxlbmd0aChqc29uLCAndXRmOCcpfVxcclxcblxcclxcbiR7anNvbn1gLCAndXRmOCcpO1xuXHRcdH1cblx0XHR0aGlzLl9zZW5kTWVzc2FnZS5maXJlKG1lc3NhZ2UpO1xuXHR9XG5cblx0cHJpdmF0ZSBfaGFuZGxlRGF0YShkYXRhOiBCdWZmZXIpOiB2b2lkIHtcblxuXHRcdHRoaXMuX3Jhd0RhdGEgPSBCdWZmZXIuY29uY2F0KFt0aGlzLl9yYXdEYXRhLCBkYXRhXSk7XG5cblx0XHR3aGlsZSAodHJ1ZSkge1xuXHRcdFx0aWYgKHRoaXMuX2NvbnRlbnRMZW5ndGggPj0gMCkge1xuXHRcdFx0XHRpZiAodGhpcy5fcmF3RGF0YS5sZW5ndGggPj0gdGhpcy5fY29udGVudExlbmd0aCkge1xuXHRcdFx0XHRcdGNvbnN0IG1lc3NhZ2UgPSB0aGlzLl9yYXdEYXRhLnRvU3RyaW5nKCd1dGY4JywgMCwgdGhpcy5fY29udGVudExlbmd0aCk7XG5cdFx0XHRcdFx0dGhpcy5fcmF3RGF0YSA9IHRoaXMuX3Jhd0RhdGEuc2xpY2UodGhpcy5fY29udGVudExlbmd0aCk7XG5cdFx0XHRcdFx0dGhpcy5fY29udGVudExlbmd0aCA9IC0xO1xuXHRcdFx0XHRcdGlmIChtZXNzYWdlLmxlbmd0aCA+IDApIHtcblx0XHRcdFx0XHRcdHRyeSB7XG5cdFx0XHRcdFx0XHRcdGxldCBtc2c6IERlYnVnUHJvdG9jb2wuUHJvdG9jb2xNZXNzYWdlID0gSlNPTi5wYXJzZShtZXNzYWdlKTtcblx0XHRcdFx0XHRcdFx0dGhpcy5oYW5kbGVNZXNzYWdlKG1zZyk7XG5cdFx0XHRcdFx0XHR9XG5cdFx0XHRcdFx0XHRjYXRjaCAoZSkge1xuXHRcdFx0XHRcdFx0XHR0aGlzLl9lbWl0RXZlbnQobmV3IEV2ZW50KCdlcnJvcicsICdFcnJvciBoYW5kbGluZyBkYXRhOiAnICsgKGUgJiYgZS5tZXNzYWdlKSkpO1xuXHRcdFx0XHRcdFx0fVxuXHRcdFx0XHRcdH1cblx0XHRcdFx0XHRjb250aW51ZTtcdC8vIHRoZXJlIG1heSBiZSBtb3JlIGNvbXBsZXRlIG1lc3NhZ2VzIHRvIHByb2Nlc3Ncblx0XHRcdFx0fVxuXHRcdFx0fSBlbHNlIHtcblx0XHRcdFx0Y29uc3QgaWR4ID0gdGhpcy5fcmF3RGF0YS5pbmRleE9mKFByb3RvY29sU2VydmVyLlRXT19DUkxGKTtcblx0XHRcdFx0aWYgKGlkeCAhPT0gLTEpIHtcblx0XHRcdFx0XHRjb25zdCBoZWFkZXIgPSB0aGlzLl9yYXdEYXRhLnRvU3RyaW5nKCd1dGY4JywgMCwgaWR4KTtcblx0XHRcdFx0XHRjb25zdCBsaW5lcyA9IGhlYWRlci5zcGxpdCgnXFxyXFxuJyk7XG5cdFx0XHRcdFx0Zm9yIChsZXQgaSA9IDA7IGkgPCBsaW5lcy5sZW5ndGg7IGkrKykge1xuXHRcdFx0XHRcdFx0Y29uc3QgcGFpciA9IGxpbmVzW2ldLnNwbGl0KC86ICsvKTtcblx0XHRcdFx0XHRcdGlmIChwYWlyWzBdID09ICdDb250ZW50LUxlbmd0aCcpIHtcblx0XHRcdFx0XHRcdFx0dGhpcy5fY29udGVudExlbmd0aCA9ICtwYWlyWzFdO1xuXHRcdFx0XHRcdFx0fVxuXHRcdFx0XHRcdH1cblx0XHRcdFx0XHR0aGlzLl9yYXdEYXRhID0gdGhpcy5fcmF3RGF0YS5zbGljZShpZHggKyBQcm90b2NvbFNlcnZlci5UV09fQ1JMRi5sZW5ndGgpO1xuXHRcdFx0XHRcdGNvbnRpbnVlO1xuXHRcdFx0XHR9XG5cdFx0XHR9XG5cdFx0XHRicmVhaztcblx0XHR9XG5cdH1cbn1cbiJdfQ==

/***/ }),
/* 8 */
/***/ ((module) => {

"use strict";
module.exports = require("events");

/***/ }),
/* 9 */
/***/ ((__unused_webpack_module, exports) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.Event = exports.Response = exports.Message = void 0;
class Message {
    constructor(type) {
        this.seq = 0;
        this.type = type;
    }
}
exports.Message = Message;
class Response extends Message {
    constructor(request, message) {
        super('response');
        this.request_seq = request.seq;
        this.command = request.command;
        if (message) {
            this.success = false;
            this.message = message;
        }
        else {
            this.success = true;
        }
    }
}
exports.Response = Response;
class Event extends Message {
    constructor(event, body) {
        super('event');
        this.event = event;
        if (body) {
            this.body = body;
        }
    }
}
exports.Event = Event;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoibWVzc2FnZXMuanMiLCJzb3VyY2VSb290IjoiIiwic291cmNlcyI6WyIuLi9zcmMvbWVzc2FnZXMudHMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IjtBQUFBOzs7Z0dBR2dHOzs7QUFLaEcsTUFBYSxPQUFPO0lBSW5CLFlBQW1CLElBQVk7UUFDOUIsSUFBSSxDQUFDLEdBQUcsR0FBRyxDQUFDLENBQUM7UUFDYixJQUFJLENBQUMsSUFBSSxHQUFHLElBQUksQ0FBQztJQUNsQixDQUFDO0NBQ0Q7QUFSRCwwQkFRQztBQUVELE1BQWEsUUFBUyxTQUFRLE9BQU87SUFLcEMsWUFBbUIsT0FBOEIsRUFBRSxPQUFnQjtRQUNsRSxLQUFLLENBQUMsVUFBVSxDQUFDLENBQUM7UUFDbEIsSUFBSSxDQUFDLFdBQVcsR0FBRyxPQUFPLENBQUMsR0FBRyxDQUFDO1FBQy9CLElBQUksQ0FBQyxPQUFPLEdBQUcsT0FBTyxDQUFDLE9BQU8sQ0FBQztRQUMvQixJQUFJLE9BQU8sRUFBRTtZQUNaLElBQUksQ0FBQyxPQUFPLEdBQUcsS0FBSyxDQUFDO1lBQ2YsSUFBSyxDQUFDLE9BQU8sR0FBRyxPQUFPLENBQUM7U0FDOUI7YUFBTTtZQUNOLElBQUksQ0FBQyxPQUFPLEdBQUcsSUFBSSxDQUFDO1NBQ3BCO0lBQ0YsQ0FBQztDQUNEO0FBaEJELDRCQWdCQztBQUVELE1BQWEsS0FBTSxTQUFRLE9BQU87SUFHakMsWUFBbUIsS0FBYSxFQUFFLElBQVU7UUFDM0MsS0FBSyxDQUFDLE9BQU8sQ0FBQyxDQUFDO1FBQ2YsSUFBSSxDQUFDLEtBQUssR0FBRyxLQUFLLENBQUM7UUFDbkIsSUFBSSxJQUFJLEVBQUU7WUFDSCxJQUFLLENBQUMsSUFBSSxHQUFHLElBQUksQ0FBQztTQUN4QjtJQUNGLENBQUM7Q0FDRDtBQVZELHNCQVVDIiwic291cmNlc0NvbnRlbnQiOlsiLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cbiAqICBDb3B5cmlnaHQgKGMpIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbi4gQWxsIHJpZ2h0cyByZXNlcnZlZC5cbiAqICBMaWNlbnNlZCB1bmRlciB0aGUgTUlUIExpY2Vuc2UuIFNlZSBMaWNlbnNlLnR4dCBpbiB0aGUgcHJvamVjdCByb290IGZvciBsaWNlbnNlIGluZm9ybWF0aW9uLlxuICotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbmltcG9ydCB7IERlYnVnUHJvdG9jb2wgfSBmcm9tICd2c2NvZGUtZGVidWdwcm90b2NvbCc7XG5cblxuZXhwb3J0IGNsYXNzIE1lc3NhZ2UgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLlByb3RvY29sTWVzc2FnZSB7XG5cdHNlcTogbnVtYmVyO1xuXHR0eXBlOiBzdHJpbmc7XG5cblx0cHVibGljIGNvbnN0cnVjdG9yKHR5cGU6IHN0cmluZykge1xuXHRcdHRoaXMuc2VxID0gMDtcblx0XHR0aGlzLnR5cGUgPSB0eXBlO1xuXHR9XG59XG5cbmV4cG9ydCBjbGFzcyBSZXNwb25zZSBleHRlbmRzIE1lc3NhZ2UgaW1wbGVtZW50cyBEZWJ1Z1Byb3RvY29sLlJlc3BvbnNlIHtcblx0cmVxdWVzdF9zZXE6IG51bWJlcjtcblx0c3VjY2VzczogYm9vbGVhbjtcblx0Y29tbWFuZDogc3RyaW5nO1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihyZXF1ZXN0OiBEZWJ1Z1Byb3RvY29sLlJlcXVlc3QsIG1lc3NhZ2U/OiBzdHJpbmcpIHtcblx0XHRzdXBlcigncmVzcG9uc2UnKTtcblx0XHR0aGlzLnJlcXVlc3Rfc2VxID0gcmVxdWVzdC5zZXE7XG5cdFx0dGhpcy5jb21tYW5kID0gcmVxdWVzdC5jb21tYW5kO1xuXHRcdGlmIChtZXNzYWdlKSB7XG5cdFx0XHR0aGlzLnN1Y2Nlc3MgPSBmYWxzZTtcblx0XHRcdCg8YW55PnRoaXMpLm1lc3NhZ2UgPSBtZXNzYWdlO1xuXHRcdH0gZWxzZSB7XG5cdFx0XHR0aGlzLnN1Y2Nlc3MgPSB0cnVlO1xuXHRcdH1cblx0fVxufVxuXG5leHBvcnQgY2xhc3MgRXZlbnQgZXh0ZW5kcyBNZXNzYWdlIGltcGxlbWVudHMgRGVidWdQcm90b2NvbC5FdmVudCB7XG5cdGV2ZW50OiBzdHJpbmc7XG5cblx0cHVibGljIGNvbnN0cnVjdG9yKGV2ZW50OiBzdHJpbmcsIGJvZHk/OiBhbnkpIHtcblx0XHRzdXBlcignZXZlbnQnKTtcblx0XHR0aGlzLmV2ZW50ID0gZXZlbnQ7XG5cdFx0aWYgKGJvZHkpIHtcblx0XHRcdCg8YW55PnRoaXMpLmJvZHkgPSBib2R5O1xuXHRcdH1cblx0fVxufVxuIl19

/***/ }),
/* 10 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.runDebugAdapter = void 0;
const Net = __webpack_require__(11);
function runDebugAdapter(debugSession) {
    // parse arguments
    let port = 0;
    const args = process.argv.slice(2);
    args.forEach(function (val, index, array) {
        const portMatch = /^--server=(\d{4,5})$/.exec(val);
        if (portMatch) {
            port = parseInt(portMatch[1], 10);
        }
    });
    if (port > 0) {
        // start as a server
        console.error(`waiting for debug protocol on port ${port}`);
        Net.createServer((socket) => {
            console.error('>> accepted connection from client');
            socket.on('end', () => {
                console.error('>> client connection closed\n');
            });
            const session = new debugSession(false, true);
            session.setRunAsServer(true);
            session.start(socket, socket);
        }).listen(port);
    }
    else {
        // start a session
        //console.error('waiting for debug protocol on stdin/stdout');
        const session = new debugSession(false);
        process.on('SIGTERM', () => {
            session.shutdown();
        });
        session.start(process.stdin, process.stdout);
    }
}
exports.runDebugAdapter = runDebugAdapter;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoicnVuRGVidWdBZGFwdGVyLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXMiOlsiLi4vc3JjL3J1bkRlYnVnQWRhcHRlci50cyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiO0FBQUE7OztnR0FHZ0c7OztBQUVoRywyQkFBMkI7QUFJM0IsU0FBZ0IsZUFBZSxDQUFDLFlBQWlDO0lBRWhFLGtCQUFrQjtJQUNsQixJQUFJLElBQUksR0FBRyxDQUFDLENBQUM7SUFDYixNQUFNLElBQUksR0FBRyxPQUFPLENBQUMsSUFBSSxDQUFDLEtBQUssQ0FBQyxDQUFDLENBQUMsQ0FBQztJQUNuQyxJQUFJLENBQUMsT0FBTyxDQUFDLFVBQVUsR0FBRyxFQUFFLEtBQUssRUFBRSxLQUFLO1FBQ3ZDLE1BQU0sU0FBUyxHQUFHLHNCQUFzQixDQUFDLElBQUksQ0FBQyxHQUFHLENBQUMsQ0FBQztRQUNuRCxJQUFJLFNBQVMsRUFBRTtZQUNkLElBQUksR0FBRyxRQUFRLENBQUMsU0FBUyxDQUFDLENBQUMsQ0FBQyxFQUFFLEVBQUUsQ0FBQyxDQUFDO1NBQ2xDO0lBQ0YsQ0FBQyxDQUFDLENBQUM7SUFFSCxJQUFJLElBQUksR0FBRyxDQUFDLEVBQUU7UUFDYixvQkFBb0I7UUFDcEIsT0FBTyxDQUFDLEtBQUssQ0FBQyxzQ0FBc0MsSUFBSSxFQUFFLENBQUMsQ0FBQztRQUM1RCxHQUFHLENBQUMsWUFBWSxDQUFDLENBQUMsTUFBTSxFQUFFLEVBQUU7WUFDM0IsT0FBTyxDQUFDLEtBQUssQ0FBQyxvQ0FBb0MsQ0FBQyxDQUFDO1lBQ3BELE1BQU0sQ0FBQyxFQUFFLENBQUMsS0FBSyxFQUFFLEdBQUcsRUFBRTtnQkFDckIsT0FBTyxDQUFDLEtBQUssQ0FBQywrQkFBK0IsQ0FBQyxDQUFDO1lBQ2hELENBQUMsQ0FBQyxDQUFDO1lBQ0gsTUFBTSxPQUFPLEdBQUcsSUFBSSxZQUFZLENBQUMsS0FBSyxFQUFFLElBQUksQ0FBQyxDQUFDO1lBQzlDLE9BQU8sQ0FBQyxjQUFjLENBQUMsSUFBSSxDQUFDLENBQUM7WUFDN0IsT0FBTyxDQUFDLEtBQUssQ0FBQyxNQUFNLEVBQUUsTUFBTSxDQUFDLENBQUM7UUFDL0IsQ0FBQyxDQUFDLENBQUMsTUFBTSxDQUFDLElBQUksQ0FBQyxDQUFDO0tBQ2hCO1NBQU07UUFFTixrQkFBa0I7UUFDbEIsOERBQThEO1FBQzlELE1BQU0sT0FBTyxHQUFHLElBQUksWUFBWSxDQUFDLEtBQUssQ0FBQyxDQUFDO1FBQ3hDLE9BQU8sQ0FBQyxFQUFFLENBQUMsU0FBUyxFQUFFLEdBQUcsRUFBRTtZQUMxQixPQUFPLENBQUMsUUFBUSxFQUFFLENBQUM7UUFDcEIsQ0FBQyxDQUFDLENBQUM7UUFDSCxPQUFPLENBQUMsS0FBSyxDQUFDLE9BQU8sQ0FBQyxLQUFLLEVBQUUsT0FBTyxDQUFDLE1BQU0sQ0FBQyxDQUFDO0tBQzdDO0FBQ0YsQ0FBQztBQWxDRCwwQ0FrQ0MiLCJzb3VyY2VzQ29udGVudCI6WyIvKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLVxuICogIENvcHlyaWdodCAoYykgTWljcm9zb2Z0IENvcnBvcmF0aW9uLiBBbGwgcmlnaHRzIHJlc2VydmVkLlxuICogIExpY2Vuc2VkIHVuZGVyIHRoZSBNSVQgTGljZW5zZS4gU2VlIExpY2Vuc2UudHh0IGluIHRoZSBwcm9qZWN0IHJvb3QgZm9yIGxpY2Vuc2UgaW5mb3JtYXRpb24uXG4gKi0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tKi9cblxuaW1wb3J0ICogYXMgTmV0IGZyb20gJ25ldCc7XG5cbmltcG9ydCB7IERlYnVnU2Vzc2lvbiB9IGZyb20gJy4vZGVidWdTZXNzaW9uJztcblxuZXhwb3J0IGZ1bmN0aW9uIHJ1bkRlYnVnQWRhcHRlcihkZWJ1Z1Nlc3Npb246IHR5cGVvZiBEZWJ1Z1Nlc3Npb24pIHtcblxuXHQvLyBwYXJzZSBhcmd1bWVudHNcblx0bGV0IHBvcnQgPSAwO1xuXHRjb25zdCBhcmdzID0gcHJvY2Vzcy5hcmd2LnNsaWNlKDIpO1xuXHRhcmdzLmZvckVhY2goZnVuY3Rpb24gKHZhbCwgaW5kZXgsIGFycmF5KSB7XG5cdFx0Y29uc3QgcG9ydE1hdGNoID0gL14tLXNlcnZlcj0oXFxkezQsNX0pJC8uZXhlYyh2YWwpO1xuXHRcdGlmIChwb3J0TWF0Y2gpIHtcblx0XHRcdHBvcnQgPSBwYXJzZUludChwb3J0TWF0Y2hbMV0sIDEwKTtcblx0XHR9XG5cdH0pO1xuXG5cdGlmIChwb3J0ID4gMCkge1xuXHRcdC8vIHN0YXJ0IGFzIGEgc2VydmVyXG5cdFx0Y29uc29sZS5lcnJvcihgd2FpdGluZyBmb3IgZGVidWcgcHJvdG9jb2wgb24gcG9ydCAke3BvcnR9YCk7XG5cdFx0TmV0LmNyZWF0ZVNlcnZlcigoc29ja2V0KSA9PiB7XG5cdFx0XHRjb25zb2xlLmVycm9yKCc+PiBhY2NlcHRlZCBjb25uZWN0aW9uIGZyb20gY2xpZW50Jyk7XG5cdFx0XHRzb2NrZXQub24oJ2VuZCcsICgpID0+IHtcblx0XHRcdFx0Y29uc29sZS5lcnJvcignPj4gY2xpZW50IGNvbm5lY3Rpb24gY2xvc2VkXFxuJyk7XG5cdFx0XHR9KTtcblx0XHRcdGNvbnN0IHNlc3Npb24gPSBuZXcgZGVidWdTZXNzaW9uKGZhbHNlLCB0cnVlKTtcblx0XHRcdHNlc3Npb24uc2V0UnVuQXNTZXJ2ZXIodHJ1ZSk7XG5cdFx0XHRzZXNzaW9uLnN0YXJ0KHNvY2tldCwgc29ja2V0KTtcblx0XHR9KS5saXN0ZW4ocG9ydCk7XG5cdH0gZWxzZSB7XG5cblx0XHQvLyBzdGFydCBhIHNlc3Npb25cblx0XHQvL2NvbnNvbGUuZXJyb3IoJ3dhaXRpbmcgZm9yIGRlYnVnIHByb3RvY29sIG9uIHN0ZGluL3N0ZG91dCcpO1xuXHRcdGNvbnN0IHNlc3Npb24gPSBuZXcgZGVidWdTZXNzaW9uKGZhbHNlKTtcblx0XHRwcm9jZXNzLm9uKCdTSUdURVJNJywgKCkgPT4ge1xuXHRcdFx0c2Vzc2lvbi5zaHV0ZG93bigpO1xuXHRcdH0pO1xuXHRcdHNlc3Npb24uc3RhcnQocHJvY2Vzcy5zdGRpbiwgcHJvY2Vzcy5zdGRvdXQpO1xuXHR9XG59XG4iXX0=

/***/ }),
/* 11 */
/***/ ((module) => {

"use strict";
module.exports = require("net");

/***/ }),
/* 12 */
/***/ ((module) => {

"use strict";
module.exports = require("url");

/***/ }),
/* 13 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.LoggingDebugSession = void 0;
const Logger = __webpack_require__(14);
const logger = Logger.logger;
const debugSession_1 = __webpack_require__(6);
class LoggingDebugSession extends debugSession_1.DebugSession {
    constructor(obsolete_logFilePath, obsolete_debuggerLinesAndColumnsStartAt1, obsolete_isServer) {
        super(obsolete_debuggerLinesAndColumnsStartAt1, obsolete_isServer);
        this.obsolete_logFilePath = obsolete_logFilePath;
        this.on('error', (event) => {
            logger.error(event.body);
        });
    }
    start(inStream, outStream) {
        super.start(inStream, outStream);
        logger.init(e => this.sendEvent(e), this.obsolete_logFilePath, this._isServer);
    }
    /**
     * Overload sendEvent to log
     */
    sendEvent(event) {
        if (!(event instanceof Logger.LogOutputEvent)) {
            // Don't create an infinite loop...
            let objectToLog = event;
            if (event instanceof debugSession_1.OutputEvent && event.body && event.body.data && event.body.data.doNotLogOutput) {
                delete event.body.data.doNotLogOutput;
                objectToLog = Object.assign({}, event);
                objectToLog.body = Object.assign(Object.assign({}, event.body), { output: '<output not logged>' });
            }
            logger.verbose(`To client: ${JSON.stringify(objectToLog)}`);
        }
        super.sendEvent(event);
    }
    /**
     * Overload sendRequest to log
     */
    sendRequest(command, args, timeout, cb) {
        logger.verbose(`To client: ${JSON.stringify(command)}(${JSON.stringify(args)}), timeout: ${timeout}`);
        super.sendRequest(command, args, timeout, cb);
    }
    /**
     * Overload sendResponse to log
     */
    sendResponse(response) {
        logger.verbose(`To client: ${JSON.stringify(response)}`);
        super.sendResponse(response);
    }
    dispatchRequest(request) {
        logger.verbose(`From client: ${request.command}(${JSON.stringify(request.arguments)})`);
        super.dispatchRequest(request);
    }
}
exports.LoggingDebugSession = LoggingDebugSession;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoibG9nZ2luZ0RlYnVnU2Vzc2lvbi5qcyIsInNvdXJjZVJvb3QiOiIiLCJzb3VyY2VzIjpbIi4uL3NyYy9sb2dnaW5nRGVidWdTZXNzaW9uLnRzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiI7QUFBQTs7O2dHQUdnRzs7O0FBSWhHLG1DQUFtQztBQUNuQyxNQUFNLE1BQU0sR0FBRyxNQUFNLENBQUMsTUFBTSxDQUFDO0FBQzdCLGlEQUF5RDtBQUV6RCxNQUFhLG1CQUFvQixTQUFRLDJCQUFZO0lBQ3BELFlBQTJCLG9CQUE2QixFQUFFLHdDQUFrRCxFQUFFLGlCQUEyQjtRQUN4SSxLQUFLLENBQUMsd0NBQXdDLEVBQUUsaUJBQWlCLENBQUMsQ0FBQztRQUR6Qyx5QkFBb0IsR0FBcEIsb0JBQW9CLENBQVM7UUFHdkQsSUFBSSxDQUFDLEVBQUUsQ0FBQyxPQUFPLEVBQUUsQ0FBQyxLQUEwQixFQUFFLEVBQUU7WUFDL0MsTUFBTSxDQUFDLEtBQUssQ0FBQyxLQUFLLENBQUMsSUFBSSxDQUFDLENBQUM7UUFDMUIsQ0FBQyxDQUFDLENBQUM7SUFDSixDQUFDO0lBRU0sS0FBSyxDQUFDLFFBQStCLEVBQUUsU0FBZ0M7UUFDN0UsS0FBSyxDQUFDLEtBQUssQ0FBQyxRQUFRLEVBQUUsU0FBUyxDQUFDLENBQUM7UUFDakMsTUFBTSxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsRUFBRSxDQUFDLElBQUksQ0FBQyxTQUFTLENBQUMsQ0FBQyxDQUFDLEVBQUUsSUFBSSxDQUFDLG9CQUFvQixFQUFFLElBQUksQ0FBQyxTQUFTLENBQUMsQ0FBQztJQUNoRixDQUFDO0lBRUQ7O09BRUc7SUFDSSxTQUFTLENBQUMsS0FBMEI7UUFDMUMsSUFBSSxDQUFDLENBQUMsS0FBSyxZQUFZLE1BQU0sQ0FBQyxjQUFjLENBQUMsRUFBRTtZQUM5QyxtQ0FBbUM7WUFFbkMsSUFBSSxXQUFXLEdBQUcsS0FBSyxDQUFDO1lBQ3hCLElBQUksS0FBSyxZQUFZLDBCQUFXLElBQUksS0FBSyxDQUFDLElBQUksSUFBSSxLQUFLLENBQUMsSUFBSSxDQUFDLElBQUksSUFBSSxLQUFLLENBQUMsSUFBSSxDQUFDLElBQUksQ0FBQyxjQUFjLEVBQUU7Z0JBQ3BHLE9BQU8sS0FBSyxDQUFDLElBQUksQ0FBQyxJQUFJLENBQUMsY0FBYyxDQUFDO2dCQUN0QyxXQUFXLHFCQUFRLEtBQUssQ0FBRSxDQUFDO2dCQUMzQixXQUFXLENBQUMsSUFBSSxtQ0FBUSxLQUFLLENBQUMsSUFBSSxLQUFFLE1BQU0sRUFBRSxxQkFBcUIsR0FBRSxDQUFBO2FBQ25FO1lBRUQsTUFBTSxDQUFDLE9BQU8sQ0FBQyxjQUFjLElBQUksQ0FBQyxTQUFTLENBQUMsV0FBVyxDQUFDLEVBQUUsQ0FBQyxDQUFDO1NBQzVEO1FBRUQsS0FBSyxDQUFDLFNBQVMsQ0FBQyxLQUFLLENBQUMsQ0FBQztJQUN4QixDQUFDO0lBRUQ7O09BRUc7SUFDSSxXQUFXLENBQUMsT0FBZSxFQUFFLElBQVMsRUFBRSxPQUFlLEVBQUUsRUFBOEM7UUFDN0csTUFBTSxDQUFDLE9BQU8sQ0FBQyxjQUFjLElBQUksQ0FBQyxTQUFTLENBQUMsT0FBTyxDQUFDLElBQUksSUFBSSxDQUFDLFNBQVMsQ0FBQyxJQUFJLENBQUMsZUFBZSxPQUFPLEVBQUUsQ0FBQyxDQUFDO1FBQ3RHLEtBQUssQ0FBQyxXQUFXLENBQUMsT0FBTyxFQUFFLElBQUksRUFBRSxPQUFPLEVBQUUsRUFBRSxDQUFDLENBQUM7SUFDL0MsQ0FBQztJQUVEOztPQUVHO0lBQ0ksWUFBWSxDQUFDLFFBQWdDO1FBQ25ELE1BQU0sQ0FBQyxPQUFPLENBQUMsY0FBYyxJQUFJLENBQUMsU0FBUyxDQUFDLFFBQVEsQ0FBQyxFQUFFLENBQUMsQ0FBQztRQUN6RCxLQUFLLENBQUMsWUFBWSxDQUFDLFFBQVEsQ0FBQyxDQUFDO0lBQzlCLENBQUM7SUFFUyxlQUFlLENBQUMsT0FBOEI7UUFDdkQsTUFBTSxDQUFDLE9BQU8sQ0FBQyxnQkFBZ0IsT0FBTyxDQUFDLE9BQU8sSUFBSSxJQUFJLENBQUMsU0FBUyxDQUFDLE9BQU8sQ0FBQyxTQUFTLENBQUUsR0FBRyxDQUFDLENBQUM7UUFDekYsS0FBSyxDQUFDLGVBQWUsQ0FBQyxPQUFPLENBQUMsQ0FBQztJQUNoQyxDQUFDO0NBQ0Q7QUF0REQsa0RBc0RDIiwic291cmNlc0NvbnRlbnQiOlsiLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cbiAqICBDb3B5cmlnaHQgKGMpIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbi4gQWxsIHJpZ2h0cyByZXNlcnZlZC5cbiAqICBMaWNlbnNlZCB1bmRlciB0aGUgTUlUIExpY2Vuc2UuIFNlZSBMaWNlbnNlLnR4dCBpbiB0aGUgcHJvamVjdCByb290IGZvciBsaWNlbnNlIGluZm9ybWF0aW9uLlxuICotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbmltcG9ydCB7RGVidWdQcm90b2NvbH0gZnJvbSAndnNjb2RlLWRlYnVncHJvdG9jb2wnO1xuXG5pbXBvcnQgKiBhcyBMb2dnZXIgZnJvbSAnLi9sb2dnZXInO1xuY29uc3QgbG9nZ2VyID0gTG9nZ2VyLmxvZ2dlcjtcbmltcG9ydCB7RGVidWdTZXNzaW9uLCBPdXRwdXRFdmVudH0gZnJvbSAnLi9kZWJ1Z1Nlc3Npb24nO1xuXG5leHBvcnQgY2xhc3MgTG9nZ2luZ0RlYnVnU2Vzc2lvbiBleHRlbmRzIERlYnVnU2Vzc2lvbiB7XG5cdHB1YmxpYyBjb25zdHJ1Y3Rvcihwcml2YXRlIG9ic29sZXRlX2xvZ0ZpbGVQYXRoPzogc3RyaW5nLCBvYnNvbGV0ZV9kZWJ1Z2dlckxpbmVzQW5kQ29sdW1uc1N0YXJ0QXQxPzogYm9vbGVhbiwgb2Jzb2xldGVfaXNTZXJ2ZXI/OiBib29sZWFuKSB7XG5cdFx0c3VwZXIob2Jzb2xldGVfZGVidWdnZXJMaW5lc0FuZENvbHVtbnNTdGFydEF0MSwgb2Jzb2xldGVfaXNTZXJ2ZXIpO1xuXG5cdFx0dGhpcy5vbignZXJyb3InLCAoZXZlbnQ6IERlYnVnUHJvdG9jb2wuRXZlbnQpID0+IHtcblx0XHRcdGxvZ2dlci5lcnJvcihldmVudC5ib2R5KTtcblx0XHR9KTtcblx0fVxuXG5cdHB1YmxpYyBzdGFydChpblN0cmVhbTogTm9kZUpTLlJlYWRhYmxlU3RyZWFtLCBvdXRTdHJlYW06IE5vZGVKUy5Xcml0YWJsZVN0cmVhbSk6IHZvaWQge1xuXHRcdHN1cGVyLnN0YXJ0KGluU3RyZWFtLCBvdXRTdHJlYW0pO1xuXHRcdGxvZ2dlci5pbml0KGUgPT4gdGhpcy5zZW5kRXZlbnQoZSksIHRoaXMub2Jzb2xldGVfbG9nRmlsZVBhdGgsIHRoaXMuX2lzU2VydmVyKTtcblx0fVxuXG5cdC8qKlxuXHQgKiBPdmVybG9hZCBzZW5kRXZlbnQgdG8gbG9nXG5cdCAqL1xuXHRwdWJsaWMgc2VuZEV2ZW50KGV2ZW50OiBEZWJ1Z1Byb3RvY29sLkV2ZW50KTogdm9pZCB7XG5cdFx0aWYgKCEoZXZlbnQgaW5zdGFuY2VvZiBMb2dnZXIuTG9nT3V0cHV0RXZlbnQpKSB7XG5cdFx0XHQvLyBEb24ndCBjcmVhdGUgYW4gaW5maW5pdGUgbG9vcC4uLlxuXG5cdFx0XHRsZXQgb2JqZWN0VG9Mb2cgPSBldmVudDtcblx0XHRcdGlmIChldmVudCBpbnN0YW5jZW9mIE91dHB1dEV2ZW50ICYmIGV2ZW50LmJvZHkgJiYgZXZlbnQuYm9keS5kYXRhICYmIGV2ZW50LmJvZHkuZGF0YS5kb05vdExvZ091dHB1dCkge1xuXHRcdFx0XHRkZWxldGUgZXZlbnQuYm9keS5kYXRhLmRvTm90TG9nT3V0cHV0O1xuXHRcdFx0XHRvYmplY3RUb0xvZyA9IHsgLi4uZXZlbnQgfTtcblx0XHRcdFx0b2JqZWN0VG9Mb2cuYm9keSA9IHsgLi4uZXZlbnQuYm9keSwgb3V0cHV0OiAnPG91dHB1dCBub3QgbG9nZ2VkPicgfVxuXHRcdFx0fVxuXG5cdFx0XHRsb2dnZXIudmVyYm9zZShgVG8gY2xpZW50OiAke0pTT04uc3RyaW5naWZ5KG9iamVjdFRvTG9nKX1gKTtcblx0XHR9XG5cblx0XHRzdXBlci5zZW5kRXZlbnQoZXZlbnQpO1xuXHR9XG5cblx0LyoqXG5cdCAqIE92ZXJsb2FkIHNlbmRSZXF1ZXN0IHRvIGxvZ1xuXHQgKi9cblx0cHVibGljIHNlbmRSZXF1ZXN0KGNvbW1hbmQ6IHN0cmluZywgYXJnczogYW55LCB0aW1lb3V0OiBudW1iZXIsIGNiOiAocmVzcG9uc2U6IERlYnVnUHJvdG9jb2wuUmVzcG9uc2UpID0+IHZvaWQpOiB2b2lkIHtcblx0XHRsb2dnZXIudmVyYm9zZShgVG8gY2xpZW50OiAke0pTT04uc3RyaW5naWZ5KGNvbW1hbmQpfSgke0pTT04uc3RyaW5naWZ5KGFyZ3MpfSksIHRpbWVvdXQ6ICR7dGltZW91dH1gKTtcblx0XHRzdXBlci5zZW5kUmVxdWVzdChjb21tYW5kLCBhcmdzLCB0aW1lb3V0LCBjYik7XG5cdH1cblxuXHQvKipcblx0ICogT3ZlcmxvYWQgc2VuZFJlc3BvbnNlIHRvIGxvZ1xuXHQgKi9cblx0cHVibGljIHNlbmRSZXNwb25zZShyZXNwb25zZTogRGVidWdQcm90b2NvbC5SZXNwb25zZSk6IHZvaWQge1xuXHRcdGxvZ2dlci52ZXJib3NlKGBUbyBjbGllbnQ6ICR7SlNPTi5zdHJpbmdpZnkocmVzcG9uc2UpfWApO1xuXHRcdHN1cGVyLnNlbmRSZXNwb25zZShyZXNwb25zZSk7XG5cdH1cblxuXHRwcm90ZWN0ZWQgZGlzcGF0Y2hSZXF1ZXN0KHJlcXVlc3Q6IERlYnVnUHJvdG9jb2wuUmVxdWVzdCk6IHZvaWQge1xuXHRcdGxvZ2dlci52ZXJib3NlKGBGcm9tIGNsaWVudDogJHtyZXF1ZXN0LmNvbW1hbmR9KCR7SlNPTi5zdHJpbmdpZnkocmVxdWVzdC5hcmd1bWVudHMpIH0pYCk7XG5cdFx0c3VwZXIuZGlzcGF0Y2hSZXF1ZXN0KHJlcXVlc3QpO1xuXHR9XG59XG4iXX0=

/***/ }),
/* 14 */
/***/ ((__unused_webpack_module, exports, __webpack_require__) => {

"use strict";

/*---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.trimLastNewline = exports.LogOutputEvent = exports.logger = exports.Logger = exports.LogLevel = void 0;
const internalLogger_1 = __webpack_require__(15);
const debugSession_1 = __webpack_require__(6);
var LogLevel;
(function (LogLevel) {
    LogLevel[LogLevel["Verbose"] = 0] = "Verbose";
    LogLevel[LogLevel["Log"] = 1] = "Log";
    LogLevel[LogLevel["Warn"] = 2] = "Warn";
    LogLevel[LogLevel["Error"] = 3] = "Error";
    LogLevel[LogLevel["Stop"] = 4] = "Stop";
})(LogLevel = exports.LogLevel || (exports.LogLevel = {}));
class Logger {
    constructor() {
        this._pendingLogQ = [];
    }
    log(msg, level = LogLevel.Log) {
        msg = msg + '\n';
        this._write(msg, level);
    }
    verbose(msg) {
        this.log(msg, LogLevel.Verbose);
    }
    warn(msg) {
        this.log(msg, LogLevel.Warn);
    }
    error(msg) {
        this.log(msg, LogLevel.Error);
    }
    dispose() {
        if (this._currentLogger) {
            const disposeP = this._currentLogger.dispose();
            this._currentLogger = null;
            return disposeP;
        }
        else {
            return Promise.resolve();
        }
    }
    /**
     * `log` adds a newline, `write` doesn't
     */
    _write(msg, level = LogLevel.Log) {
        // [null, undefined] => string
        msg = msg + '';
        if (this._pendingLogQ) {
            this._pendingLogQ.push({ msg, level });
        }
        else if (this._currentLogger) {
            this._currentLogger.log(msg, level);
        }
    }
    /**
     * Set the logger's minimum level to log in the console, and whether to log to the file. Log messages are queued before this is
     * called the first time, because minLogLevel defaults to Warn.
     */
    setup(consoleMinLogLevel, _logFilePath, prependTimestamp = true) {
        const logFilePath = typeof _logFilePath === 'string' ?
            _logFilePath :
            (_logFilePath && this._logFilePathFromInit);
        if (this._currentLogger) {
            const options = {
                consoleMinLogLevel,
                logFilePath,
                prependTimestamp
            };
            this._currentLogger.setup(options).then(() => {
                // Now that we have a minimum logLevel, we can clear out the queue of pending messages
                if (this._pendingLogQ) {
                    const logQ = this._pendingLogQ;
                    this._pendingLogQ = null;
                    logQ.forEach(item => this._write(item.msg, item.level));
                }
            });
        }
    }
    init(logCallback, logFilePath, logToConsole) {
        // Re-init, create new global Logger
        this._pendingLogQ = this._pendingLogQ || [];
        this._currentLogger = new internalLogger_1.InternalLogger(logCallback, logToConsole);
        this._logFilePathFromInit = logFilePath;
    }
}
exports.Logger = Logger;
exports.logger = new Logger();
class LogOutputEvent extends debugSession_1.OutputEvent {
    constructor(msg, level) {
        const category = level === LogLevel.Error ? 'stderr' :
            level === LogLevel.Warn ? 'console' :
                'stdout';
        super(msg, category);
    }
}
exports.LogOutputEvent = LogOutputEvent;
function trimLastNewline(str) {
    return str.replace(/(\n|\r\n)$/, '');
}
exports.trimLastNewline = trimLastNewline;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoibG9nZ2VyLmpzIiwic291cmNlUm9vdCI6IiIsInNvdXJjZXMiOlsiLi4vc3JjL2xvZ2dlci50cyJdLCJuYW1lcyI6W10sIm1hcHBpbmdzIjoiO0FBQUE7OzREQUU0RDs7O0FBRTVELHFEQUFrRDtBQUNsRCxpREFBNkM7QUFFN0MsSUFBWSxRQU1YO0FBTkQsV0FBWSxRQUFRO0lBQ25CLDZDQUFXLENBQUE7SUFDWCxxQ0FBTyxDQUFBO0lBQ1AsdUNBQVEsQ0FBQTtJQUNSLHlDQUFTLENBQUE7SUFDVCx1Q0FBUSxDQUFBO0FBQ1QsQ0FBQyxFQU5XLFFBQVEsR0FBUixnQkFBUSxLQUFSLGdCQUFRLFFBTW5CO0FBNEJELE1BQWEsTUFBTTtJQUFuQjtRQUlTLGlCQUFZLEdBQWUsRUFBRSxDQUFDO0lBMkV2QyxDQUFDO0lBekVBLEdBQUcsQ0FBQyxHQUFXLEVBQUUsS0FBSyxHQUFHLFFBQVEsQ0FBQyxHQUFHO1FBQ3BDLEdBQUcsR0FBRyxHQUFHLEdBQUcsSUFBSSxDQUFDO1FBQ2pCLElBQUksQ0FBQyxNQUFNLENBQUMsR0FBRyxFQUFFLEtBQUssQ0FBQyxDQUFDO0lBQ3pCLENBQUM7SUFFRCxPQUFPLENBQUMsR0FBVztRQUNsQixJQUFJLENBQUMsR0FBRyxDQUFDLEdBQUcsRUFBRSxRQUFRLENBQUMsT0FBTyxDQUFDLENBQUM7SUFDakMsQ0FBQztJQUVELElBQUksQ0FBQyxHQUFXO1FBQ2YsSUFBSSxDQUFDLEdBQUcsQ0FBQyxHQUFHLEVBQUUsUUFBUSxDQUFDLElBQUksQ0FBQyxDQUFDO0lBQzlCLENBQUM7SUFFRCxLQUFLLENBQUMsR0FBVztRQUNoQixJQUFJLENBQUMsR0FBRyxDQUFDLEdBQUcsRUFBRSxRQUFRLENBQUMsS0FBSyxDQUFDLENBQUM7SUFDL0IsQ0FBQztJQUVELE9BQU87UUFDTixJQUFJLElBQUksQ0FBQyxjQUFjLEVBQUU7WUFDeEIsTUFBTSxRQUFRLEdBQUcsSUFBSSxDQUFDLGNBQWMsQ0FBQyxPQUFPLEVBQUUsQ0FBQztZQUMvQyxJQUFJLENBQUMsY0FBYyxHQUFHLElBQUksQ0FBQztZQUMzQixPQUFPLFFBQVEsQ0FBQztTQUNoQjthQUFNO1lBQ04sT0FBTyxPQUFPLENBQUMsT0FBTyxFQUFFLENBQUM7U0FDekI7SUFDRixDQUFDO0lBRUQ7O09BRUc7SUFDSyxNQUFNLENBQUMsR0FBVyxFQUFFLEtBQUssR0FBRyxRQUFRLENBQUMsR0FBRztRQUMvQyw4QkFBOEI7UUFDOUIsR0FBRyxHQUFHLEdBQUcsR0FBRyxFQUFFLENBQUM7UUFDZixJQUFJLElBQUksQ0FBQyxZQUFZLEVBQUU7WUFDdEIsSUFBSSxDQUFDLFlBQVksQ0FBQyxJQUFJLENBQUMsRUFBRSxHQUFHLEVBQUUsS0FBSyxFQUFFLENBQUMsQ0FBQztTQUN2QzthQUFNLElBQUksSUFBSSxDQUFDLGNBQWMsRUFBRTtZQUMvQixJQUFJLENBQUMsY0FBYyxDQUFDLEdBQUcsQ0FBQyxHQUFHLEVBQUUsS0FBSyxDQUFDLENBQUM7U0FDcEM7SUFDRixDQUFDO0lBRUQ7OztPQUdHO0lBQ0gsS0FBSyxDQUFDLGtCQUE0QixFQUFFLFlBQTZCLEVBQUUsbUJBQTRCLElBQUk7UUFDbEcsTUFBTSxXQUFXLEdBQUcsT0FBTyxZQUFZLEtBQUssUUFBUSxDQUFDLENBQUM7WUFDckQsWUFBWSxDQUFDLENBQUM7WUFDZCxDQUFDLFlBQVksSUFBSSxJQUFJLENBQUMsb0JBQW9CLENBQUMsQ0FBQztRQUU3QyxJQUFJLElBQUksQ0FBQyxjQUFjLEVBQUU7WUFDeEIsTUFBTSxPQUFPLEdBQUc7Z0JBQ2Ysa0JBQWtCO2dCQUNsQixXQUFXO2dCQUNYLGdCQUFnQjthQUNoQixDQUFDO1lBQ0YsSUFBSSxDQUFDLGNBQWMsQ0FBQyxLQUFLLENBQUMsT0FBTyxDQUFDLENBQUMsSUFBSSxDQUFDLEdBQUcsRUFBRTtnQkFDNUMsc0ZBQXNGO2dCQUN0RixJQUFJLElBQUksQ0FBQyxZQUFZLEVBQUU7b0JBQ3RCLE1BQU0sSUFBSSxHQUFHLElBQUksQ0FBQyxZQUFZLENBQUM7b0JBQy9CLElBQUksQ0FBQyxZQUFZLEdBQUcsSUFBSSxDQUFDO29CQUN6QixJQUFJLENBQUMsT0FBTyxDQUFDLElBQUksQ0FBQyxFQUFFLENBQUMsSUFBSSxDQUFDLE1BQU0sQ0FBQyxJQUFJLENBQUMsR0FBRyxFQUFFLElBQUksQ0FBQyxLQUFLLENBQUMsQ0FBQyxDQUFDO2lCQUN4RDtZQUNGLENBQUMsQ0FBQyxDQUFDO1NBRUg7SUFDRixDQUFDO0lBRUQsSUFBSSxDQUFDLFdBQXlCLEVBQUUsV0FBb0IsRUFBRSxZQUFzQjtRQUMzRSxvQ0FBb0M7UUFDcEMsSUFBSSxDQUFDLFlBQVksR0FBRyxJQUFJLENBQUMsWUFBWSxJQUFJLEVBQUUsQ0FBQztRQUM1QyxJQUFJLENBQUMsY0FBYyxHQUFHLElBQUksK0JBQWMsQ0FBQyxXQUFXLEVBQUUsWUFBWSxDQUFDLENBQUM7UUFDcEUsSUFBSSxDQUFDLG9CQUFvQixHQUFHLFdBQVcsQ0FBQztJQUN6QyxDQUFDO0NBQ0Q7QUEvRUQsd0JBK0VDO0FBRVksUUFBQSxNQUFNLEdBQUcsSUFBSSxNQUFNLEVBQUUsQ0FBQztBQUVuQyxNQUFhLGNBQWUsU0FBUSwwQkFBVztJQUM5QyxZQUFZLEdBQVcsRUFBRSxLQUFlO1FBQ3ZDLE1BQU0sUUFBUSxHQUNiLEtBQUssS0FBSyxRQUFRLENBQUMsS0FBSyxDQUFDLENBQUMsQ0FBQyxRQUFRLENBQUMsQ0FBQztZQUNyQyxLQUFLLEtBQUssUUFBUSxDQUFDLElBQUksQ0FBQyxDQUFDLENBQUMsU0FBUyxDQUFDLENBQUM7Z0JBQ3JDLFFBQVEsQ0FBQztRQUNWLEtBQUssQ0FBQyxHQUFHLEVBQUUsUUFBUSxDQUFDLENBQUM7SUFDdEIsQ0FBQztDQUNEO0FBUkQsd0NBUUM7QUFFRCxTQUFnQixlQUFlLENBQUMsR0FBVztJQUMxQyxPQUFPLEdBQUcsQ0FBQyxPQUFPLENBQUMsWUFBWSxFQUFFLEVBQUUsQ0FBQyxDQUFDO0FBQ3RDLENBQUM7QUFGRCwwQ0FFQyIsInNvdXJjZXNDb250ZW50IjpbIi8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tXG4gKiBDb3B5cmlnaHQgKEMpIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbi4gQWxsIHJpZ2h0cyByZXNlcnZlZC5cbiAqLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG5pbXBvcnQgeyBJbnRlcm5hbExvZ2dlciB9IGZyb20gJy4vaW50ZXJuYWxMb2dnZXInO1xuaW1wb3J0IHsgT3V0cHV0RXZlbnQgfSBmcm9tICcuL2RlYnVnU2Vzc2lvbic7XG5cbmV4cG9ydCBlbnVtIExvZ0xldmVsIHtcblx0VmVyYm9zZSA9IDAsXG5cdExvZyA9IDEsXG5cdFdhcm4gPSAyLFxuXHRFcnJvciA9IDMsXG5cdFN0b3AgPSA0XG59XG5cbmV4cG9ydCB0eXBlIElMb2dDYWxsYmFjayA9IChvdXRwdXRFdmVudDogT3V0cHV0RXZlbnQpID0+IHZvaWQ7XG5cbmludGVyZmFjZSBJTG9nSXRlbSB7XG5cdG1zZzogc3RyaW5nO1xuXHRsZXZlbDogTG9nTGV2ZWw7XG59XG5cbmV4cG9ydCBpbnRlcmZhY2UgSUxvZ2dlciB7XG5cdGxvZyhtc2c6IHN0cmluZywgbGV2ZWw/OiBMb2dMZXZlbCk6IHZvaWQ7XG5cdHZlcmJvc2UobXNnOiBzdHJpbmcpOiB2b2lkO1xuXHR3YXJuKG1zZzogc3RyaW5nKTogdm9pZDtcblx0ZXJyb3IobXNnOiBzdHJpbmcpOiB2b2lkO1xufVxuXG5leHBvcnQgaW50ZXJmYWNlIElJbnRlcm5hbExvZ2dlciB7XG5cdGRpc3Bvc2UoKTogUHJvbWlzZTx2b2lkPjtcblx0bG9nKG1zZzogc3RyaW5nLCBsZXZlbDogTG9nTGV2ZWwsIHByZXBlbmRUaW1lc3RhbXA/OiBib29sZWFuKSA6IHZvaWQ7XG5cdHNldHVwKG9wdGlvbnM6IElJbnRlcm5hbExvZ2dlck9wdGlvbnMpOiBQcm9taXNlPHZvaWQ+O1xufVxuXG5leHBvcnQgaW50ZXJmYWNlIElJbnRlcm5hbExvZ2dlck9wdGlvbnMge1xuXHRjb25zb2xlTWluTG9nTGV2ZWw6IExvZ0xldmVsO1xuXHRsb2dGaWxlUGF0aD86IHN0cmluZztcblx0cHJlcGVuZFRpbWVzdGFtcD86IGJvb2xlYW47XG59XG5cbmV4cG9ydCBjbGFzcyBMb2dnZXIge1xuXHRwcml2YXRlIF9sb2dGaWxlUGF0aEZyb21Jbml0OiBzdHJpbmc7XG5cblx0cHJpdmF0ZSBfY3VycmVudExvZ2dlcjogSUludGVybmFsTG9nZ2VyO1xuXHRwcml2YXRlIF9wZW5kaW5nTG9nUTogSUxvZ0l0ZW1bXSA9IFtdO1xuXG5cdGxvZyhtc2c6IHN0cmluZywgbGV2ZWwgPSBMb2dMZXZlbC5Mb2cpOiB2b2lkIHtcblx0XHRtc2cgPSBtc2cgKyAnXFxuJztcblx0XHR0aGlzLl93cml0ZShtc2csIGxldmVsKTtcblx0fVxuXG5cdHZlcmJvc2UobXNnOiBzdHJpbmcpOiB2b2lkIHtcblx0XHR0aGlzLmxvZyhtc2csIExvZ0xldmVsLlZlcmJvc2UpO1xuXHR9XG5cblx0d2Fybihtc2c6IHN0cmluZyk6IHZvaWQge1xuXHRcdHRoaXMubG9nKG1zZywgTG9nTGV2ZWwuV2Fybik7XG5cdH1cblxuXHRlcnJvcihtc2c6IHN0cmluZyk6IHZvaWQge1xuXHRcdHRoaXMubG9nKG1zZywgTG9nTGV2ZWwuRXJyb3IpO1xuXHR9XG5cblx0ZGlzcG9zZSgpOiBQcm9taXNlPHZvaWQ+IHtcblx0XHRpZiAodGhpcy5fY3VycmVudExvZ2dlcikge1xuXHRcdFx0Y29uc3QgZGlzcG9zZVAgPSB0aGlzLl9jdXJyZW50TG9nZ2VyLmRpc3Bvc2UoKTtcblx0XHRcdHRoaXMuX2N1cnJlbnRMb2dnZXIgPSBudWxsO1xuXHRcdFx0cmV0dXJuIGRpc3Bvc2VQO1xuXHRcdH0gZWxzZSB7XG5cdFx0XHRyZXR1cm4gUHJvbWlzZS5yZXNvbHZlKCk7XG5cdFx0fVxuXHR9XG5cblx0LyoqXG5cdCAqIGBsb2dgIGFkZHMgYSBuZXdsaW5lLCBgd3JpdGVgIGRvZXNuJ3Rcblx0ICovXG5cdHByaXZhdGUgX3dyaXRlKG1zZzogc3RyaW5nLCBsZXZlbCA9IExvZ0xldmVsLkxvZyk6IHZvaWQge1xuXHRcdC8vIFtudWxsLCB1bmRlZmluZWRdID0+IHN0cmluZ1xuXHRcdG1zZyA9IG1zZyArICcnO1xuXHRcdGlmICh0aGlzLl9wZW5kaW5nTG9nUSkge1xuXHRcdFx0dGhpcy5fcGVuZGluZ0xvZ1EucHVzaCh7IG1zZywgbGV2ZWwgfSk7XG5cdFx0fSBlbHNlIGlmICh0aGlzLl9jdXJyZW50TG9nZ2VyKSB7XG5cdFx0XHR0aGlzLl9jdXJyZW50TG9nZ2VyLmxvZyhtc2csIGxldmVsKTtcblx0XHR9XG5cdH1cblxuXHQvKipcblx0ICogU2V0IHRoZSBsb2dnZXIncyBtaW5pbXVtIGxldmVsIHRvIGxvZyBpbiB0aGUgY29uc29sZSwgYW5kIHdoZXRoZXIgdG8gbG9nIHRvIHRoZSBmaWxlLiBMb2cgbWVzc2FnZXMgYXJlIHF1ZXVlZCBiZWZvcmUgdGhpcyBpc1xuXHQgKiBjYWxsZWQgdGhlIGZpcnN0IHRpbWUsIGJlY2F1c2UgbWluTG9nTGV2ZWwgZGVmYXVsdHMgdG8gV2Fybi5cblx0ICovXG5cdHNldHVwKGNvbnNvbGVNaW5Mb2dMZXZlbDogTG9nTGV2ZWwsIF9sb2dGaWxlUGF0aD86IHN0cmluZ3xib29sZWFuLCBwcmVwZW5kVGltZXN0YW1wOiBib29sZWFuID0gdHJ1ZSk6IHZvaWQge1xuXHRcdGNvbnN0IGxvZ0ZpbGVQYXRoID0gdHlwZW9mIF9sb2dGaWxlUGF0aCA9PT0gJ3N0cmluZycgP1xuXHRcdFx0X2xvZ0ZpbGVQYXRoIDpcblx0XHRcdChfbG9nRmlsZVBhdGggJiYgdGhpcy5fbG9nRmlsZVBhdGhGcm9tSW5pdCk7XG5cblx0XHRpZiAodGhpcy5fY3VycmVudExvZ2dlcikge1xuXHRcdFx0Y29uc3Qgb3B0aW9ucyA9IHtcblx0XHRcdFx0Y29uc29sZU1pbkxvZ0xldmVsLFxuXHRcdFx0XHRsb2dGaWxlUGF0aCxcblx0XHRcdFx0cHJlcGVuZFRpbWVzdGFtcFxuXHRcdFx0fTtcblx0XHRcdHRoaXMuX2N1cnJlbnRMb2dnZXIuc2V0dXAob3B0aW9ucykudGhlbigoKSA9PiB7XG5cdFx0XHRcdC8vIE5vdyB0aGF0IHdlIGhhdmUgYSBtaW5pbXVtIGxvZ0xldmVsLCB3ZSBjYW4gY2xlYXIgb3V0IHRoZSBxdWV1ZSBvZiBwZW5kaW5nIG1lc3NhZ2VzXG5cdFx0XHRcdGlmICh0aGlzLl9wZW5kaW5nTG9nUSkge1xuXHRcdFx0XHRcdGNvbnN0IGxvZ1EgPSB0aGlzLl9wZW5kaW5nTG9nUTtcblx0XHRcdFx0XHR0aGlzLl9wZW5kaW5nTG9nUSA9IG51bGw7XG5cdFx0XHRcdFx0bG9nUS5mb3JFYWNoKGl0ZW0gPT4gdGhpcy5fd3JpdGUoaXRlbS5tc2csIGl0ZW0ubGV2ZWwpKTtcblx0XHRcdFx0fVxuXHRcdFx0fSk7XG5cblx0XHR9XG5cdH1cblxuXHRpbml0KGxvZ0NhbGxiYWNrOiBJTG9nQ2FsbGJhY2ssIGxvZ0ZpbGVQYXRoPzogc3RyaW5nLCBsb2dUb0NvbnNvbGU/OiBib29sZWFuKTogdm9pZCB7XG5cdFx0Ly8gUmUtaW5pdCwgY3JlYXRlIG5ldyBnbG9iYWwgTG9nZ2VyXG5cdFx0dGhpcy5fcGVuZGluZ0xvZ1EgPSB0aGlzLl9wZW5kaW5nTG9nUSB8fCBbXTtcblx0XHR0aGlzLl9jdXJyZW50TG9nZ2VyID0gbmV3IEludGVybmFsTG9nZ2VyKGxvZ0NhbGxiYWNrLCBsb2dUb0NvbnNvbGUpO1xuXHRcdHRoaXMuX2xvZ0ZpbGVQYXRoRnJvbUluaXQgPSBsb2dGaWxlUGF0aDtcblx0fVxufVxuXG5leHBvcnQgY29uc3QgbG9nZ2VyID0gbmV3IExvZ2dlcigpO1xuXG5leHBvcnQgY2xhc3MgTG9nT3V0cHV0RXZlbnQgZXh0ZW5kcyBPdXRwdXRFdmVudCB7XG5cdGNvbnN0cnVjdG9yKG1zZzogc3RyaW5nLCBsZXZlbDogTG9nTGV2ZWwpIHtcblx0XHRjb25zdCBjYXRlZ29yeSA9XG5cdFx0XHRsZXZlbCA9PT0gTG9nTGV2ZWwuRXJyb3IgPyAnc3RkZXJyJyA6XG5cdFx0XHRsZXZlbCA9PT0gTG9nTGV2ZWwuV2FybiA/ICdjb25zb2xlJyA6XG5cdFx0XHQnc3Rkb3V0Jztcblx0XHRzdXBlcihtc2csIGNhdGVnb3J5KTtcblx0fVxufVxuXG5leHBvcnQgZnVuY3Rpb24gdHJpbUxhc3ROZXdsaW5lKHN0cjogc3RyaW5nKTogc3RyaW5nIHtcblx0cmV0dXJuIHN0ci5yZXBsYWNlKC8oXFxufFxcclxcbikkLywgJycpO1xufVxuXG5cbiJdfQ==

/***/ }),
/* 15 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.InternalLogger = void 0;
const fs = __webpack_require__(2);
const path = __webpack_require__(3);
const mkdirp = __webpack_require__(16);
const logger_1 = __webpack_require__(14);
/**
 * Manages logging, whether to console.log, file, or VS Code console.
 * Encapsulates the state specific to each logging session
 */
class InternalLogger {
    constructor(logCallback, isServer) {
        /** Dispose and allow exit to continue normally */
        this.beforeExitCallback = () => this.dispose();
        this._logCallback = logCallback;
        this._logToConsole = isServer;
        this._minLogLevel = logger_1.LogLevel.Warn;
        this.disposeCallback = (signal, code) => {
            this.dispose();
            // Exit with 128 + value of the signal code.
            // https://nodejs.org/api/process.html#process_exit_codes
            code = code || 2; // SIGINT
            code += 128;
            process.exit(code);
        };
    }
    setup(options) {
        return __awaiter(this, void 0, void 0, function* () {
            this._minLogLevel = options.consoleMinLogLevel;
            this._prependTimestamp = options.prependTimestamp;
            // Open a log file in the specified location. Overwritten on each run.
            if (options.logFilePath) {
                if (!path.isAbsolute(options.logFilePath)) {
                    this.log(`logFilePath must be an absolute path: ${options.logFilePath}`, logger_1.LogLevel.Error);
                }
                else {
                    const handleError = err => this.sendLog(`Error creating log file at path: ${options.logFilePath}. Error: ${err.toString()}\n`, logger_1.LogLevel.Error);
                    try {
                        yield mkdirp(path.dirname(options.logFilePath));
                        this.log(`Verbose logs are written to:\n`, logger_1.LogLevel.Warn);
                        this.log(options.logFilePath + '\n', logger_1.LogLevel.Warn);
                        this._logFileStream = fs.createWriteStream(options.logFilePath);
                        this.logDateTime();
                        this.setupShutdownListeners();
                        this._logFileStream.on('error', err => {
                            handleError(err);
                        });
                    }
                    catch (err) {
                        handleError(err);
                    }
                }
            }
        });
    }
    logDateTime() {
        let d = new Date();
        let dateString = d.getUTCFullYear() + '-' + `${d.getUTCMonth() + 1}` + '-' + d.getUTCDate();
        const timeAndDateStamp = dateString + ', ' + getFormattedTimeString();
        this.log(timeAndDateStamp + '\n', logger_1.LogLevel.Verbose, false);
    }
    setupShutdownListeners() {
        process.addListener('beforeExit', this.beforeExitCallback);
        process.addListener('SIGTERM', this.disposeCallback);
        process.addListener('SIGINT', this.disposeCallback);
    }
    removeShutdownListeners() {
        process.removeListener('beforeExit', this.beforeExitCallback);
        process.removeListener('SIGTERM', this.disposeCallback);
        process.removeListener('SIGINT', this.disposeCallback);
    }
    dispose() {
        return new Promise(resolve => {
            this.removeShutdownListeners();
            if (this._logFileStream) {
                this._logFileStream.end(resolve);
                this._logFileStream = null;
            }
            else {
                resolve();
            }
        });
    }
    log(msg, level, prependTimestamp = true) {
        if (this._minLogLevel === logger_1.LogLevel.Stop) {
            return;
        }
        if (level >= this._minLogLevel) {
            this.sendLog(msg, level);
        }
        if (this._logToConsole) {
            const logFn = level === logger_1.LogLevel.Error ? console.error :
                level === logger_1.LogLevel.Warn ? console.warn :
                    null;
            if (logFn) {
                logFn((0, logger_1.trimLastNewline)(msg));
            }
        }
        // If an error, prepend with '[Error]'
        if (level === logger_1.LogLevel.Error) {
            msg = `[${logger_1.LogLevel[level]}] ${msg}`;
        }
        if (this._prependTimestamp && prependTimestamp) {
            msg = '[' + getFormattedTimeString() + '] ' + msg;
        }
        if (this._logFileStream) {
            this._logFileStream.write(msg);
        }
    }
    sendLog(msg, level) {
        // Truncate long messages, they can hang VS Code
        if (msg.length > 1500) {
            const endsInNewline = !!msg.match(/(\n|\r\n)$/);
            msg = msg.substr(0, 1500) + '[...]';
            if (endsInNewline) {
                msg = msg + '\n';
            }
        }
        if (this._logCallback) {
            const event = new logger_1.LogOutputEvent(msg, level);
            this._logCallback(event);
        }
    }
}
exports.InternalLogger = InternalLogger;
function getFormattedTimeString() {
    let d = new Date();
    let hourString = _padZeroes(2, String(d.getUTCHours()));
    let minuteString = _padZeroes(2, String(d.getUTCMinutes()));
    let secondString = _padZeroes(2, String(d.getUTCSeconds()));
    let millisecondString = _padZeroes(3, String(d.getUTCMilliseconds()));
    return hourString + ':' + minuteString + ':' + secondString + '.' + millisecondString + ' UTC';
}
function _padZeroes(minDesiredLength, numberToPad) {
    if (numberToPad.length >= minDesiredLength) {
        return numberToPad;
    }
    else {
        return String('0'.repeat(minDesiredLength) + numberToPad).slice(-minDesiredLength);
    }
}
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiaW50ZXJuYWxMb2dnZXIuanMiLCJzb3VyY2VSb290IjoiIiwic291cmNlcyI6WyIuLi9zcmMvaW50ZXJuYWxMb2dnZXIudHMiXSwibmFtZXMiOltdLCJtYXBwaW5ncyI6IjtBQUFBOzs7Z0dBR2dHOzs7Ozs7Ozs7Ozs7QUFFaEcseUJBQXlCO0FBQ3pCLDZCQUE2QjtBQUM3QixpQ0FBaUM7QUFFakMscUNBQTRIO0FBRTVIOzs7R0FHRztBQUNILE1BQWEsY0FBYztJQW1CMUIsWUFBWSxXQUF5QixFQUFFLFFBQWtCO1FBVHpELGtEQUFrRDtRQUMxQyx1QkFBa0IsR0FBRyxHQUFHLEVBQUUsQ0FBQyxJQUFJLENBQUMsT0FBTyxFQUFFLENBQUM7UUFTakQsSUFBSSxDQUFDLFlBQVksR0FBRyxXQUFXLENBQUM7UUFDaEMsSUFBSSxDQUFDLGFBQWEsR0FBRyxRQUFRLENBQUM7UUFFOUIsSUFBSSxDQUFDLFlBQVksR0FBRyxpQkFBUSxDQUFDLElBQUksQ0FBQztRQUVsQyxJQUFJLENBQUMsZUFBZSxHQUFHLENBQUMsTUFBYyxFQUFFLElBQVksRUFBRSxFQUFFO1lBQ3ZELElBQUksQ0FBQyxPQUFPLEVBQUUsQ0FBQztZQUVmLDRDQUE0QztZQUM1Qyx5REFBeUQ7WUFDekQsSUFBSSxHQUFHLElBQUksSUFBSSxDQUFDLENBQUMsQ0FBQyxTQUFTO1lBQzNCLElBQUksSUFBSSxHQUFHLENBQUM7WUFFWixPQUFPLENBQUMsSUFBSSxDQUFDLElBQUksQ0FBQyxDQUFDO1FBQ3BCLENBQUMsQ0FBQztJQUNILENBQUM7SUFFWSxLQUFLLENBQUMsT0FBK0I7O1lBQ2pELElBQUksQ0FBQyxZQUFZLEdBQUcsT0FBTyxDQUFDLGtCQUFrQixDQUFDO1lBQy9DLElBQUksQ0FBQyxpQkFBaUIsR0FBRyxPQUFPLENBQUMsZ0JBQWdCLENBQUM7WUFFbEQsc0VBQXNFO1lBQ3RFLElBQUksT0FBTyxDQUFDLFdBQVcsRUFBRTtnQkFDeEIsSUFBSSxDQUFDLElBQUksQ0FBQyxVQUFVLENBQUMsT0FBTyxD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LFlBQVksR0FBRyxHQUFHLEdBQUcsWUFBWSxHQUFHLEdBQUcsR0FBRyxpQkFBaUIsR0FBRyxNQUFNLENBQUM7QUFDaEcsQ0FBQztBQUVELFNBQVMsVUFBVSxDQUFDLGdCQUF3QixFQUFFLFdBQW1CO0lBQ2hFLElBQUksV0FBVyxDQUFDLE1BQU0sSUFBSSxnQkFBZ0IsRUFBRTtRQUMzQyxPQUFPLFdBQVcsQ0FBQztLQUNuQjtTQUFNO1FBQ04sT0FBTyxNQUFNLENBQUMsR0FBRyxDQUFDLE1BQU0sQ0FBQyxnQkFBZ0IsQ0FBQyxHQUFHLFdBQVcsQ0FBQyxDQUFDLEtBQUssQ0FBQyxDQUFDLGdCQUFnQixDQUFDLENBQUM7S0FDbkY7QUFDRixDQUFDIiwic291cmNlc0NvbnRlbnQiOlsiLyotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS1cbiAqICBDb3B5cmlnaHQgKGMpIE1pY3Jvc29mdCBDb3Jwb3JhdGlvbi4gQWxsIHJpZ2h0cyByZXNlcnZlZC5cbiAqICBMaWNlbnNlZCB1bmRlciB0aGUgTUlUIExpY2Vuc2UuIFNlZSBMaWNlbnNlLnR4dCBpbiB0aGUgcHJvamVjdCByb290IGZvciBsaWNlbnNlIGluZm9ybWF0aW9uLlxuICotLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLSovXG5cbmltcG9ydCAqIGFzIGZzIGZyb20gJ2ZzJztcbmltcG9ydCAqIGFzIHBhdGggZnJvbSAncGF0aCc7XG5pbXBvcnQgKiBhcyBta2RpcnAgZnJvbSAnbWtkaXJwJztcblxuaW1wb3J0IHsgTG9nTGV2ZWwsIElMb2dDYWxsYmFjaywgdHJpbUxhc3ROZXdsaW5lLCBMb2dPdXRwdXRFdmVudCwgSUludGVybmFsTG9nZ2VyT3B0aW9ucywgSUludGVybmFsTG9nZ2VyIH0gZnJvbSAnLi9sb2dnZXInO1xuXG4vKipcbiAqIE1hbmFnZXMgbG9nZ2luZywgd2hldGhlciB0byBjb25zb2xlLmxvZywgZmlsZSwgb3IgVlMgQ29kZSBjb25zb2xlLlxuICogRW5jYXBzdWxhdGVzIHRoZSBzdGF0ZSBzcGVjaWZpYyB0byBlYWNoIGxvZ2dpbmcgc2Vzc2lvblxuICovXG5leHBvcnQgY2xhc3MgSW50ZXJuYWxMb2dnZXIgaW1wbGVtZW50cyBJSW50ZXJuYWxMb2dnZXIge1xuXHRwcml2YXRlIF9taW5Mb2dMZXZlbDogTG9nTGV2ZWw7XG5cdHByaXZhdGUgX2xvZ1RvQ29uc29sZTogYm9vbGVhbjtcblxuXHQvKiogTG9nIGluZm8gdGhhdCBtZWV0cyBtaW5Mb2dMZXZlbCBpcyBzZW50IHRvIHRoaXMgY2FsbGJhY2suICovXG5cdHByaXZhdGUgX2xvZ0NhbGxiYWNrOiBJTG9nQ2FsbGJhY2s7XG5cblx0LyoqIFdyaXRlIHN0ZWFtIGZvciBsb2cgZmlsZSAqL1xuXHRwcml2YXRlIF9sb2dGaWxlU3RyZWFtOiBmcy5Xcml0ZVN0cmVhbTtcblxuXHQvKiogRGlzcG9zZSBhbmQgYWxsb3cgZXhpdCB0byBjb250aW51ZSBub3JtYWxseSAqL1xuXHRwcml2YXRlIGJlZm9yZUV4aXRDYWxsYmFjayA9ICgpID0+IHRoaXMuZGlzcG9zZSgpO1xuXG5cdC8qKiBEaXNwb3NlIGFuZCBleGl0ICovXG5cdHByaXZhdGUgZGlzcG9zZUNhbGxiYWNrO1xuXG5cdC8qKiBXaGV0aGVyIHRvIGFkZCBhIHRpbWVzdGFtcCB0byBtZXNzYWdlcyBpbiB0aGUgbG9nZmlsZSAqL1xuXHRwcml2YXRlIF9wcmVwZW5kVGltZXN0YW1wOiBib29sZWFuO1xuXG5cdGNvbnN0cnVjdG9yKGxvZ0NhbGxiYWNrOiBJTG9nQ2FsbGJhY2ssIGlzU2VydmVyPzogYm9vbGVhbikge1xuXHRcdHRoaXMuX2xvZ0NhbGxiYWNrID0gbG9nQ2FsbGJhY2s7XG5cdFx0dGhpcy5fbG9nVG9Db25zb2xlID0gaXNTZXJ2ZXI7XG5cblx0XHR0aGlzLl9taW5Mb2dMZXZlbCA9IExvZ0xldmVsLldhcm47XG5cblx0XHR0aGlzLmRpc3Bvc2VDYWxsYmFjayA9IChzaWduYWw6IHN0cmluZywgY29kZTogbnVtYmVyKSA9PiB7XG5cdFx0XHR0aGlzLmRpc3Bvc2UoKTtcblxuXHRcdFx0Ly8gRXhpdCB3aXRoIDEyOCArIHZhbHVlIG9mIHRoZSBzaWduYWwgY29kZS5cblx0XHRcdC8vIGh0dHBzOi8vbm9kZWpzLm9yZy9hcGkvcHJvY2Vzcy5odG1sI3Byb2Nlc3NfZXhpdF9jb2Rlc1xuXHRcdFx0Y29kZSA9IGNvZGUgfHwgMjsgLy8gU0lHSU5UXG5cdFx0XHRjb2RlICs9IDEyODtcblxuXHRcdFx0cHJvY2Vzcy5leGl0KGNvZGUpO1xuXHRcdH07XG5cdH1cblxuXHRwdWJsaWMgYXN5bmMgc2V0dXAob3B0aW9uczogSUludGVybmFsTG9nZ2VyT3B0aW9ucyk6IFByb21pc2U8dm9pZD4ge1xuXHRcdHRoaXMuX21pbkxvZ0xldmVsID0gb3B0aW9ucy5jb25zb2xlTWluTG9nTGV2ZWw7XG5cdFx0dGhpcy5fcHJlcGVuZFRpbWVzdGFtcCA9IG9wdGlvbnMucHJlcGVuZFRpbWVzdGFtcDtcblxuXHRcdC8vIE9wZW4gYSBsb2cgZmlsZSBpbiB0aGUgc3BlY2lmaWVkIGxvY2F0aW9uLiBPdmVyd3JpdHRlbiBvbiBlYWNoIHJ1bi5cblx0XHRpZiAob3B0aW9ucy5sb2dGaWxlUGF0aCkge1xuXHRcdFx0aWYgKCFwYXRoLmlzQWJzb2x1dGUob3B0aW9ucy5sb2dGaWxlUGF0aCkpIHtcblx0XHRcdFx0dGhpcy5sb2coYGxvZ0ZpbGVQYXRoIG11c3QgYmUgYW4gYWJzb2x1dGUgcGF0aDogJHtvcHRpb25zLmxvZ0ZpbGVQYXRofWAsIExvZ0xldmVsLkVycm9yKTtcblx0XHRcdH0gZWxzZSB7XG5cdFx0XHRcdGNvbnN0IGhhbmRsZUVycm9yID0gZXJyID0+IHRoaXMuc2VuZExvZyhgRXJyb3IgY3JlYXRpbmcgbG9nIGZpbGUgYXQgcGF0aDogJHtvcHRpb25zLmxvZ0ZpbGVQYXRofS4gRXJyb3I6ICR7ZXJyLnRvU3RyaW5nKCl9XFxuYCwgTG9nTGV2ZWwuRXJyb3IpO1xuXG5cdFx0XHRcdHRyeSB7XG5cdFx0XHRcdFx0YXdhaXQgbWtkaXJwKHBhdGguZGlybmFtZShvcHRpb25zLmxvZ0ZpbGVQYXRoKSk7XG5cdFx0XHRcdFx0dGhpcy5sb2coYFZlcmJvc2UgbG9ncyBhcmUgd3JpdHRlbiB0bzpcXG5gLCBMb2dMZXZlbC5XYXJuKTtcblx0XHRcdFx0XHR0aGlzLmxvZyhvcHRpb25zLmxvZ0ZpbGVQYXRoICsgJ1xcbicsIExvZ0xldmVsLldhcm4pO1xuXG5cdFx0XHRcdFx0dGhpcy5fbG9nRmlsZVN0cmVhbSA9IGZzLmNyZWF0ZVdyaXRlU3RyZWFtKG9wdGlvbnMubG9nRmlsZVBhdGgpO1xuXHRcdFx0XHRcdHRoaXMubG9nRGF0ZVRpbWUoKTtcblx0XHRcdFx0XHR0aGlzLnNldHVwU2h1dGRvd25MaXN0ZW5lcnMoKTtcblx0XHRcdFx0XHR0aGlzLl9sb2dGaWxlU3RyZWFtLm9uKCdlcnJvcicsIGVyciA9PiB7XG5cdFx0XHRcdFx0XHRoYW5kbGVFcnJvcihlcnIpO1xuXHRcdFx0XHRcdH0pO1xuXHRcdFx0XHR9IGNhdGNoIChlcnIpIHtcblx0XHRcdFx0XHRoYW5kbGVFcnJvcihlcnIpO1xuXHRcdFx0XHR9XG5cdFx0XHR9XG5cdFx0fVxuXHR9XG5cblx0cHJpdmF0ZSBsb2dEYXRlVGltZSgpOiB2b2lkIHtcblx0XHRsZXQgZCA9IG5ldyBEYXRlKCk7XG5cdFx0bGV0IGRhdGVTdHJpbmcgPSBkLmdldFVUQ0Z1bGxZZWFyKCkgKyAnLScgKyBgJHtkLmdldFVUQ01vbnRoKCkgKyAxfWAgKyAnLScgKyBkLmdldFVUQ0RhdGUoKTtcblx0XHRjb25zdCB0aW1lQW5kRGF0ZVN0YW1wID0gZGF0ZVN0cmluZyArICcsICcgKyBnZXRGb3JtYXR0ZWRUaW1lU3RyaW5nKCk7XG5cdFx0dGhpcy5sb2codGltZUFuZERhdGVTdGFtcCArICdcXG4nLCBMb2dMZXZlbC5WZXJib3NlLCBmYWxzZSk7XG5cdH1cblxuXHRwcml2YXRlIHNldHVwU2h1dGRvd25MaXN0ZW5lcnMoKTogdm9pZCB7XG5cdFx0cHJvY2Vzcy5hZGRMaXN0ZW5lcignYmVmb3JlRXhpdCcsIHRoaXMuYmVmb3JlRXhpdENhbGxiYWNrKTtcblx0XHRwcm9jZXNzLmFkZExpc3RlbmVyKCdTSUdURVJNJywgdGhpcy5kaXNwb3NlQ2FsbGJhY2spO1xuXHRcdHByb2Nlc3MuYWRkTGlzdGVuZXIoJ1NJR0lOVCcsIHRoaXMuZGlzcG9zZUNhbGxiYWNrKTtcblx0fVxuXG5cdHByaXZhdGUgcmVtb3ZlU2h1dGRvd25MaXN0ZW5lcnMoKTogdm9pZCB7XG5cdFx0cHJvY2Vzcy5yZW1vdmVMaXN0ZW5lcignYmVmb3JlRXhpdCcsIHRoaXMuYmVmb3JlRXhpdENhbGxiYWNrKTtcblx0XHRwcm9jZXNzLnJlbW92ZUxpc3RlbmVyKCdTSUdURVJNJywgdGhpcy5kaXNwb3NlQ2FsbGJhY2spO1xuXHRcdHByb2Nlc3MucmVtb3ZlTGlzdGVuZXIoJ1NJR0lOVCcsIHRoaXMuZGlzcG9zZUNhbGxiYWNrKTtcblx0fVxuXG5cdHB1YmxpYyBkaXNwb3NlKCk6IFByb21pc2U8dm9pZD4ge1xuXHRcdHJldHVybiBuZXcgUHJvbWlzZShyZXNvbHZlID0+IHtcblx0XHRcdHRoaXMucmVtb3ZlU2h1dGRvd25MaXN0ZW5lcnMoKTtcblx0XHRcdGlmICh0aGlzLl9sb2dGaWxlU3RyZWFtKSB7XG5cdFx0XHRcdHRoaXMuX2xvZ0ZpbGVTdHJlYW0uZW5kKHJlc29sdmUpO1xuXHRcdFx0XHR0aGlzLl9sb2dGaWxlU3RyZWFtID0gbnVsbDtcblx0XHRcdH0gZWxzZSB7XG5cdFx0XHRcdHJlc29sdmUoKTtcblx0XHRcdH1cblx0XHR9KTtcblx0fVxuXG5cdHB1YmxpYyBsb2cobXNnOiBzdHJpbmcsIGxldmVsOiBMb2dMZXZlbCwgcHJlcGVuZFRpbWVzdGFtcCA9IHRydWUpOiB2b2lkIHtcblx0XHRpZiAodGhpcy5fbWluTG9nTGV2ZWwgPT09IExvZ0xldmVsLlN0b3ApIHtcblx0XHRcdHJldHVybjtcblx0XHR9XG5cblx0XHRpZiAobGV2ZWwgPj0gdGhpcy5fbWluTG9nTGV2ZWwpIHtcblx0XHRcdHRoaXMuc2VuZExvZyhtc2csIGxldmVsKTtcblx0XHR9XG5cblx0XHRpZiAodGhpcy5fbG9nVG9Db25zb2xlKSB7XG5cdFx0XHRjb25zdCBsb2dGbiA9XG5cdFx0XHRcdGxldmVsID09PSBMb2dMZXZlbC5FcnJvciA/IGNvbnNvbGUuZXJyb3IgOlxuXHRcdFx0XHRsZXZlbCA9PT0gTG9nTGV2ZWwuV2FybiA/IGNvbnNvbGUud2FybiA6XG5cdFx0XHRcdG51bGw7XG5cblx0XHRcdGlmIChsb2dGbikge1xuXHRcdFx0XHRsb2dGbih0cmltTGFzdE5ld2xpbmUobXNnKSk7XG5cdFx0XHR9XG5cdFx0fVxuXG5cdFx0Ly8gSWYgYW4gZXJyb3IsIHByZXBlbmQgd2l0aCAnW0Vycm9yXSdcblx0XHRpZiAobGV2ZWwgPT09IExvZ0xldmVsLkVycm9yKSB7XG5cdFx0XHRtc2cgPSBgWyR7TG9nTGV2ZWxbbGV2ZWxdfV0gJHttc2d9YDtcblx0XHR9XG5cblx0XHRpZiAodGhpcy5fcHJlcGVuZFRpbWVzdGFtcCAmJiBwcmVwZW5kVGltZXN0YW1wKSB7XG5cdFx0XHRtc2cgPSAnWycgKyBnZXRGb3JtYXR0ZWRUaW1lU3RyaW5nKCkgKyAnXSAnICsgbXNnO1xuXHRcdH1cblxuXHRcdGlmICh0aGlzLl9sb2dGaWxlU3RyZWFtKSB7XG5cdFx0XHR0aGlzLl9sb2dGaWxlU3RyZWFtLndyaXRlKG1zZyk7XG5cdFx0fVxuXHR9XG5cblx0cHJpdmF0ZSBzZW5kTG9nKG1zZzogc3RyaW5nLCBsZXZlbDogTG9nTGV2ZWwpOiB2b2lkIHtcblx0XHQvLyBUcnVuY2F0ZSBsb25nIG1lc3NhZ2VzLCB0aGV5IGNhbiBoYW5nIFZTIENvZGVcblx0XHRpZiAobXNnLmxlbmd0aCA+IDE1MDApIHtcblx0XHRcdGNvbnN0IGVuZHNJbk5ld2xpbmUgPSAhIW1zZy5tYXRjaCgvKFxcbnxcXHJcXG4pJC8pO1xuXHRcdFx0bXNnID0gbXNnLnN1YnN0cigwLCAxNTAwKSArICdbLi4uXSc7XG5cdFx0XHRpZiAoZW5kc0luTmV3bGluZSkge1xuXHRcdFx0XHRtc2cgPSBtc2cgKyAnXFxuJztcblx0XHRcdH1cblx0XHR9XG5cblx0XHRpZiAodGhpcy5fbG9nQ2FsbGJhY2spIHtcblx0XHRcdGNvbnN0IGV2ZW50ID0gbmV3IExvZ091dHB1dEV2ZW50KG1zZywgbGV2ZWwpO1xuXHRcdFx0dGhpcy5fbG9nQ2FsbGJhY2soZXZlbnQpO1xuXHRcdH1cblx0fVxufVxuXG5mdW5jdGlvbiBnZXRGb3JtYXR0ZWRUaW1lU3RyaW5nKCk6IHN0cmluZyB7XG5cdGxldCBkID0gbmV3IERhdGUoKTtcblx0bGV0IGhvdXJTdHJpbmcgPSBfcGFkWmVyb2VzKDIsIFN0cmluZyhkLmdldFVUQ0hvdXJzKCkpKTtcblx0bGV0IG1pbnV0ZVN0cmluZyA9IF9wYWRaZXJvZXMoMiwgU3RyaW5nKGQuZ2V0VVRDTWludXRlcygpKSk7XG5cdGxldCBzZWNvbmRTdHJpbmcgPSBfcGFkWmVyb2VzKDIsIFN0cmluZyhkLmdldFVUQ1NlY29uZHMoKSkpO1xuXHRsZXQgbWlsbGlzZWNvbmRTdHJpbmcgPSBfcGFkWmVyb2VzKDMsIFN0cmluZyhkLmdldFVUQ01pbGxpc2Vjb25kcygpKSk7XG5cdHJldHVybiBob3VyU3RyaW5nICsgJzonICsgbWludXRlU3RyaW5nICsgJzonICsgc2Vjb25kU3RyaW5nICsgJy4nICsgbWlsbGlzZWNvbmRTdHJpbmcgKyAnIFVUQyc7XG59XG5cbmZ1bmN0aW9uIF9wYWRaZXJvZXMobWluRGVzaXJlZExlbmd0aDogbnVtYmVyLCBudW1iZXJUb1BhZDogc3RyaW5nKTogc3RyaW5nIHtcblx0aWYgKG51bWJlclRvUGFkLmxlbmd0aCA+PSBtaW5EZXNpcmVkTGVuZ3RoKSB7XG5cdFx0cmV0dXJuIG51bWJlclRvUGFkO1xuXHR9IGVsc2Uge1xuXHRcdHJldHVybiBTdHJpbmcoJzAnLnJlcGVhdChtaW5EZXNpcmVkTGVuZ3RoKSArIG51bWJlclRvUGFkKS5zbGljZSgtbWluRGVzaXJlZExlbmd0aCk7XG5cdH1cbn1cbiJdfQ==

/***/ }),
/* 16 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const optsArg = __webpack_require__(17)
const pathArg = __webpack_require__(19)

const {mkdirpNative, mkdirpNativeSync} = __webpack_require__(20)
const {mkdirpManual, mkdirpManualSync} = __webpack_require__(22)
const {useNative, useNativeSync} = __webpack_require__(23)


const mkdirp = (path, opts) => {
  path = pathArg(path)
  opts = optsArg(opts)
  return useNative(opts)
    ? mkdirpNative(path, opts)
    : mkdirpManual(path, opts)
}

const mkdirpSync = (path, opts) => {
  path = pathArg(path)
  opts = optsArg(opts)
  return useNativeSync(opts)
    ? mkdirpNativeSync(path, opts)
    : mkdirpManualSync(path, opts)
}

mkdirp.sync = mkdirpSync
mkdirp.native = (path, opts) => mkdirpNative(pathArg(path), optsArg(opts))
mkdirp.manual = (path, opts) => mkdirpManual(pathArg(path), optsArg(opts))
mkdirp.nativeSync = (path, opts) => mkdirpNativeSync(pathArg(path), optsArg(opts))
mkdirp.manualSync = (path, opts) => mkdirpManualSync(pathArg(path), optsArg(opts))

module.exports = mkdirp


/***/ }),
/* 17 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const { promisify } = __webpack_require__(18)
const fs = __webpack_require__(2)
const optsArg = opts => {
  if (!opts)
    opts = { mode: 0o777, fs }
  else if (typeof opts === 'object')
    opts = { mode: 0o777, fs, ...opts }
  else if (typeof opts === 'number')
    opts = { mode: opts, fs }
  else if (typeof opts === 'string')
    opts = { mode: parseInt(opts, 8), fs }
  else
    throw new TypeError('invalid options argument')

  opts.mkdir = opts.mkdir || opts.fs.mkdir || fs.mkdir
  opts.mkdirAsync = promisify(opts.mkdir)
  opts.stat = opts.stat || opts.fs.stat || fs.stat
  opts.statAsync = promisify(opts.stat)
  opts.statSync = opts.statSync || opts.fs.statSync || fs.statSync
  opts.mkdirSync = opts.mkdirSync || opts.fs.mkdirSync || fs.mkdirSync
  return opts
}
module.exports = optsArg


/***/ }),
/* 18 */
/***/ ((module) => {

"use strict";
module.exports = require("util");

/***/ }),
/* 19 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const platform = process.env.__TESTING_MKDIRP_PLATFORM__ || process.platform
const { resolve, parse } = __webpack_require__(3)
const pathArg = path => {
  if (/\0/.test(path)) {
    // simulate same failure that node raises
    throw Object.assign(
      new TypeError('path must be a string without null bytes'),
      {
        path,
        code: 'ERR_INVALID_ARG_VALUE',
      }
    )
  }

  path = resolve(path)
  if (platform === 'win32') {
    const badWinChars = /[*|"<>?:]/
    const {root} = parse(path)
    if (badWinChars.test(path.substr(root.length))) {
      throw Object.assign(new Error('Illegal characters in path.'), {
        path,
        code: 'EINVAL',
      })
    }
  }

  return path
}
module.exports = pathArg


/***/ }),
/* 20 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const {dirname} = __webpack_require__(3)
const {findMade, findMadeSync} = __webpack_require__(21)
const {mkdirpManual, mkdirpManualSync} = __webpack_require__(22)

const mkdirpNative = (path, opts) => {
  opts.recursive = true
  const parent = dirname(path)
  if (parent === path)
    return opts.mkdirAsync(path, opts)

  return findMade(opts, path).then(made =>
    opts.mkdirAsync(path, opts).then(() => made)
    .catch(er => {
      if (er.code === 'ENOENT')
        return mkdirpManual(path, opts)
      else
        throw er
    }))
}

const mkdirpNativeSync = (path, opts) => {
  opts.recursive = true
  const parent = dirname(path)
  if (parent === path)
    return opts.mkdirSync(path, opts)

  const made = findMadeSync(opts, path)
  try {
    opts.mkdirSync(path, opts)
    return made
  } catch (er) {
    if (er.code === 'ENOENT')
      return mkdirpManualSync(path, opts)
    else
      throw er
  }
}

module.exports = {mkdirpNative, mkdirpNativeSync}


/***/ }),
/* 21 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const {dirname} = __webpack_require__(3)

const findMade = (opts, parent, path = undefined) => {
  // we never want the 'made' return value to be a root directory
  if (path === parent)
    return Promise.resolve()

  return opts.statAsync(parent).then(
    st => st.isDirectory() ? path : undefined, // will fail later
    er => er.code === 'ENOENT'
      ? findMade(opts, dirname(parent), parent)
      : undefined
  )
}

const findMadeSync = (opts, parent, path = undefined) => {
  if (path === parent)
    return undefined

  try {
    return opts.statSync(parent).isDirectory() ? path : undefined
  } catch (er) {
    return er.code === 'ENOENT'
      ? findMadeSync(opts, dirname(parent), parent)
      : undefined
  }
}

module.exports = {findMade, findMadeSync}


/***/ }),
/* 22 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const {dirname} = __webpack_require__(3)

const mkdirpManual = (path, opts, made) => {
  opts.recursive = false
  const parent = dirname(path)
  if (parent === path) {
    return opts.mkdirAsync(path, opts).catch(er => {
      // swallowed by recursive implementation on posix systems
      // any other error is a failure
      if (er.code !== 'EISDIR')
        throw er
    })
  }

  return opts.mkdirAsync(path, opts).then(() => made || path, er => {
    if (er.code === 'ENOENT')
      return mkdirpManual(parent, opts)
        .then(made => mkdirpManual(path, opts, made))
    if (er.code !== 'EEXIST' && er.code !== 'EROFS')
      throw er
    return opts.statAsync(path).then(st => {
      if (st.isDirectory())
        return made
      else
        throw er
    }, () => { throw er })
  })
}

const mkdirpManualSync = (path, opts, made) => {
  const parent = dirname(path)
  opts.recursive = false

  if (parent === path) {
    try {
      return opts.mkdirSync(path, opts)
    } catch (er) {
      // swallowed by recursive implementation on posix systems
      // any other error is a failure
      if (er.code !== 'EISDIR')
        throw er
      else
        return
    }
  }

  try {
    opts.mkdirSync(path, opts)
    return made || path
  } catch (er) {
    if (er.code === 'ENOENT')
      return mkdirpManualSync(path, opts, mkdirpManualSync(parent, opts, made))
    if (er.code !== 'EEXIST' && er.code !== 'EROFS')
      throw er
    try {
      if (!opts.statSync(path).isDirectory())
        throw er
    } catch (_) {
      throw er
    }
  }
}

module.exports = {mkdirpManual, mkdirpManualSync}


/***/ }),
/* 23 */
/***/ ((module, __unused_webpack_exports, __webpack_require__) => {

const fs = __webpack_require__(2)

const version = process.env.__TESTING_MKDIRP_NODE_VERSION__ || process.version
const versArr = version.replace(/^v/, '').split('.')
const hasNative = +versArr[0] > 10 || +versArr[0] === 10 && +versArr[1] >= 12

const useNative = !hasNative ? () => false : opts => opts.mkdir === fs.mkdir
const useNativeSync = !hasNative ? () => false : opts => opts.mkdirSync === fs.mkdirSync

module.exports = {useNative, useNativeSync}


/***/ }),
/* 24 */
/***/ ((__unused_webpack_module, exports) => {

"use strict";

/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.Handles = void 0;
class Handles {
    constructor(startHandle) {
        this.START_HANDLE = 1000;
        this._handleMap = new Map();
        this._nextHandle = typeof startHandle === 'number' ? startHandle : this.START_HANDLE;
    }
    reset() {
        this._nextHandle = this.START_HANDLE;
        this._handleMap = new Map();
    }
    create(value) {
        var handle = this._nextHandle++;
        this._handleMap.set(handle, value);
        return handle;
    }
    get(handle, dflt) {
        return this._handleMap.get(handle) || dflt;
    }
}
exports.Handles = Handles;
//# sourceMappingURL=data:application/json;base64,eyJ2ZXJzaW9uIjozLCJmaWxlIjoiaGFuZGxlcy5qcyIsInNvdXJjZVJvb3QiOiIiLCJzb3VyY2VzIjpbIi4uL3NyYy9oYW5kbGVzLnRzIl0sIm5hbWVzIjpbXSwibWFwcGluZ3MiOiI7QUFBQTs7O2dHQUdnRzs7O0FBRWhHLE1BQWEsT0FBTztJQU9uQixZQUFtQixXQUFvQjtRQUwvQixpQkFBWSxHQUFHLElBQUksQ0FBQztRQUdwQixlQUFVLEdBQUcsSUFBSSxHQUFHLEVBQWEsQ0FBQztRQUd6QyxJQUFJLENBQUMsV0FBVyxHQUFHLE9BQU8sV0FBVyxLQUFLLFFBQVEsQ0FBQyxDQUFDLENBQUMsV0FBVyxDQUFDLENBQUMsQ0FBQyxJQUFJLENBQUMsWUFBWSxDQUFDO0lBQ3RGLENBQUM7SUFFTSxLQUFLO1FBQ1gsSUFBSSxDQUFDLFdBQVcsR0FBRyxJQUFJLENBQUMsWUFBWSxDQUFDO1FBQ3JDLElBQUksQ0FBQyxVQUFVLEdBQUcsSUFBSSxHQUFHLEVBQWEsQ0FBQztJQUN4QyxDQUFDO0lBRU0sTUFBTSxDQUFDLEtBQVE7UUFDckIsSUFBSSxNQUFNLEdBQUcsSUFBSSxDQUFDLFdBQVcsRUFBRSxDQUFDO1FBQ2hDLElBQUksQ0FBQyxVQUFVLENBQUMsR0FBRyxDQUFDLE1BQU0sRUFBRSxLQUFLLENBQUMsQ0FBQztRQUNuQyxPQUFPLE1BQU0sQ0FBQztJQUNmLENBQUM7SUFFTSxHQUFHLENBQUMsTUFBYyxFQUFFLElBQVE7UUFDbEMsT0FBTyxJQUFJLENBQUMsVUFBVSxDQUFDLEdBQUcsQ0FBQyxNQUFNLENBQUMsSUFBSSxJQUFJLENBQUM7SUFDNUMsQ0FBQztDQUNEO0FBekJELDBCQXlCQyIsInNvdXJjZXNDb250ZW50IjpbIi8qLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tXG4gKiAgQ29weXJpZ2h0IChjKSBNaWNyb3NvZnQgQ29ycG9yYXRpb24uIEFsbCByaWdodHMgcmVzZXJ2ZWQuXG4gKiAgTGljZW5zZWQgdW5kZXIgdGhlIE1JVCBMaWNlbnNlLiBTZWUgTGljZW5zZS50eHQgaW4gdGhlIHByb2plY3Qgcm9vdCBmb3IgbGljZW5zZSBpbmZvcm1hdGlvbi5cbiAqLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0tLS0qL1xuXG5leHBvcnQgY2xhc3MgSGFuZGxlczxUPiB7XG5cblx0cHJpdmF0ZSBTVEFSVF9IQU5ETEUgPSAxMDAwO1xuXG5cdHByaXZhdGUgX25leHRIYW5kbGUgOiBudW1iZXI7XG5cdHByaXZhdGUgX2hhbmRsZU1hcCA9IG5ldyBNYXA8bnVtYmVyLCBUPigpO1xuXG5cdHB1YmxpYyBjb25zdHJ1Y3RvcihzdGFydEhhbmRsZT86IG51bWJlcikge1xuXHRcdHRoaXMuX25leHRIYW5kbGUgPSB0eXBlb2Ygc3RhcnRIYW5kbGUgPT09ICdudW1iZXInID8gc3RhcnRIYW5kbGUgOiB0aGlzLlNUQVJUX0hBTkRMRTtcblx0fVxuXG5cdHB1YmxpYyByZXNldCgpOiB2b2lkIHtcblx0XHR0aGlzLl9uZXh0SGFuZGxlID0gdGhpcy5TVEFSVF9IQU5ETEU7XG5cdFx0dGhpcy5faGFuZGxlTWFwID0gbmV3IE1hcDxudW1iZXIsIFQ+KCk7XG5cdH1cblxuXHRwdWJsaWMgY3JlYXRlKHZhbHVlOiBUKTogbnVtYmVyIHtcblx0XHR2YXIgaGFuZGxlID0gdGhpcy5fbmV4dEhhbmRsZSsrO1xuXHRcdHRoaXMuX2hhbmRsZU1hcC5zZXQoaGFuZGxlLCB2YWx1ZSk7XG5cdFx0cmV0dXJuIGhhbmRsZTtcblx0fVxuXG5cdHB1YmxpYyBnZXQoaGFuZGxlOiBudW1iZXIsIGRmbHQ/OiBUKTogVCB7XG5cdFx0cmV0dXJuIHRoaXMuX2hhbmRsZU1hcC5nZXQoaGFuZGxlKSB8fCBkZmx0O1xuXHR9XG59XG4iXX0=

/***/ }),
/* 25 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.EmulatorClient = void 0;
const net = __importStar(__webpack_require__(11));
/**
 * TCP client for the Sugarbox debug server.
 *
 * Commands are serialised through an internal queue: only one command is
 * in-flight at a time.  This prevents the earlier single-pendingResolve
 * design from misfiring when several callers (e.g. hardware-panel refresh)
 * issue concurrent send() calls.
 *
 * Emulator events (type === "event") are dispatched independently via
 * onEvent and never go through the queue.
 */
class EmulatorClient {
    constructor() {
        this.buffer = "";
        this.queue = [];
        this.inflight = null;
    }
    connect(port = 1234, host = "127.0.0.1") {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection(port, host, () => { resolve(); });
            this.socket.on("data", data => this.onData(data));
            this.socket.on("error", err => { reject(err); });
            this.socket.on("close", () => console.log(`EmulatorClient: socket closed (port ${port})`));
        });
    }
    disconnect() {
        if (this.socket && !this.socket.destroyed) {
            this.socket.destroy();
        }
    }
    send(cmd) {
        return new Promise((resolve, reject) => {
            if (!this.socket || this.socket.destroyed) {
                reject(new Error("Socket not connected"));
                return;
            }
            this.queue.push({ msg: JSON.stringify(cmd) + "\n", resolve, reject });
            this.flush();
        });
    }
    // ── Internal ─────────────────────────────────────────────────────────────
    flush() {
        if (this.inflight || this.queue.length === 0)
            return;
        const { msg, resolve, reject } = this.queue.shift();
        let settled = false;
        const timer = setTimeout(() => {
            if (settled)
                return;
            settled = true;
            this.inflight = null;
            reject(new Error("Emulator did not respond in time"));
            this.flush(); // continue with next queued command
        }, 10000);
        this.inflight = (response) => {
            if (settled)
                return;
            settled = true;
            clearTimeout(timer);
            this.inflight = null;
            resolve(response);
            this.flush(); // continue with next queued command
        };
        this.socket.write(msg);
    }
    onData(data) {
        this.buffer += data.toString();
        let idx;
        while ((idx = this.buffer.indexOf("\n")) !== -1) {
            const line = this.buffer.slice(0, idx);
            this.buffer = this.buffer.slice(idx + 1);
            if (!line.trim())
                continue;
            try {
                const msg = JSON.parse(line);
                if (msg.type === "event") {
                    this.onEvent?.(msg);
                }
                else if (this.inflight) {
                    this.inflight(msg);
                }
                else {
                    console.warn("EmulatorClient: received response with no pending command:", line);
                }
            }
            catch (e) {
                console.error("EmulatorClient: invalid JSON from emulator:", line);
            }
        }
    }
}
exports.EmulatorClient = EmulatorClient;
//# sourceMappingURL=EmulatorClient.js.map

/***/ }),
/* 26 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.SymbolTable = void 0;
const fs = __importStar(__webpack_require__(2));
/**
 * Symbol table built from assembler output files.
 * Supports multiple loaders (RASM for now).
 */
class SymbolTable {
    constructor() {
        // address → list of symbol names (multiple labels can share an address)
        this.addressToNames = new Map();
        // name → address (reverse lookup for label breakpoints)
        this.nameToAddress = new Map();
        this.symbols = [];
        // ─── Future loaders ─────────────────────────────────────────────────────
        // static fromSjasmplus(filePath: string): SymbolTable { ... }
        // static fromPasmo(filePath: string): SymbolTable { ... }
    }
    get size() { return this.symbols.length; }
    /** Returns all label names defined at a given address. */
    getLabelsAt(address) {
        return this.addressToNames.get(address) ?? [];
    }
    /** Resolves a label name to its address, or undefined if not found. */
    resolveLabel(name) {
        return this.nameToAddress.get(name);
    }
    /** Returns all known label names (for completions). */
    getAllNames() {
        return Array.from(this.nameToAddress.keys());
    }
    /** True if any label exists in [startAddr, endAddr). */
    hasLabelsInRange(startAddr, endAddr) {
        for (const addr of this.addressToNames.keys()) {
            if (addr >= startAddr && addr < endAddr)
                return true;
        }
        return false;
    }
    addEntry(entry) {
        this.symbols.push(entry);
        const existing = this.addressToNames.get(entry.address) ?? [];
        existing.push(entry.name);
        this.addressToNames.set(entry.address, existing);
        // First definition wins for the reverse map (aliases don't override labels)
        if (!this.nameToAddress.has(entry.name)) {
            this.nameToAddress.set(entry.name, entry.address);
        }
    }
    /** Merge all entries from another SymbolTable into this one. */
    merge(other) {
        for (const entry of other.symbols) {
            this.addEntry(entry);
        }
    }
    // ─── RASM loader ────────────────────────────────────────────────────────
    /**
     * Parse a RASM super-symbol file (.rasm).
     * Format (all on one line, semicolon-separated):
     *   romlabel NAME DECIMAL_ADDR BANK
     *   alias NAME DECIMAL_ADDR
     */
    static fromRasm(filePath) {
        const table = new SymbolTable();
        let content;
        try {
            content = fs.readFileSync(filePath, "utf-8");
        }
        catch (e) {
            console.error("SymbolTable: cannot read", filePath, e);
            return table;
        }
        // Entries are separated by semicolons (file is typically one long line)
        const entries = content.split(";");
        for (const raw of entries) {
            const token = raw.trim();
            if (!token)
                continue;
            const parts = token.split(/\s+/);
            const tag = parts[0];
            // label NAME ADDR BANK  — RAM label (regular assembler label)
            // romlabel NAME ADDR BANK — ROM label
            if ((tag === "label" || tag === "romlabel") && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr)) {
                    table.addEntry({ name: parts[1], address: addr, bank });
                }
                continue;
            }
            // alias NAME VALUE — EQU constant
            if (tag === "alias" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                if (!isNaN(addr)) {
                    table.addEntry({ name: parts[1], address: addr });
                }
                continue;
            }
        }
        console.log(`SymbolTable: loaded ${table.symbols.length} symbols from ${filePath}`);
        return table;
    }
    // ─── Super snapshot (REMU chunk) loader ─────────────────────────────────
    /**
     * Extract symbols and breakpoint addresses from the REMU chunk of a RASM
     * super-snapshot (.sna v3).  Returns an empty result if the file cannot be
     * read or contains no REMU chunk.
     *
     * REMU is pure ASCII, semicolon-separated tags:
     *   brk ADDR BANK          — exec BP in RAM
     *   rombrk ADDR ROM        — exec BP in ROM (stored but not yet used)
     *   label NAME ADDR BANK   — RAM symbol
     *   romlabel NAME ADDR ROM — ROM symbol
     *   alias NAME VALUE       — constant
     *   comz / romcomz         — comments, ignored
     */
    static fromSnapshotRemu(snapshotPath) {
        const empty = { table: new SymbolTable(), breakpoints: [] };
        let buf;
        try {
            buf = fs.readFileSync(snapshotPath);
        }
        catch {
            return empty;
        }
        const snaVersion = buf[16]; // byte 16 of SNA header = version (0/1=v1, 2=v2, 3=v3)
        console.log(`SymbolTable: SNA file size=${buf.length}, header version byte=${snaVersion}`);
        // SNA v3: 256-byte header, then chunks of [4-byte id][4-byte LE size][data].
        // A v3 SNA can be much smaller than 65792 bytes (no full RAM dump needed),
        // so do NOT short-circuit on file size — just scan chunks from offset 256.
        let offset = 256;
        const foundChunks = [];
        while (offset + 8 <= buf.length) {
            const chunkId = buf.toString("ascii", offset, offset + 4);
            const chunkSize = buf.readUInt32LE(offset + 4);
            foundChunks.push(`${chunkId}(${chunkSize})`);
            offset += 8;
            if (offset + chunkSize > buf.length) {
                console.log(`SymbolTable: chunk ${chunkId} size=${chunkSize} overflows file — stopping`);
                break;
            }
            if (chunkId === "REMU") {
                const text = buf.toString("ascii", offset, offset + chunkSize);
                console.log(`SymbolTable: found REMU chunk (${chunkSize} bytes)`);
                return SymbolTable._parseRemuText(text);
            }
            offset += chunkSize;
        }
        console.log(`SymbolTable: no REMU chunk found. Chunks: ${foundChunks.join(", ")}`);
        return empty;
    }
    static _parseRemuText(text) {
        const table = new SymbolTable();
        const breakpoints = [];
        for (const raw of text.split(";")) {
            const token = raw.trim();
            if (!token)
                continue;
            const parts = token.split(/\s+/);
            const tag = parts[0];
            if (tag === "brk" && parts.length >= 2) {
                const addr = parseInt(parts[1], 10);
                if (!isNaN(addr))
                    breakpoints.push(addr);
            }
            else if (tag === "label" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr))
                    table.addEntry({ name: parts[1], address: addr, bank });
            }
            else if (tag === "romlabel" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr))
                    table.addEntry({ name: parts[1], address: addr, bank });
            }
            else if (tag === "alias" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                if (!isNaN(addr))
                    table.addEntry({ name: parts[1], address: addr });
            }
            // comz, romcomz — ignored
        }
        console.log(`SymbolTable: REMU — ${table.symbols.length} symbols, ${breakpoints.length} breakpoints`);
        return { table, breakpoints };
    }
}
exports.SymbolTable = SymbolTable;
//# sourceMappingURL=SymbolTable.js.map

/***/ }),
/* 27 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.SourceAnnotations = void 0;
const fs = __importStar(__webpack_require__(2));
/**
 * Extracts source-level annotations (comments) from a Z80 assembly file.
 *
 * For each label found in the source, records:
 *   - the block of comment/blank lines immediately before it (preamble)
 *   - the inline comment on the label line itself
 *
 * These annotations are used to enrich the disassembly view shown in VS Code.
 */
class SourceAnnotations {
    constructor() {
        this.byLabel = new Map();
    }
    getAnnotation(labelName) {
        return this.byLabel.get(labelName);
    }
    static fromFile(filePath) {
        const ann = new SourceAnnotations();
        let content;
        try {
            content = fs.readFileSync(filePath, "utf-8");
        }
        catch (e) {
            console.warn(`SourceAnnotations: cannot read ${filePath}:`, e);
            return ann;
        }
        const lines = content.split(/\r?\n/);
        let preamble = [];
        for (const line of lines) {
            const trimmed = line.trim();
            // Standalone comment or blank line — accumulate as potential preamble
            if (trimmed.startsWith(";") || trimmed === "") {
                preamble.push(line.trimEnd());
                continue;
            }
            // Label at column 0 (RASM convention): "labelname:" optionally followed
            // by an instruction and/or a comment.
            // We only match labels starting at column 0 (no leading whitespace).
            const labelMatch = line.match(/^(\w+)\s*:(.*)/);
            if (labelMatch) {
                const labelName = labelMatch[1];
                const rest = labelMatch[2].trim();
                // Extract inline comment (everything from the first ";" onward)
                let inlineComment = "";
                const semicolonIdx = rest.indexOf(";");
                if (semicolonIdx !== -1) {
                    inlineComment = rest.slice(semicolonIdx).trim();
                }
                ann.byLabel.set(labelName, {
                    comment: inlineComment,
                    preamble: trimPreamble(preamble),
                });
                // Reset preamble after consuming it
                preamble = [];
                continue;
            }
            // Any other line (instruction, directive) — reset preamble accumulator
            preamble = [];
        }
        console.log(`SourceAnnotations: ${ann.byLabel.size} annotated label(s) from ${filePath}`);
        return ann;
    }
}
exports.SourceAnnotations = SourceAnnotations;
/** Remove leading and trailing blank lines from a preamble block. */
function trimPreamble(lines) {
    let start = 0;
    while (start < lines.length && lines[start].trim() === "")
        start++;
    let end = lines.length - 1;
    while (end >= start && lines[end].trim() === "")
        end--;
    return lines.slice(start, end + 1);
}
//# sourceMappingURL=SourceAnnotations.js.map

/***/ }),
/* 28 */
/***/ ((module) => {

"use strict";
module.exports = require("child_process");

/***/ }),
/* 29 */
/***/ ((module) => {

"use strict";
module.exports = require("os");

/***/ }),
/* 30 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.MemoryViewPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const BYTES_PER_ROW = 16;
const ROW_COUNT = 16; // lines visible by default (one "page" = 256 bytes)
class MemoryViewPanel {
    // ── Static factory ────────────────────────────────────────────────────────
    static createOrShow(address) {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;
        if (MemoryViewPanel.currentPanel) {
            MemoryViewPanel.currentPanel._panel.reveal(column);
            if (address !== undefined) {
                MemoryViewPanel.currentPanel._address = address & 0xFFFF;
                MemoryViewPanel.currentPanel._sendAddress();
                MemoryViewPanel.currentPanel._loadMemory();
            }
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80memoryView", "Z80 Memory", column, { enableScripts: true, retainContextWhenHidden: true });
        MemoryViewPanel.currentPanel = new MemoryViewPanel(panel, address ?? 0);
    }
    // ── Constructor ───────────────────────────────────────────────────────────
    constructor(panel, address) {
        this._address = 0;
        this._source = { type: "read", bank: -1, label: "Memory (Read)", maxAddr: 0xFFFF };
        this._panel = panel;
        this._address = address & 0xFFFF;
        this._panel.webview.html = this._buildHtml();
        this._panel.onDidDispose(() => {
            MemoryViewPanel.currentPanel = undefined;
        });
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            switch (msg.type) {
                case "ready":
                    this._sendAddress();
                    await this._loadSources();
                    // _loadMemory called at end of _loadSources after sources are sent
                    break;
                case "requestMemory":
                    this._address = msg.address & 0xFFFF;
                    if (msg.source)
                        this._source = msg.source;
                    await this._loadMemory();
                    break;
            }
        });
    }
    // ── Helpers ───────────────────────────────────────────────────────────────
    _sendAddress() {
        this._panel.webview.postMessage({ type: "setAddress", address: this._address });
    }
    _defaultSources() {
        return [
            { type: "read", bank: -1, label: "Memory (Read)", maxAddr: 0xFFFF },
            { type: "write", bank: -1, label: "Memory (Write)", maxAddr: 0xFFFF },
            { type: "ram", bank: -1, label: "RAM lower bank", maxAddr: 0xFFFF },
        ];
    }
    async _loadSources() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        let sources;
        try {
            const result = await session.customRequest("getMemBanks", {});
            // result.sources === null signals "emulator binary too old, no getMemBanks"
            if (result?.sources === null) {
                sources = this._defaultSources();
            }
            else {
                sources = Array.isArray(result?.sources) && result.sources.length > 0
                    ? result.sources
                    : this._defaultSources();
            }
        }
        catch {
            sources = this._defaultSources();
        }
        this._panel.webview.postMessage({ type: "memSources", sources });
        await this._loadMemory();
    }
    async _loadMemory() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        const count = BYTES_PER_ROW * ROW_COUNT;
        const src = this._source;
        try {
            const result = await session.customRequest("readMemoryEx", {
                address: this._address,
                count,
                memType: src.type,
                bank: src.bank
            });
            const bytes = result?.bytes ?? [];
            this._panel.webview.postMessage({ type: "memoryData", address: this._address, bytes });
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    // ── HTML ──────────────────────────────────────────────────────────────────
    _buildHtml() {
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
  :root {
    --addr-color:   var(--vscode-descriptionForeground, #888);
    --hex-color:    var(--vscode-editor-foreground, #ccc);
    --ascii-color:  var(--vscode-textPreformat-foreground, #9cdcfe);
    --bg:           var(--vscode-editor-background, #1e1e1e);
    --sel-primary:  var(--vscode-editor-selectionBackground, #264f78);
    --sel-secondary:var(--vscode-editor-inactiveSelectionBackground, #3a3d41);
    --hover-bg:     var(--vscode-list-hoverBackground, #2a2d2e);
    --border:       var(--vscode-panel-border, #444);
    --font:         var(--vscode-editor-font-family, 'Consolas', 'Courier New', monospace);
    --font-size:    var(--vscode-editor-font-size, 13px);
  }
  * { box-sizing: border-box; }
  body {
    background: var(--bg);
    color: var(--hex-color);
    font-family: var(--font);
    font-size: var(--font-size);
    margin: 0;
    padding: 8px;
    user-select: none;
  }

  /* ── Toolbar ── */
  #toolbar {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-bottom: 8px;
    flex-wrap: wrap;
  }
  #toolbar label { color: var(--addr-color); }
  #sourceSelect {
    font-family: var(--font);
    font-size: var(--font-size);
    background: var(--vscode-dropdown-background, #3c3c3c);
    color: var(--vscode-dropdown-foreground, #ccc);
    border: 1px solid var(--vscode-dropdown-border, #555);
    padding: 2px 4px;
    min-width: 14ch;
    max-width: 22ch;
    cursor: pointer;
  }
  #addrInput {
    font-family: var(--font);
    font-size: var(--font-size);
    background: var(--vscode-input-background, #3c3c3c);
    color: var(--vscode-input-foreground, #ccc);
    border: 1px solid var(--vscode-input-border, #555);
    padding: 2px 6px;
    width: 7ch;
    text-transform: uppercase;
  }
  button {
    font-family: var(--font);
    font-size: var(--font-size);
    background: var(--vscode-button-secondaryBackground, #3a3d41);
    color: var(--vscode-button-secondaryForeground, #ccc);
    border: 1px solid var(--border);
    padding: 2px 8px;
    cursor: pointer;
  }
  button:hover { background: var(--vscode-button-secondaryHoverBackground, #4a4d52); }
  #statusBar {
    color: var(--addr-color);
    font-size: 0.9em;
    margin-left: auto;
  }

  /* ── Memory table ── */
  #memTable {
    border-collapse: collapse;
    white-space: nowrap;
  }
  #memTable thead th {
    color: var(--addr-color);
    font-weight: normal;
    padding: 0 0 4px 0;
    text-align: left;
  }
  th.hex-head { padding-left: 4px; }
  th.ascii-head { padding-left: 12px; }
  .hb.hdr { color: var(--addr-color); pointer-events: none; }

  td.addr-cell {
    color: var(--addr-color);
    padding-right: 12px;
    padding-top: 1px;
    padding-bottom: 1px;
    vertical-align: top;
  }
  td.hex-cell {
    padding-right: 12px;
    vertical-align: top;
  }
  td.ascii-cell {
    color: var(--ascii-color);
    vertical-align: top;
  }

  /* Individual byte spans */
  .hb, .ab {
    display: inline-block;
    cursor: default;
    border-radius: 2px;
    padding: 0 1px;
  }
  .hb { min-width: 2.5ch; }
  .hb.gap { margin-left: 6px; }   /* gap between byte 7 and 8 */

  .hb:hover, .ab:hover { background: var(--hover-bg); }

  /* Selection states */
  .sel-primary   { background: var(--sel-primary) !important; }
  .sel-secondary { background: var(--sel-secondary) !important; }

  /* Error / status */
  #errorMsg {
    color: var(--vscode-errorForeground, #f48771);
    padding: 4px;
    display: none;
  }
</style>
</head>
<body>

<div id="toolbar">
  <label for="sourceSelect">Source:</label>
  <select id="sourceSelect" title="Memory source"></select>
  <label for="addrInput">Address:</label>
  <input id="addrInput" type="text" value="0000" maxlength="6" spellcheck="false">
  <button id="btnGo">Go</button>
  <button id="btnPrev">&#8592; &#x2212;256</button>
  <button id="btnNext">&#x2B;256 &#8594;</button>
  <button id="btnRefresh">&#x21BA; Refresh</button>
  <span id="statusBar"></span>
</div>
<div id="errorMsg"></div>

<table id="memTable" cellspacing="0">
  <thead>
    <tr>
      <th>Addr</th>
      <th class="hex-head" id="hexHeader"></th>
      <th class="ascii-head">ASCII</th>
    </tr>
  </thead>
  <tbody id="memBody"></tbody>
</table>

<script>
// ─── State ────────────────────────────────────────────────────────────────────
const vscode = acquireVsCodeApi();
const BYTES_PER_ROW = 16;
const ROW_COUNT     = 16;

let currentAddress = 0;
let currentBytes   = [];        // flat array of ROW_COUNT*BYTES_PER_ROW bytes
let currentSource  = { type: 'read', bank: -1, label: 'Memory (Read)', maxAddr: 0xFFFF };
let sources        = [];        // list received from extension

// Selection
let selStart  = -1;
let selEnd    = -1;
let selColumn = null;           // 'hex' | 'ascii'
let dragging  = false;

// ─── DOM helpers ─────────────────────────────────────────────────────────────
const sourceSelect = document.getElementById('sourceSelect');
const addrInput  = document.getElementById('addrInput');
const btnGo      = document.getElementById('btnGo');
const btnPrev    = document.getElementById('btnPrev');
const btnNext    = document.getElementById('btnNext');
const btnRefresh = document.getElementById('btnRefresh');
const statusBar  = document.getElementById('statusBar');
const errorMsg   = document.getElementById('errorMsg');
const memBody    = document.getElementById('memBody');

// ─── Render ───────────────────────────────────────────────────────────────────
function render(address, bytes) {
    currentAddress = address;
    currentBytes   = bytes;

    // Reset selection on new data
    clearSelection();
    addrInput.value = address.toString(16).toUpperCase().padStart(4, '0');
    statusBar.textContent = '';
    errorMsg.style.display = 'none';

    memBody.innerHTML = '';
    for (let row = 0; row < ROW_COUNT; row++) {
        const rowAddr = (address + row * BYTES_PER_ROW) & 0xFFFF;
        const tr = document.createElement('tr');

        // ── Address cell ──
        const tdA = document.createElement('td');
        tdA.className = 'addr-cell';
        tdA.textContent = rowAddr.toString(16).toUpperCase().padStart(4, '0');
        tr.appendChild(tdA);

        // ── Hex cell ──
        const tdH = document.createElement('td');
        tdH.className = 'hex-cell';
        for (let col = 0; col < BYTES_PER_ROW; col++) {
            const idx   = row * BYTES_PER_ROW + col;
            const b     = bytes[idx] ?? 0;
            const sp    = document.createElement('span');
            sp.className = 'hb' + (col === 8 ? ' gap' : '');
            sp.dataset.idx = idx;
            sp.dataset.col = 'hex';
            sp.textContent = b.toString(16).toUpperCase().padStart(2, '0');
            tdH.appendChild(sp);
        }
        tr.appendChild(tdH);

        // ── ASCII cell ──
        const tdAsc = document.createElement('td');
        tdAsc.className = 'ascii-cell';
        for (let col = 0; col < BYTES_PER_ROW; col++) {
            const idx = row * BYTES_PER_ROW + col;
            const b   = bytes[idx] ?? 0;
            const sp  = document.createElement('span');
            sp.className  = 'ab';
            sp.dataset.idx = idx;
            sp.dataset.col = 'ascii';
            sp.textContent = (b >= 0x20 && b <= 0x7E) ? String.fromCharCode(b) : '.';
            tdAsc.appendChild(sp);
        }
        tr.appendChild(tdAsc);

        memBody.appendChild(tr);
    }
    applySelection();
}

// ─── Selection ────────────────────────────────────────────────────────────────
function clearSelection() {
    selStart = selEnd = -1;
    selColumn = null;
    applySelection();
}

function selMin() { return Math.min(selStart, selEnd); }
function selMax() { return Math.max(selStart, selEnd); }

function applySelection() {
    document.querySelectorAll('.hb, .ab').forEach(el => {
        el.classList.remove('sel-primary', 'sel-secondary');
        if (selStart < 0) return;
        const idx = parseInt(el.dataset.idx, 10);
        if (idx < selMin() || idx > selMax()) return;
        const elCol = el.dataset.col;
        if (elCol === selColumn) {
            el.classList.add('sel-primary');
        } else {
            el.classList.add('sel-secondary');
        }
    });
    updateStatus();
}

function updateStatus() {
    if (selStart < 0 || currentBytes.length === 0) { statusBar.textContent = ''; return; }
    const lo  = selMin();
    const hi  = selMax();
    const len = hi - lo + 1;
    const baseAddr = currentAddress + lo;
    let info = \`\${len} byte\${len > 1 ? 's' : ''} selected  [\${baseAddr.toString(16).toUpperCase().padStart(4,'0')}–\${(currentAddress+hi).toString(16).toUpperCase().padStart(4,'0')}]\`;
    statusBar.textContent = info;
}

// ─── Mouse selection ──────────────────────────────────────────────────────────
memBody.addEventListener('mousedown', e => {
    const sp = e.target.closest('[data-idx]');
    if (!sp) return;
    e.preventDefault();
    const idx = parseInt(sp.dataset.idx, 10);
    const col = sp.dataset.col;
    selStart  = idx;
    selEnd    = idx;
    selColumn = col;
    dragging  = true;
    applySelection();
});

document.addEventListener('mousemove', e => {
    if (!dragging) return;
    const sp = document.elementFromPoint(e.clientX, e.clientY)?.closest('[data-idx]');
    if (!sp || sp.dataset.col !== selColumn) return;
    selEnd = parseInt(sp.dataset.idx, 10);
    applySelection();
});

document.addEventListener('mouseup', () => { dragging = false; });

// ─── Copy ─────────────────────────────────────────────────────────────────────
document.addEventListener('keydown', e => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'c') {
        if (selStart < 0) return;
        const lo    = selMin();
        const hi    = selMax();
        const slice = currentBytes.slice(lo, hi + 1);
        let text;
        if (selColumn === 'hex') {
            text = slice.map(b => b.toString(16).toUpperCase().padStart(2,'0')).join(' ');
        } else {
            text = slice.map(b => (b >= 0x20 && b <= 0x7E) ? String.fromCharCode(b) : '.').join('');
        }
        navigator.clipboard.writeText(text);
        statusBar.textContent += '  — copied!';
    }
    // Escape clears selection
    if (e.key === 'Escape') clearSelection();
});

// ─── Sources ─────────────────────────────────────────────────────────────────
function populateSources(list) {
    sources = list;
    sourceSelect.innerHTML = '';
    list.forEach((src, idx) => {
        const opt = document.createElement('option');
        opt.value = idx;
        opt.textContent = src.label;
        if (src.type === currentSource.type && src.bank === currentSource.bank) {
            opt.selected = true;
        }
        sourceSelect.appendChild(opt);
    });
    // Sync currentSource to actual selection
    const sel = sources[parseInt(sourceSelect.value, 10)];
    if (sel) currentSource = sel;
}

sourceSelect.addEventListener('change', () => {
    const sel = sources[parseInt(sourceSelect.value, 10)];
    if (!sel) return;
    currentSource = sel;
    // Clamp address to new source's address space
    if (currentAddress > sel.maxAddr) {
        currentAddress = 0;
        addrInput.value = '0000';
    }
    requestMemory(currentAddress);
});

// ─── Navigation ───────────────────────────────────────────────────────────────
function parseAddr(str) {
    const s   = str.trim().replace(/^0x/i,'');
    const val = parseInt(s, 16);
    return isNaN(val) ? null : val & currentSource.maxAddr;
}

function requestMemory(addr) {
    vscode.postMessage({ type: 'requestMemory', address: addr & currentSource.maxAddr, source: currentSource });
}

btnGo.addEventListener('click', () => {
    const a = parseAddr(addrInput.value);
    if (a !== null) requestMemory(a);
});

addrInput.addEventListener('keydown', e => {
    if (e.key === 'Enter') btnGo.click();
});

btnPrev.addEventListener('click', () => {
    const step = BYTES_PER_ROW * ROW_COUNT;
    const wrap = currentSource.maxAddr + 1;
    requestMemory(((currentAddress - step) + wrap) % wrap);
});

btnNext.addEventListener('click', () => {
    const step = BYTES_PER_ROW * ROW_COUNT;
    const wrap = currentSource.maxAddr + 1;
    requestMemory((currentAddress + step) % wrap);
});

btnRefresh.addEventListener('click', () => {
    requestMemory(currentAddress);
});

// ─── Message handler ──────────────────────────────────────────────────────────
window.addEventListener('message', e => {
    const msg = e.data;
    switch (msg.type) {
        case 'memSources':
            populateSources(msg.sources);
            break;
        case 'memoryData':
            render(msg.address, msg.bytes);
            break;
        case 'setAddress':
            addrInput.value = (msg.address).toString(16).toUpperCase().padStart(4, '0');
            currentAddress = msg.address;
            break;
        case 'error':
            errorMsg.textContent = 'Error: ' + msg.message;
            errorMsg.style.display = 'block';
            break;
    }
});

// ─── Build hex column header (same spans as data rows → perfect alignment) ────
(function buildHexHeader() {
    const th = document.getElementById('hexHeader');
    for (let col = 0; col < BYTES_PER_ROW; col++) {
        const sp = document.createElement('span');
        sp.className = 'hb hdr' + (col === 8 ? ' gap' : '');
        sp.textContent = col.toString(16).toUpperCase().padStart(2, '\u00A0'); // nbsp-pad
        th.appendChild(sp);
    }
})();

// ─── Init ─────────────────────────────────────────────────────────────────────
vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.MemoryViewPanel = MemoryViewPanel;
//# sourceMappingURL=MemoryViewPanel.js.map

/***/ }),
/* 31 */
/***/ ((__unused_webpack_module, exports) => {

"use strict";

Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.HardwarePanel = void 0;
// Base class for all hardware debug panels.
// Each subclass implements refresh() to fetch state via customRequest and update its webview.
// HardwarePanel.refreshAll() is called on every StoppedEvent.
class HardwarePanel {
    constructor(panel) {
        this._disposed = false;
        this._panel = panel;
        HardwarePanel._registry.add(this);
        this._panel.onDidDispose(() => {
            HardwarePanel._registry.delete(this);
            this._disposed = true;
            this.onDispose();
        });
    }
    onDispose() { }
    get isDisposed() { return this._disposed; }
    static async refreshAll() {
        for (const panel of HardwarePanel._registry) {
            try {
                await panel.refresh();
            }
            catch { /* ignore — session may be gone */ }
        }
    }
    // ── Shared CSS ────────────────────────────────────────────────────────────
    static commonCss() {
        return /* css */ `
  :root {
    --fg:         var(--vscode-editor-foreground, #ccc);
    --fg-dim:     var(--vscode-descriptionForeground, #888);
    --bg:         var(--vscode-editor-background, #1e1e1e);
    --bg-input:   var(--vscode-input-background, #3c3c3c);
    --border:     var(--vscode-panel-border, #444);
    --btn-bg:     var(--vscode-button-secondaryBackground, #3a3d41);
    --btn-fg:     var(--vscode-button-secondaryForeground, #ccc);
    --btn-hover:  var(--vscode-button-secondaryHoverBackground, #4a4d52);
    --diff-ins:   var(--vscode-diffEditor-insertedTextBackground, rgba(155,185,85,.2));
    --font:       var(--vscode-editor-font-family, 'Consolas','Courier New',monospace);
    --font-size:  var(--vscode-editor-font-size, 13px);
  }
  * { box-sizing: border-box; }
  body {
    background: var(--bg);
    color: var(--fg);
    font-family: var(--font);
    font-size: var(--font-size);
    margin: 0;
    padding: 8px;
  }
  /* Toolbar */
  .toolbar {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-bottom: 8px;
    flex-wrap: wrap;
  }
  .badge {
    display: inline-block;
    background: var(--vscode-badge-background, #4d4d4d);
    color: var(--vscode-badge-foreground, #fff);
    border-radius: 10px;
    padding: 1px 8px;
    font-size: 0.85em;
    font-family: var(--font);
  }
  button {
    font-family: var(--font);
    font-size: var(--font-size);
    background: var(--btn-bg);
    color: var(--btn-fg);
    border: 1px solid var(--border);
    padding: 2px 8px;
    cursor: pointer;
  }
  button:hover { background: var(--btn-hover); }
  /* Tables */
  table { border-collapse: collapse; width: 100%; }
  th {
    color: var(--fg-dim);
    font-weight: normal;
    text-align: left;
    padding: 0 8px 4px 0;
    border-bottom: 1px solid var(--border);
  }
  td {
    padding: 2px 8px 2px 0;
    vertical-align: top;
  }
  tr.changed td { background: var(--diff-ins); }
  /* Section titles */
  .section-title {
    color: var(--fg-dim);
    font-size: 0.85em;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    margin: 10px 0 4px 0;
    border-bottom: 1px solid var(--border);
    padding-bottom: 2px;
  }
  /* Error */
  .error {
    color: var(--vscode-errorForeground, #f48771);
    padding: 4px;
    display: none;
  }
  .mono { font-family: var(--font); }
  .dim  { color: var(--fg-dim); }`;
    }
}
exports.HardwarePanel = HardwarePanel;
HardwarePanel._registry = new Set();
//# sourceMappingURL=HardwarePanel.js.map

/***/ }),
/* 32 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.CrtcAsicPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
const CRTC_TYPE_NAMES = ["HD6845S/UM6845", "UM6845R", "MC6845", "AMS40489", "AMS40226"];
const CRTC_REG_NAMES = [
    "R0  Horizontal Total",
    "R1  H. Displayed",
    "R2  H. Sync Position",
    "R3  H+V Sync Widths",
    "R4  Vertical Total",
    "R5  Vertical Adjust",
    "R6  V. Displayed",
    "R7  V. Sync Position",
    "R8  Interlace & Skew",
    "R9  Max Raster Addr",
    "R10 Cursor Start",
    "R11 Cursor End",
    "R12 Disp. Start (H)",
    "R13 Disp. Start (L)",
    "R14 Cursor Addr (H)",
    "R15 Cursor Addr (L)",
    "R16 Light Pen (H)",
    "R17 Light Pen (L)",
];
class CrtcAsicPanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;
        if (CrtcAsicPanel.currentPanel) {
            CrtcAsicPanel.currentPanel._panel.reveal(column);
            CrtcAsicPanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80crtcPanel", "CRTC / ASIC", column, { enableScripts: true, retainContextWhenHidden: true });
        CrtcAsicPanel.currentPanel = new CrtcAsicPanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
        });
    }
    onDispose() {
        CrtcAsicPanel.currentPanel = undefined;
    }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const crtcResult = await session.customRequest("getCrtcState", {});
            if (crtcResult?.error) {
                this._panel.webview.postMessage({ type: "error", message: crtcResult.error });
                return;
            }
            this._panel.webview.postMessage({ type: "crtcState", state: crtcResult });
            if (crtcResult.isPlus) {
                const asicResult = await session.customRequest("getAsicState", {});
                if (!asicResult?.error) {
                    this._panel.webview.postMessage({ type: "asicState", state: asicResult });
                }
            }
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    _buildHtml() {
        const regNames = JSON.stringify(CRTC_REG_NAMES);
        const typeNames = JSON.stringify(CRTC_TYPE_NAMES);
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}
  .reg-binary { font-family: var(--font); letter-spacing: 0.05em; }
  .bit-masked { color: var(--fg-dim); }
  .counters-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
    gap: 4px 16px;
    margin-top: 4px;
  }
  .counter-row { display: flex; justify-content: space-between; }
  .counter-label { color: var(--fg-dim); }

  /* ── ASIC sections ── */
  #asicSections { margin-top: 4px; }

  /* ASIC registers grid */
  .asic-reg-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 4px 16px;
    margin-top: 4px;
  }
  .asic-reg-row { display: flex; justify-content: space-between; gap: 8px; }
  .asic-reg-label { color: var(--fg-dim); white-space: nowrap; }

  /* Sprite palette swatches */
  .spal-grid {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    margin-top: 4px;
  }
  .spal-cell {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
  }
  .spal-swatch {
    width: 22px;
    height: 22px;
    border-radius: 3px;
    border: 1px solid var(--border);
  }
  .spal-swatch.transparent-bg {
    background-image: linear-gradient(45deg, #555 25%, transparent 25%),
                      linear-gradient(-45deg, #555 25%, transparent 25%),
                      linear-gradient(45deg, transparent 75%, #555 75%),
                      linear-gradient(-45deg, transparent 75%, #555 75%);
    background-size: 8px 8px;
    background-position: 0 0, 0 4px, 4px -4px, -4px 0;
    background-color: #333;
  }
  .spal-label { font-size: 0.7em; color: var(--fg-dim); font-family: var(--font); }

  /* Sprite grid */
  .sprite-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(162px, 1fr));
    gap: 10px;
    margin-top: 4px;
  }
  .sprite-cell {
    border: 1px solid var(--border);
    border-radius: 3px;
    padding: 6px;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    background: var(--bg-input);
  }
  .sprite-cell.displayed { border-color: #4ec9b0; }
  .sprite-header {
    display: flex;
    justify-content: space-between;
    width: 100%;
    font-size: 0.8em;
  }
  .sprite-num { font-family: var(--font); font-weight: bold; }
  .dot-on  { color: #4ec9b0; }
  .dot-off { color: var(--fg-dim); }
  canvas.spr-canvas {
    /* 16×16 canvas scaled 8× → 128×128 CSS px = 8 CSS px per CPC pixel */
    width: 128px;
    height: 128px;
    image-rendering: pixelated;
    image-rendering: crisp-edges;
    border: 1px solid var(--border);
    /* checkerboard background visible through transparent pixels */
    background-image: linear-gradient(45deg, #555 25%, transparent 25%),
                      linear-gradient(-45deg, #555 25%, transparent 25%),
                      linear-gradient(45deg, transparent 75%, #555 75%),
                      linear-gradient(-45deg, transparent 75%, #555 75%);
    background-size: 16px 16px;
    background-position: 0 0, 0 8px, 8px -8px, -8px 0;
    background-color: #2a2a2a;
  }
  .sprite-info { font-size: 0.75em; font-family: var(--font); width: 100%; }
  .sprite-info div { display: flex; justify-content: space-between; }
  .sprite-info .lbl { color: var(--fg-dim); }

  /* DMA */
  .dma-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    margin-top: 4px;
  }
  .dma-channel {
    border: 1px solid var(--border);
    border-radius: 3px;
    padding: 6px;
    background: var(--bg-input);
  }
  .dma-title {
    font-size: 0.85em;
    font-weight: bold;
    margin-bottom: 4px;
    color: var(--fg-dim);
    border-bottom: 1px solid var(--border);
    padding-bottom: 2px;
  }
  .dma-row { display: flex; justify-content: space-between; font-size: 0.85em; }
  .dma-row .lbl { color: var(--fg-dim); }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badge" class="badge">CRTC</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Registers</div>
<table id="regTable">
  <thead>
    <tr>
      <th>Register</th>
      <th>Hex</th>
      <th>Binary</th>
    </tr>
  </thead>
  <tbody id="regBody"></tbody>
</table>

<div class="section-title">Internal Counters</div>
<div id="counters" class="counters-grid"></div>

<!-- ASIC-only sections, hidden until isPlus confirmed -->
<div id="asicSections" style="display:none">

  <div class="section-title">ASIC Registers</div>
  <div id="asicRegs" class="asic-reg-grid"></div>

  <div class="section-title">Sprite Palette</div>
  <div id="spritePalette" class="spal-grid"></div>

  <div class="section-title">Sprites (0–15)</div>
  <div id="spriteGrid" class="sprite-grid"></div>

  <div class="section-title">DMA Channels</div>
  <div id="dmaChannels" class="dma-grid"></div>

</div>

<script>
const vscode    = acquireVsCodeApi();
const REG_NAMES  = ${regNames};
const TYPE_NAMES = ${typeNames};

let prevRegs     = null;
let prevCounters = null;

function hex2(v)  { return (v & 0xFF).toString(16).toUpperCase().padStart(2,'0'); }
function hex4(v)  { return (v & 0xFFFF).toString(16).toUpperCase().padStart(4,'0'); }

// ── CRTC ─────────────────────────────────────────────────────────────────────

function renderBinary(value, mask) {
    let html = '';
    for (let bit = 7; bit >= 0; bit--) {
        const b = (value >> bit) & 1;
        const used = (mask >> bit) & 1;
        html += used
            ? \`<span>\${b}</span>\`
            : \`<span class="bit-masked">\${b}</span>\`;
        if (bit === 4) html += ' ';
    }
    return html;
}

function renderRegisters(state) {
    const regs  = state.registers;
    const masks = state.masks;
    const tbody = document.getElementById('regBody');
    tbody.innerHTML = '';
    for (let i = 0; i < 18; i++) {
        const val  = regs[i]  ?? 0;
        const mask = masks[i] ?? 0xFF;
        const changed = prevRegs && prevRegs[i] !== val;
        const tr = document.createElement('tr');
        if (changed) tr.className = 'changed';
        tr.innerHTML =
            \`<td class="dim">\${REG_NAMES[i]}</td>\` +
            \`<td class="mono">\${hex2(val)}</td>\` +
            \`<td class="reg-binary mono">\${renderBinary(val, mask)}</td>\`;
        tbody.appendChild(tr);
    }
    prevRegs = regs;
}

function counterChanged(key, value) {
    if (!prevCounters) return false;
    return prevCounters[key] !== value;
}

function renderCounters(state) {
    const items = [
        { label: 'HCC',        key: 'hcc',       fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'VLC',        key: 'vlc',       fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'VCC',        key: 'vcc',       fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'VA',         key: 'vertAdj',   fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'MA',         key: 'ma',        fmt: v => hex4(v) },
        { label: 'H.Pulse',    key: 'hPulse',    fmt: v => hex2(v) },
        { label: 'Vert.Pulse', key: 'vertPulse', fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'R52',        key: 'r52',       fmt: v => hex2(v) + ' (' + v + ')' },
        { label: 'Addr Reg',   key: 'addrReg',   fmt: v => hex2(v) },
        { label: 'Status',     key: 'statusReg', fmt: v => hex2(v) },
        { label: 'Beam X',     key: 'beamX',     fmt: v => String(v) },
        { label: 'Beam Y',     key: 'beamY',     fmt: v => String(v) },
    ];
    const grid = document.getElementById('counters');
    grid.innerHTML = '';
    for (const item of items) {
        const val = state[item.key] ?? 0;
        const changed = counterChanged(item.key, val);
        const div = document.createElement('div');
        div.className = 'counter-row' + (changed ? ' changed' : '');
        div.innerHTML =
            \`<span class="counter-label">\${item.label}</span>\` +
            \`<span class="mono">\${item.fmt(val)}</span>\`;
        grid.appendChild(div);
    }
    prevCounters = Object.fromEntries(items.map(i => [i.key, state[i.key] ?? 0]));
}

function applyCrtcState(state) {
    const crtcType = state.crtcType ?? 0;
    const isPlus   = state.isPlus  ?? false;
    const badge    = document.getElementById('badge');
    badge.textContent = isPlus
        ? 'ASIC (CPC+)'
        : 'CRTC ' + crtcType + ' — ' + (TYPE_NAMES[crtcType] ?? '?');

    document.getElementById('errorMsg').style.display = 'none';
    renderRegisters(state);
    renderCounters(state);

    // Show/hide ASIC sections
    document.getElementById('asicSections').style.display = isPlus ? 'block' : 'none';
}

// ── ASIC ─────────────────────────────────────────────────────────────────────

function argbToRgb(v) {
    return { r: (v >> 16) & 0xFF, g: (v >> 8) & 0xFF, b: v & 0xFF };
}

function renderAsicRegs(state) {
    const sccrRaw = state.sscr ?? 0;
    const hScroll = sccrRaw & 0x0F;
    const vScroll = (sccrRaw >> 4) & 0x07;
    const extBorder = (sccrRaw >> 7) & 1;

    const dcsr = state.dcsr ?? 0;

    const items = [
        { label: 'PRI',  value: '0x' + hex2(state.pri  ?? 0) + '  (sprite priority)' },
        { label: 'SPLT', value: '0x' + hex2(state.splt ?? 0) + '  (sprite split)' },
        { label: 'SSA',  value: '0x' + hex4(state.ssa  ?? 0) },
        { label: 'SSCR', value: '0x' + hex2(sccrRaw) + '  H=' + hScroll + ' V=' + vScroll + (extBorder ? ' ExtBdr' : '') },
        { label: 'IVR',  value: '0x' + hex2(state.ivr  ?? 0) + '  (interrupt vector)' },
        { label: 'DCSR', value: '0x' + hex2(dcsr) + '  CH' + ((dcsr & 1) ? '0' : '') + ((dcsr & 2) ? '1' : '') + ((dcsr & 4) ? '2' : '') + ' enabled' },
    ];

    const grid = document.getElementById('asicRegs');
    grid.innerHTML = '';
    for (const it of items) {
        const div = document.createElement('div');
        div.className = 'asic-reg-row';
        div.innerHTML = \`<span class="asic-reg-label">\${it.label}</span><span class="mono">\${it.value}</span>\`;
        grid.appendChild(div);
    }
}

function renderSpritePalette(palette) {
    const container = document.getElementById('spritePalette');
    container.innerHTML = '';
    for (let i = 0; i < 16; i++) {
        const { r, g, b } = argbToRgb(palette[i]);
        const cell = document.createElement('div');
        cell.className = 'spal-cell';
        const swatch = document.createElement('div');
        swatch.className = 'spal-swatch' + (i === 0 ? ' transparent-bg' : '');
        if (i !== 0) swatch.style.backgroundColor = \`rgb(\${r},\${g},\${b})\`;
        swatch.title = i === 0
            ? 'Color 0 — transparent'
            : \`Color \${i} — rgb(\${r},\${g},\${b})\`;
        const lbl = document.createElement('div');
        lbl.className = 'spal-label';
        lbl.textContent = String(i);
        cell.appendChild(swatch);
        cell.appendChild(lbl);
        container.appendChild(cell);
    }
}

function paintSprite(canvas, pixels, palette) {
    const ctx = canvas.getContext('2d');
    const img = ctx.createImageData(16, 16);
    for (let i = 0; i < 256; i++) {
        const ci = pixels[i] & 0xF;
        if (ci === 0) {
            // transparent
            img.data[i * 4 + 3] = 0;
        } else {
            const { r, g, b } = argbToRgb(palette[ci]);
            img.data[i * 4]     = r;
            img.data[i * 4 + 1] = g;
            img.data[i * 4 + 2] = b;
            img.data[i * 4 + 3] = 255;
        }
    }
    ctx.putImageData(img, 0, 0);
}

const ZOOM_NAMES = ['', '1×', '2×', '4×', '8×'];

function renderSprites(sprites, palette) {
    const grid = document.getElementById('spriteGrid');
    grid.innerHTML = '';
    for (let i = 0; i < 16; i++) {
        const spr = sprites[i];
        const cell = document.createElement('div');
        cell.className = 'sprite-cell' + (spr.displayed ? ' displayed' : '');

        const hdr = document.createElement('div');
        hdr.className = 'sprite-header';
        hdr.innerHTML =
            \`<span class="sprite-num">Spr \${i}</span>\` +
            \`<span class="\${spr.displayed ? 'dot-on' : 'dot-off'}">\${spr.displayed ? '●' : '○'}</span>\`;
        cell.appendChild(hdr);

        const canvas = document.createElement('canvas');
        canvas.className = 'spr-canvas';
        canvas.width  = 16;
        canvas.height = 16;
        paintSprite(canvas, spr.pixels, palette);
        cell.appendChild(canvas);

        const info = document.createElement('div');
        info.className = 'sprite-info';
        info.innerHTML =
            \`<div><span class="lbl">X</span><span>\${spr.x}</span></div>\` +
            \`<div><span class="lbl">Y</span><span>\${spr.y}</span></div>\` +
            \`<div><span class="lbl">Zoom</span><span>\${ZOOM_NAMES[spr.zoomx] ?? spr.zoomx}×\${ZOOM_NAMES[spr.zoomy] ?? spr.zoomy}</span></div>\`;
        cell.appendChild(info);

        grid.appendChild(cell);
    }
}

function renderDma(dmaChannels, dcsr) {
    const container = document.getElementById('dmaChannels');
    container.innerHTML = '';
    for (let c = 0; c < 3; c++) {
        const ch = dmaChannels[c];
        const enabled = !!(dcsr & (1 << c));
        const irqPending = ch.interrupt;

        const div = document.createElement('div');
        div.className = 'dma-channel';
        div.innerHTML =
            \`<div class="dma-title">DMA \${c} \${enabled ? '<span style="color:#4ec9b0">▶</span>' : '<span style="color:#888">■</span>'}\` +
            \`\${irqPending ? ' <span style="color:#f48771">IRQ</span>' : ''}</div>\` +
            \`<div class="dma-row"><span class="lbl">SAR</span><span class="mono">0x\${hex4(ch.sar)}</span></div>\` +
            \`<div class="dma-row"><span class="lbl">PPR</span><span class="mono">0x\${hex2(ch.ppr)}</span></div>\` +
            \`<div class="dma-row"><span class="lbl">Instr</span><span class="mono">0x\${hex4(ch.currentInstr)}</span></div>\` +
            \`<div class="dma-row"><span class="lbl">State</span><span>\${ch.paused ? 'PAUSE' : '—'}</span></div>\`;
        container.appendChild(div);
    }
}

function applyAsicState(state) {
    renderAsicRegs(state);
    renderSpritePalette(state.spritePalette ?? []);
    renderSprites(state.sprites ?? [], state.spritePalette ?? []);
    renderDma(state.dma ?? [], state.dcsr ?? 0);
}

// ── Event listeners ───────────────────────────────────────────────────────────

document.getElementById('btnRefresh').addEventListener('click', () => {
    vscode.postMessage({ type: 'refresh' });
});

window.addEventListener('message', e => {
    const msg = e.data;
    switch (msg.type) {
        case 'crtcState':
            applyCrtcState(msg.state);
            break;
        case 'asicState':
            applyAsicState(msg.state);
            break;
        case 'error':
            document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
            document.getElementById('errorMsg').style.display = 'block';
            break;
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.CrtcAsicPanel = CrtcAsicPanel;
//# sourceMappingURL=CrtcAsicPanel.js.map

/***/ }),
/* 33 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.GateArrayPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
class GateArrayPanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;
        if (GateArrayPanel.currentPanel) {
            GateArrayPanel.currentPanel._panel.reveal(column);
            GateArrayPanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80gaPanel", "Gate Array", column, { enableScripts: true, retainContextWhenHidden: true });
        GateArrayPanel.currentPanel = new GateArrayPanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
        });
    }
    onDispose() {
        GateArrayPanel.currentPanel = undefined;
    }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getGateArrayState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            }
            else {
                this._panel.webview.postMessage({ type: "gaState", state: result });
            }
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    _buildHtml() {
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}
  /* Palette grid */
  .palette-grid {
    display: grid;
    grid-template-columns: repeat(8, 1fr);
    gap: 4px;
    margin-top: 4px;
  }
  .swatch {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
    padding: 3px;
    border: 1px solid var(--border);
    border-radius: 3px;
    cursor: default;
  }
  .swatch.active-pen {
    border: 2px solid var(--vscode-focusBorder, #007fd4);
  }
  .swatch.changed {
    background: var(--diff-ins);
  }
  .color-box {
    width: 36px;
    height: 24px;
    border-radius: 2px;
    border: 1px solid rgba(255,255,255,0.15);
    flex-shrink: 0;
  }
  .swatch-label {
    font-size: 0.75em;
    color: var(--fg-dim);
    font-family: var(--font);
    text-align: center;
    line-height: 1.2;
  }
  /* Border swatch */
  .border-row {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-top: 4px;
  }
  .border-box {
    width: 60px;
    height: 24px;
    border-radius: 2px;
    border: 1px solid rgba(255,255,255,0.15);
  }
  /* Flags row */
  .flags-row {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
    margin-top: 4px;
  }
  .flag {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-size: 0.85em;
    padding: 1px 6px;
    border: 1px solid var(--border);
    border-radius: 3px;
  }
  .flag .dot {
    width: 7px; height: 7px;
    border-radius: 50%;
  }
  .dot-on  { background: var(--vscode-testing-iconPassed, #73c991); }
  .dot-off { background: var(--fg-dim); }
  .dot-warn { background: var(--vscode-testing-iconFailed, #f48771); }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badgeMode" class="badge">Mode ?</span>
  <span id="badgeIrq"  class="badge">IRQ —</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Ink Palette (INK 0–15)</div>
<div id="paletteGrid" class="palette-grid"></div>

<div class="section-title">Border</div>
<div class="border-row">
  <div id="borderBox" class="border-box"></div>
  <span id="borderHex" class="mono dim"></span>
</div>

<div class="section-title">Flags</div>
<div id="flagsRow" class="flags-row"></div>

<script>
const vscode = acquireVsCodeApi();

let prevInks   = null;
let prevBorder = null;

function argbToRgb(v) {
    const r = (v >>> 16) & 0xFF;
    const g = (v >>> 8)  & 0xFF;
    const b =  v         & 0xFF;
    return { r, g, b, css: \`rgb(\${r},\${g},\${b})\`, hex: '#' + r.toString(16).padStart(2,'0') + g.toString(16).padStart(2,'0') + b.toString(16).padStart(2,'0') };
}

function luminance(r, g, b) { return 0.299*r + 0.587*g + 0.114*b; }

function renderPalette(state) {
    const inks  = state.inks;   // array[16]
    const pen   = state.pen ?? 0;
    const grid  = document.getElementById('paletteGrid');
    grid.innerHTML = '';

    for (let i = 0; i < 16; i++) {
        const col     = argbToRgb(inks[i]);
        const changed = prevInks && prevInks[i] !== inks[i];
        const isActive = (i === pen);
        const lum     = luminance(col.r, col.g, col.b);
        const labelCol = lum > 128 ? '#000' : '#fff';

        const div = document.createElement('div');
        div.className = 'swatch' + (isActive ? ' active-pen' : '') + (changed ? ' changed' : '');
        div.title = \`INK \${i} — \${col.hex}\${isActive ? ' (selected pen)' : ''}\`;
        div.innerHTML =
            \`<div class="color-box" style="background:\${col.css}; color:\${labelCol}"></div>\` +
            \`<div class="swatch-label">\${i}<br>\${col.hex}</div>\`;
        grid.appendChild(div);
    }
    prevInks = inks.slice();
}

function renderBorder(state) {
    const col = argbToRgb(state.border ?? 0);
    const changed = prevBorder !== null && prevBorder !== state.border;
    document.getElementById('borderBox').style.background = col.css;
    const hexEl = document.getElementById('borderHex');
    hexEl.textContent = col.hex;
    if (changed) hexEl.style.background = 'var(--diff-ins)';
    else         hexEl.style.background = '';
    prevBorder = state.border;
}

function flag(label, on, warn) {
    const cls = warn ? 'dot-warn' : (on ? 'dot-on' : 'dot-off');
    return \`<span class="flag"><span class="dot \${cls}"></span>\${label}</span>\`;
}

function renderFlags(state) {
    const irqN = state.interruptCounter ?? 0;
    const irqR = state.interruptRaised  ?? false;

    document.getElementById('badgeMode').textContent = 'Mode ' + (state.mode ?? '?');
    document.getElementById('badgeIrq').textContent  = 'IRQ ' + irqN + (irqR ? ' !' : '');

    const row = document.getElementById('flagsRow');
    row.innerHTML =
        flag('IRQ Raised', irqR, irqR) +
        flag('ASIC Locked', state.asicLocked ?? false, false);
}

function applyState(state) {
    document.getElementById('errorMsg').style.display = 'none';
    renderPalette(state);
    renderBorder(state);
    renderFlags(state);
}

document.getElementById('btnRefresh').addEventListener('click', () => {
    vscode.postMessage({ type: 'refresh' });
});

window.addEventListener('message', e => {
    const msg = e.data;
    if      (msg.type === 'gaState') applyState(msg.state);
    else if (msg.type === 'error') {
        document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
        document.getElementById('errorMsg').style.display = 'block';
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.GateArrayPanel = GateArrayPanel;
//# sourceMappingURL=GateArrayPanel.js.map

/***/ }),
/* 34 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.PsgPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
const AY_CLOCK = 1000000; // CPC AY clock = 1 MHz
// Envelope shape descriptions + ASCII waveform (4-bit shape value)
const ENV_SHAPES = [
    { name: "Fall×1", wave: "\\" }, // 0
    { name: "Fall×1", wave: "\\" }, // 1
    { name: "Fall×1", wave: "\\" }, // 2
    { name: "Fall×1", wave: "\\" }, // 3
    { name: "Fall-Hold↓", wave: "\\_" }, // 4
    { name: "Fall-Rise×∞", wave: "\\/~" }, // 5
    { name: "Fall-Hold↑", wave: "\\‾" }, // 6
    { name: "Fall×1", wave: "\\" }, // 7
    { name: "Rise×∞", wave: "///~" }, // 8
    { name: "Rise-Hold↑", wave: "/‾" }, // 9
    { name: "Rise-Fall×∞", wave: "/\\/~" }, // 10
    { name: "Rise-Hold↓", wave: "/_" }, // 11
    { name: "Rise×∞", wave: "///~" }, // 12
    { name: "Rise-Hold↑", wave: "/‾" }, // 13
    { name: "Rise-Fall×∞", wave: "/\\/~" }, // 14
    { name: "Rise-Hold↓", wave: "/_" }, // 15
];
class PsgPanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;
        if (PsgPanel.currentPanel) {
            PsgPanel.currentPanel._panel.reveal(column);
            PsgPanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80psgPanel", "PSG (AY-3-8912)", column, { enableScripts: true, retainContextWhenHidden: true });
        PsgPanel.currentPanel = new PsgPanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
        });
    }
    onDispose() {
        PsgPanel.currentPanel = undefined;
    }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getPsgState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            }
            else {
                this._panel.webview.postMessage({ type: "psgState", state: result });
            }
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    _buildHtml() {
        const envShapesJson = JSON.stringify(ENV_SHAPES);
        const ayClockJson = JSON.stringify(AY_CLOCK);
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}
  /* Channel table */
  .ch-table { width: 100%; border-collapse: collapse; margin-top: 4px; }
  .ch-table th { color: var(--fg-dim); font-weight: normal; text-align: left; padding: 0 8px 4px 0; border-bottom: 1px solid var(--border); }
  .ch-table td { padding: 3px 8px 3px 0; vertical-align: middle; }
  .ch-table tr.changed td { background: var(--diff-ins); }
  .pill {
    display: inline-block;
    padding: 0 5px;
    border-radius: 3px;
    font-size: 0.8em;
    font-family: var(--font);
  }
  .pill-on  { background: rgba(115,201,145,.25); color: #73c991; }
  .pill-off { background: rgba(255,255,255,.06); color: var(--fg-dim); }
  .pill-env { background: rgba(98,174,239,.25);  color: #62aeef; }
  /* Raw register grid */
  .reg-grid {
    display: grid;
    grid-template-columns: repeat(8, 1fr);
    gap: 2px 4px;
    margin-top: 4px;
  }
  .reg-cell {
    border: 1px solid var(--border);
    border-radius: 2px;
    padding: 2px 4px;
    text-align: center;
    font-family: var(--font);
    font-size: 0.85em;
  }
  .reg-cell.changed { background: var(--diff-ins); }
  .reg-cell-label { color: var(--fg-dim); font-size: 0.75em; text-align: center; }
  /* Envelope */
  .env-row { display: flex; align-items: center; gap: 12px; margin-top: 4px; flex-wrap: wrap; }
  .env-wave {
    font-family: var(--font);
    font-size: 1.1em;
    letter-spacing: 0.1em;
    background: var(--bg-input);
    padding: 2px 8px;
    border-radius: 3px;
    border: 1px solid var(--border);
  }
  /* Noise / misc row */
  .misc-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
    gap: 4px 16px;
    margin-top: 4px;
  }
  .misc-row { display: flex; justify-content: space-between; }
  .misc-label { color: var(--fg-dim); }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badgeMixer" class="badge">PSG</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Channels</div>
<table class="ch-table" id="chTable">
  <thead>
    <tr>
      <th>Ch</th>
      <th>Period</th>
      <th>Hz</th>
      <th>Vol</th>
      <th>Tone</th>
      <th>Noise</th>
      <th>Env</th>
    </tr>
  </thead>
  <tbody id="chBody"></tbody>
</table>

<div class="section-title">Noise</div>
<div class="misc-grid" id="noiseRow"></div>

<div class="section-title">Envelope</div>
<div class="env-row" id="envRow"></div>

<div class="section-title">Raw Registers</div>
<div style="display:grid; grid-template-columns: repeat(8,1fr); gap:2px 4px; margin-top:4px;" id="regLabels"></div>
<div id="regGrid" class="reg-grid"></div>

<script>
const vscode     = acquireVsCodeApi();
const ENV_SHAPES = ${envShapesJson};
const AY_CLOCK   = ${ayClockJson};

let prevRegs = null;
let prevCh   = [null, null, null];
let prevEnv  = null;
let prevNoise = null;

function hex2(v) { return (v & 0xFF).toString(16).toUpperCase().padStart(2,'0'); }
function hz(period, divider) {
    if (!period) return '—';
    return (AY_CLOCK / (divider * period)).toFixed(1) + ' Hz';
}

function pill(label, on) {
    return \`<span class="pill \${on ? 'pill-on' : 'pill-off'}">\${label}</span>\`;
}

function renderChannels(state) {
    const regs  = state.registers;
    const mixer = state.mixer ?? 0;
    const freqs = [state.chanAFreq, state.chanBFreq, state.chanCFreq];
    const vols  = [state.chanAVol,  state.chanBVol,  state.chanCVol];
    const labels = ['A','B','C'];
    const tbody  = document.getElementById('chBody');
    tbody.innerHTML = '';

    for (let i = 0; i < 3; i++) {
        const period  = freqs[i] & 0xFFF;
        const vol     = vols[i] & 0xF;
        const envMode = !!(vols[i] & 0x10);
        const toneOn  = !((mixer >> i) & 1);
        const noiseOn = !((mixer >> (i + 3)) & 1);

        const prevPeriod = prevCh[i]?.period;
        const prevVol    = prevCh[i]?.vol;
        const changed    = prevCh[i] !== null && (prevPeriod !== period || prevVol !== vols[i]);

        const tr = document.createElement('tr');
        if (changed) tr.className = 'changed';
        tr.innerHTML =
            \`<td><b>\${labels[i]}</b></td>\` +
            \`<td class="mono">\${period} (0x\${period.toString(16).toUpperCase().padStart(3,'0')})</td>\` +
            \`<td class="mono">\${hz(period, 16)}</td>\` +
            \`<td class="mono">\${envMode ? '~' : vol}</td>\` +
            \`<td>\${pill('T', toneOn)}</td>\` +
            \`<td>\${pill('N', noiseOn)}</td>\` +
            \`<td>\${envMode ? '<span class="pill pill-env">ENV</span>' : ''}</td>\`;
        tbody.appendChild(tr);

        prevCh[i] = { period, vol: vols[i] };
    }
}

function renderNoise(state) {
    const period  = (state.noiseFreq ?? 0) & 0x1F;
    const changed = prevNoise !== null && prevNoise !== period;
    const row = document.getElementById('noiseRow');
    row.innerHTML =
        \`<div class="misc-row\${changed ? ' changed' : ''}"><span class="misc-label">Period</span><span class="mono">\${period}</span></div>\` +
        \`<div class="misc-row"><span class="misc-label">Hz</span><span class="mono">\${hz(period, 16)}</span></div>\`;
    prevNoise = period;
}

function renderEnvelope(state) {
    const period  = state.envFreq  ?? 0;
    const shape   = (state.envShape ?? 0) & 0xF;
    const info    = ENV_SHAPES[shape];
    const changed = prevEnv !== null && (prevEnv.period !== period || prevEnv.shape !== shape);

    const row = document.getElementById('envRow');
    row.innerHTML =
        \`<div class="\${changed ? 'changed' : ''}">
           <div class="misc-row"><span class="misc-label">Period&nbsp;</span><span class="mono">\${period}</span></div>
           <div class="misc-row"><span class="misc-label">Hz&nbsp;&nbsp;&nbsp;&nbsp;</span><span class="mono">\${hz(period, 256)}</span></div>
           <div class="misc-row"><span class="misc-label">Shape&nbsp;</span><span class="mono">R13=\${hex2(shape)}</span></div>
         </div>\` +
        \`<div>
           <div class="env-wave">\${info.wave}</div>
           <div class="dim" style="font-size:0.85em;margin-top:2px">\${info.name}</div>
         </div>\`;
    prevEnv = { period, shape };
}

function renderRegisters(state) {
    const regs = state.registers; // array[16]
    const labels = document.getElementById('regLabels');
    const grid   = document.getElementById('regGrid');
    labels.innerHTML = '';
    grid.innerHTML   = '';

    for (let i = 0; i < 16; i++) {
        const lbl = document.createElement('div');
        lbl.className   = 'reg-cell-label';
        lbl.textContent = 'R' + i;
        labels.appendChild(lbl);

        const cell = document.createElement('div');
        const changed = prevRegs && prevRegs[i] !== regs[i];
        cell.className   = 'reg-cell' + (changed ? ' changed' : '');
        cell.textContent = hex2(regs[i]);
        cell.title       = 'R' + i + ' = ' + hex2(regs[i]) + ' (' + regs[i] + ')';
        grid.appendChild(cell);
    }
    prevRegs = regs.slice();
}

function applyState(state) {
    document.getElementById('errorMsg').style.display = 'none';
    const mixer = state.mixer ?? 0;
    document.getElementById('badgeMixer').textContent = 'PSG  R7=' + hex2(mixer);
    renderChannels(state);
    renderNoise(state);
    renderEnvelope(state);
    renderRegisters(state);
}

document.getElementById('btnRefresh').addEventListener('click', () => {
    vscode.postMessage({ type: 'refresh' });
});

window.addEventListener('message', e => {
    const msg = e.data;
    if      (msg.type === 'psgState') applyState(msg.state);
    else if (msg.type === 'error') {
        document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
        document.getElementById('errorMsg').style.display = 'block';
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.PsgPanel = PsgPanel;
//# sourceMappingURL=PsgPanel.js.map

/***/ }),
/* 35 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.PpiPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
// Port B bit descriptions (CPC hardware)
const PORT_B_BITS = [
    { bit: 0, label: "VSYNC", desc: (v) => v ? "in VSYNC" : "no VSYNC" },
    { bit: 1, label: "Type bit 1", desc: (v) => v ? "1" : "0" },
    { bit: 2, label: "Type bit 2", desc: (v) => v ? "1" : "0" },
    { bit: 3, label: "Type bit 3", desc: (v) => v ? "1" : "0" },
    { bit: 4, label: "Screen (LK4)", desc: (v) => v ? "60 Hz" : "50 Hz" },
    { bit: 5, label: "/EXP", desc: (v) => v ? "inactive" : "active" },
    { bit: 6, label: "Printer BUSY", desc: (v) => v ? "busy" : "ready" },
    { bit: 7, label: "CAS.IN", desc: (v) => v ? "high" : "low" },
];
const PSG_MODES = ["Inactive", "Read register", "Write register", "Latch address"];
class PpiPanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;
        if (PpiPanel.currentPanel) {
            PpiPanel.currentPanel._panel.reveal(column);
            PpiPanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80ppiPanel", "PPI (8255)", column, { enableScripts: true, retainContextWhenHidden: true });
        PpiPanel.currentPanel = new PpiPanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
        });
    }
    onDispose() {
        PpiPanel.currentPanel = undefined;
    }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getPpiState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            }
            else {
                this._panel.webview.postMessage({ type: "ppiState", state: result });
            }
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    _buildHtml() {
        const portBBits = JSON.stringify(PORT_B_BITS.map(b => ({ bit: b.bit, label: b.label })));
        const psgModes = JSON.stringify(PSG_MODES);
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}
  /* Port block */
  .port-block {
    margin-top: 4px;
    border: 1px solid var(--border);
    border-radius: 3px;
    padding: 6px 10px;
    background: var(--bg-input);
  }
  .port-header {
    display: flex;
    align-items: baseline;
    gap: 12px;
    margin-bottom: 6px;
  }
  .port-name  { font-weight: bold; font-size: 1em; }
  .port-hex   { font-family: var(--font); font-size: 1.1em; }
  .port-dir   {
    font-size: 0.8em;
    padding: 1px 6px;
    border-radius: 8px;
    background: rgba(255,255,255,.06);
    color: var(--fg-dim);
  }
  .port-dir.out { background: rgba(115,201,145,.15); color: #73c991; }
  .port-dir.in  { background: rgba(98,174,239,.15);  color: #62aeef; }

  /* Binary display with bit cells */
  .bits-row {
    display: flex;
    gap: 2px;
    margin-bottom: 6px;
    font-family: var(--font);
    font-size: 0.9em;
  }
  .bit-cell {
    width: 22px;
    text-align: center;
    border: 1px solid var(--border);
    border-radius: 2px;
    padding: 2px 0;
    cursor: default;
  }
  .bit-cell.set   { background: rgba(115,201,145,.25); color: #73c991; border-color: #73c991; }
  .bit-cell.clear { background: rgba(255,255,255,.04); color: var(--fg-dim); }
  .bit-sep { width: 4px; }
  .bit-labels {
    display: flex;
    gap: 2px;
    margin-bottom: 2px;
    font-family: var(--font);
    font-size: 0.7em;
    color: var(--fg-dim);
  }
  .bit-labels span { width: 22px; text-align: center; }
  .bit-labels .bit-sep { width: 4px; }

  /* Decoded fields table */
  .decoded-table { width: 100%; border-collapse: collapse; margin-top: 2px; }
  .decoded-table td { padding: 1px 6px 1px 0; font-size: 0.85em; }
  .decoded-table td:first-child { color: var(--fg-dim); width: 110px; white-space: nowrap; }
  .decoded-table tr.active td:last-child { color: #73c991; }

  /* Control word */
  .ctrl-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 3px 16px;
    margin-top: 4px;
    font-size: 0.9em;
  }
  .ctrl-row { display: flex; justify-content: space-between; }
  .ctrl-label { color: var(--fg-dim); }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badge" class="badge">PPI 8255</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Control Word</div>
<div id="ctrlWord" class="ctrl-grid"></div>

<div class="section-title">Port A — PSG data bus</div>
<div id="portA" class="port-block"></div>

<div class="section-title">Port B — Inputs</div>
<div id="portB" class="port-block"></div>

<div class="section-title">Port C — Outputs</div>
<div id="portC" class="port-block"></div>

<script>
const vscode   = acquireVsCodeApi();
const PORT_B_BITS_DATA = ${portBBits};
const PSG_MODES_DATA   = ${psgModes};

let prev = null;

function hex2(v) { return (v & 0xFF).toString(16).toUpperCase().padStart(2, '0'); }

function bitsHtml(value, prevValue) {
    let labels = '<div class="bit-labels">';
    let cells  = '<div class="bits-row">';
    for (let bit = 7; bit >= 0; bit--) {
        const v   = (value >> bit) & 1;
        const cls = v ? 'set' : 'clear';
        const changed = prevValue !== null && ((prevValue >> bit) & 1) !== v;
        labels += \`<span>\${bit}</span>\`;
        cells  += \`<div class="bit-cell \${cls}\${changed ? ' changed' : ''}" title="bit \${bit} = \${v}">\${v}</div>\`;
        if (bit === 4) {
            labels += '<span class="bit-sep"></span>';
            cells  += '<div class="bit-sep"></div>';
        }
    }
    labels += '</div>';
    cells  += '</div>';
    return labels + cells;
}

function dirBadge(isInput) {
    return \`<span class="port-dir \${isInput ? 'in' : 'out'}">\${isInput ? '← IN' : '→ OUT'}</span>\`;
}

function renderPortA(portA, cw, prevA) {
    const isInput = !!(cw & 0x10);
    const changed = prev && prev.portA !== portA;
    const container = document.getElementById('portA');
    container.innerHTML =
        \`<div class="port-header">
           <span class="port-name">A</span>
           <span class="port-hex mono\${changed ? ' changed' : ''}">0x\${hex2(portA)}</span>
           \${dirBadge(isInput)}
         </div>\` +
        bitsHtml(portA, prevA);
}

function renderPortB(portB, prevB) {
    const changed = prev && prev.portB !== portB;
    const container = document.getElementById('portB');

    let rows = '';
    for (const { bit, label } of PORT_B_BITS_DATA) {
        const v = (portB >> bit) & 1;
        rows += \`<tr><td>bit \${bit} \${label}</td><td class="mono">\${v}</td></tr>\`;
    }

    container.innerHTML =
        \`<div class="port-header">
           <span class="port-name">B</span>
           <span class="port-hex mono\${changed ? ' changed' : ''}">0x\${hex2(portB)}</span>
           \${dirBadge(true)}
         </div>\` +
        bitsHtml(portB, prevB) +
        \`<table class="decoded-table"><tbody>\${rows}</tbody></table>\`;
}

function renderPortC(portC, cw, prevC) {
    // bits 3:0 direction from control word bit 0 (C low)
    // bits 7:4 direction from control word bit 3 (C high)
    const lowIn  = !!(cw & 0x01);
    const highIn = !!(cw & 0x08);

    const kbRow   = portC & 0x0F;
    const motor   = !!(portC & 0x10);
    const casWr   = !!(portC & 0x20);
    const psgMode = (portC >> 6) & 0x03;
    const changed = prev && prev.portC !== portC;

    const container = document.getElementById('portC');
    container.innerHTML =
        \`<div class="port-header">
           <span class="port-name">C</span>
           <span class="port-hex mono\${changed ? ' changed' : ''}">0x\${hex2(portC)}</span>
           <span class="port-dir">low:\${lowIn ? 'IN' : 'OUT'} / high:\${highIn ? 'IN' : 'OUT'}</span>
         </div>\` +
        bitsHtml(portC, prevC) +
        \`<table class="decoded-table">
           <tr><td>bits 3:0  Kbd row</td><td class="mono">\${kbRow} (0x\${kbRow.toString(16).toUpperCase()})</td></tr>
           <tr\${motor ? ' class="active"' : ''}><td>bit 4  Cass. motor</td><td>\${motor ? '▶ ON' : '■ OFF'}</td></tr>
           <tr><td>bit 5  Cass. write</td><td class="mono">\${casWr ? '1' : '0'}</td></tr>
           <tr><td>bits 7:6  PSG ctrl</td><td>\${PSG_MODES_DATA[psgMode]}</td></tr>
         </table>\`;
}

function renderControlWord(cw, prevCw) {
    const modeAval  = (cw >> 5) & 0x03;
    const modeAstr  = ['Mode 0', 'Mode 1', 'Mode 2', 'Mode 2'][modeAval];
    const modeBstr  = (cw & 0x04) ? 'Mode 1' : 'Mode 0';
    const ioA       = (cw & 0x10) ? 'Input' : 'Output';
    const ioB       = (cw & 0x02) ? 'Input' : 'Output';
    const ioClow    = (cw & 0x01) ? 'Input' : 'Output';
    const ioChi     = (cw & 0x08) ? 'Input' : 'Output';
    const changed   = prev && prev.controlWord !== cw;

    const items = [
        { label: 'Control word', value: '0x' + hex2(cw) + (changed ? ' *' : '') },
        { label: 'Group A mode', value: modeAstr },
        { label: 'Group B mode', value: modeBstr },
        { label: 'Port A',       value: ioA },
        { label: 'Port B',       value: ioB },
        { label: 'Port C low',   value: ioClow },
        { label: 'Port C high',  value: ioChi },
    ];

    const grid = document.getElementById('ctrlWord');
    grid.innerHTML = '';
    for (const it of items) {
        const div = document.createElement('div');
        div.className = 'ctrl-row';
        div.innerHTML = \`<span class="ctrl-label">\${it.label}</span><span class="mono">\${it.value}</span>\`;
        grid.appendChild(div);
    }
}

function applyState(state) {
    document.getElementById('errorMsg').style.display = 'none';
    const prevA  = prev ? prev.portA  : null;
    const prevB  = prev ? prev.portB  : null;
    const prevC  = prev ? prev.portC  : null;

    renderControlWord(state.controlWord, prev ? prev.controlWord : null);
    renderPortA(state.portA, state.controlWord, prevA);
    renderPortB(state.portB, prevB);
    renderPortC(state.portC, state.controlWord, prevC);
    prev = state;
}

document.getElementById('btnRefresh').addEventListener('click', () => {
    vscode.postMessage({ type: 'refresh' });
});

window.addEventListener('message', e => {
    const msg = e.data;
    if      (msg.type === 'ppiState') applyState(msg.state);
    else if (msg.type === 'error') {
        document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
        document.getElementById('errorMsg').style.display = 'block';
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.PpiPanel = PpiPanel;
//# sourceMappingURL=PpiPanel.js.map

/***/ }),
/* 36 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.FdcPanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
const MSR_BITS = [
    { bit: 7, label: "RQM", desc: "Request for Master" },
    { bit: 6, label: "DIO", desc: "Data direction (1=to CPU)" },
    { bit: 5, label: "NDM", desc: "Non-DMA mode" },
    { bit: 4, label: "CB", desc: "FDC busy" },
    { bit: 3, label: "D3B", desc: "Drive 3 busy" },
    { bit: 2, label: "D2B", desc: "Drive 2 busy" },
    { bit: 1, label: "D1B", desc: "Drive 1 busy" },
    { bit: 0, label: "D0B", desc: "Drive 0 busy" },
];
const ST1_BITS = [
    { bit: 7, label: "EN", desc: "End of cylinder" },
    { bit: 5, label: "DE", desc: "Data error (CRC)" },
    { bit: 4, label: "OR", desc: "Overrun" },
    { bit: 2, label: "ND", desc: "No data" },
    { bit: 1, label: "NW", desc: "Not writable" },
    { bit: 0, label: "MA", desc: "Missing address mark" },
];
const ST2_BITS = [
    { bit: 6, label: "CM", desc: "Control mark (deleted)" },
    { bit: 5, label: "DD", desc: "Data CRC error" },
    { bit: 4, label: "WC", desc: "Wrong cylinder" },
    { bit: 2, label: "BC", desc: "Bad cylinder" },
    { bit: 1, label: "SNS", desc: "Scan not satisfied" },
    { bit: 0, label: "MAM", desc: "Missing AM in data" },
];
class FdcPanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside : vscode.ViewColumn.One;
        if (FdcPanel.currentPanel) {
            FdcPanel.currentPanel._panel.reveal(column);
            FdcPanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80fdcPanel", "FDC (µPD765)", column, { enableScripts: true, retainContextWhenHidden: true });
        FdcPanel.currentPanel = new FdcPanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
            else if (msg.type === "loadRaw") {
                await this._loadRaw(msg.drive, msg.side, msg.track);
            }
        });
    }
    onDispose() { FdcPanel.currentPanel = undefined; }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getFdcState", {});
            if (result?.error)
                this._panel.webview.postMessage({ type: "error", message: result.error });
            else
                this._panel.webview.postMessage({ type: "fdcState", state: result });
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    async _loadRaw(drive, side, track) {
        const session = vscode.debug.activeDebugSession;
        if (!session)
            return;
        try {
            this._panel.webview.postMessage({ type: "rawLoading" });
            const result = await session.customRequest("getTrackRaw", { drive, side, track });
            if (result?.error)
                this._panel.webview.postMessage({ type: "rawError", message: result.error });
            else
                this._panel.webview.postMessage({ type: "rawTrack", data: result });
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "rawError", message: String(e) });
        }
    }
    _buildHtml() {
        const msrBits = JSON.stringify(MSR_BITS);
        const st1Bits = JSON.stringify(ST1_BITS);
        const st2Bits = JSON.stringify(ST2_BITS);
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}

  .sr-row  { display:flex; gap:3px; margin:4px 0; flex-wrap:wrap; }
  .sr-bit  { border:1px solid var(--border); border-radius:3px; padding:2px 5px;
             font-size:.8em; font-family:var(--font); cursor:default; min-width:30px; text-align:center; }
  .sr-bit.set   { background:rgba(115,201,145,.25); color:#73c991; border-color:#73c991; }
  .sr-bit.clear { background:rgba(255,255,255,.04); color:var(--fg-dim); }
  .sr-hex { font-family:var(--font); }

  .drives-grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:4px; }
  .drive-card  { border:1px solid var(--border); border-radius:4px; padding:8px 10px; background:var(--bg-input); }
  .drive-card.active  { border-color:#4ec9b0; }
  .drive-card.no-disk { opacity:.55; }
  .drive-title { display:flex; align-items:center; gap:8px; margin-bottom:6px; font-weight:bold; }
  .motor-on    { color:#73c991; font-size:.8em; }
  .motor-off   { color:var(--fg-dim); font-size:.8em; }
  .wp-badge    { color:#f48771; font-size:.75em; padding:1px 5px; border:1px solid #f48771; border-radius:8px; }
  .drive-row   { display:flex; justify-content:space-between; font-size:.85em; margin:2px 0; }
  .drive-lbl   { color:var(--fg-dim); }
  .drive-path  { font-size:.75em; color:var(--fg-dim); word-break:break-all; margin-top:4px; }

  .track-map   { margin:6px 0 2px; height:18px; position:relative; background:var(--bg);
                 border:1px solid var(--border); border-radius:2px; overflow:hidden; }
  .track-map-seg { position:absolute; top:0; height:100%; box-sizing:border-box; border-right:1px solid var(--bg-input); }
  .seg-ok      { background:rgba(115,201,145,.45); }
  .seg-err     { background:rgba(244,135,113,.55); }
  .seg-deleted { background:rgba(206,145,120,.45); }
  .seg-label   { position:absolute; top:1px; font-size:.65em; font-family:var(--font);
                 color:rgba(255,255,255,.75); white-space:nowrap; pointer-events:none; overflow:hidden; padding:0 1px; }

  .sectors-table     { width:100%; border-collapse:collapse; font-size:.82em; margin-top:4px; }
  .sectors-table th  { color:var(--fg-dim); font-weight:normal; text-align:left; padding:1px 5px 3px 0;
                       border-bottom:1px solid var(--border); white-space:nowrap; }
  .sectors-table td  { padding:2px 5px 2px 0; font-family:var(--font); }
  .sectors-table tr.crc-err td { color:#f48771; }
  .sectors-table tr.deleted td { color:rgba(206,145,120,.9); font-style:italic; }
  .crc-ok  { color:#73c991; }
  .crc-err { color:#f48771; }
  .st-flag { display:inline-block; padding:0 3px; border-radius:2px; font-size:.75em;
             background:rgba(244,135,113,.2); color:#f48771; margin:0 1px; }

  /* ── Raw track viewer ── */
  .raw-viewer { margin-top:10px; border-top:1px solid var(--border); padding-top:8px; }
  .raw-controls { display:flex; flex-wrap:wrap; align-items:center; gap:8px; margin-bottom:6px; }
  .raw-controls label { color:var(--fg-dim); font-size:.85em; }
  .raw-controls input[type=number] { width:50px; background:var(--bg-input); color:var(--fg);
    border:1px solid var(--border); padding:2px 4px; font-family:var(--font); font-size:.9em; }
  .raw-controls input[type=range]  { width:100px; }
  .tab-bar  { display:flex; gap:2px; margin-bottom:6px; }
  .tab-btn  { padding:2px 10px; border:1px solid var(--border); cursor:pointer; font-size:.85em;
              font-family:var(--font); background:var(--bg-input); color:var(--fg-dim); }
  .tab-btn.active { background:var(--btn-hover); color:var(--fg); border-color:var(--fg-dim); }
  .raw-status { font-size:.8em; color:var(--fg-dim); padding:4px 0; }

  /* Hex view */
  .hex-view { overflow:auto; max-height:600px; font-family:var(--font); font-size:.8em;
              line-height:1.5; white-space:pre; background:var(--bg); border:1px solid var(--border);
              border-radius:2px; padding:4px 6px; }
  .hex-row    { display:flex; gap:1px; margin-bottom:1px; }
  .hex-offset { color:var(--fg-dim); min-width:56px; user-select:none; }
  .hex-bytes  { display:flex; flex-wrap:wrap; gap:1px; }
  .hb { display:inline-block; padding:0 1px; border-radius:1px; cursor:default; }
  /* region colors */
  .h-gap1  { color:#4e9a06; }
  .h-gap0  { color:#444; }
  .h-sync  { color:#4fc3f7; font-weight:bold; }
  .h-idam  { color:#f6d32d; font-weight:bold; }
  .h-chrn  { color:#ff9800; }
  .h-hcrc  { color:#ce93d8; }
  .h-dam   { color:#ffab40; font-weight:bold; }
  .h-data  { color:var(--fg); }
  .h-dcrc  { color:#ce93d8; }
  .h-weak  { background:rgba(246,211,45,.18); text-decoration:underline dotted #f6d32d; }
  .h-crcerr { text-decoration:underline wavy #e01b24; }

  /* MFM canvas */
  .mfm-wrap { overflow:auto; max-height:600px; background:var(--bg);
              border:1px solid var(--border); border-radius:2px; padding:4px; }
  #mfmCanvas { display:block; }
  .mfm-legend { display:flex; flex-wrap:wrap; gap:8px; margin-top:4px; font-size:.75em; }
  .leg-item   { display:flex; align-items:center; gap:3px; }
  .leg-swatch { width:12px; height:12px; border-radius:2px; }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badge" class="badge">FDC µPD765</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Main Status</div>
<div id="msrRow" class="sr-row"></div>

<div class="section-title">Drives</div>
<div id="drivesGrid" class="drives-grid"></div>

<!-- Raw track viewer -->
<div class="raw-viewer">
  <div class="section-title">Raw Track Viewer</div>
  <div class="raw-controls">
    <label>Drive
      <select id="rawDrive" style="margin-left:4px;background:var(--bg-input);color:var(--fg);border:1px solid var(--border);padding:2px 4px;font-family:var(--font);">
        <option value="0">A</option>
        <option value="1">B</option>
      </select>
    </label>
    <label>Track <input id="rawTrack" type="number" min="0" max="83" value="0"></label>
    <label>Side  <input id="rawSide"  type="number" min="0" max="1"  value="0"></label>
    <button id="btnLoadRaw">&#x2193; Load</button>
    <span id="rawStatus" class="raw-status"></span>
  </div>
  <div class="tab-bar">
    <div class="tab-btn active" id="tabHex" data-tab="hex">Hex</div>
    <div class="tab-btn"        id="tabMfm" data-tab="mfm">MFM Bits</div>
  </div>
  <div id="hexControls" style="display:flex;align-items:center;gap:8px;margin-bottom:4px;font-size:.8em;">
    <label style="color:var(--fg-dim)">Bit offset
      <input id="bitOffset" type="range" min="0" max="15" value="0" style="width:120px;margin:0 4px;">
      <span id="bitOffsetVal" style="font-family:var(--font);min-width:16px;display:inline-block;">0</span>
    </label>
  </div>
  <div id="hexView"  class="hex-view" style="display:block"></div>
  <div id="mfmView"  style="display:none">
    <div class="mfm-wrap"><canvas id="mfmCanvas" width="768" height="4"></canvas></div>
    <div class="mfm-legend">
      <div class="leg-item"><div class="leg-swatch" style="background:#ff3030"></div> MFM violation (11)</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#ffcc00"></div> Weak bit</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#4fc3f7"></div> Sync region</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#f6d32d"></div> IDAM / DAM</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#ff9800"></div> CHRN / Data CRC</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#dddddd"></div> Data 1</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#555555"></div> Clock 1</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#1e1e1e"></div> 0</div>
    </div>
  </div>
</div>

<script>
const vscode   = acquireVsCodeApi();
const MSR_BITS = ${msrBits};
const ST1_BITS = ${st1Bits};
const ST2_BITS = ${st2Bits};

// ── Globals ───────────────────────────────────────────────────────────────────

let currentTab   = 'hex';
let rawData      = null;   // last loaded raw track
let bitOffset    = 0;
let drivePresent = [false, false];

// ── Helpers ───────────────────────────────────────────────────────────────────

function hex2(v) { return (v & 0xFF).toString(16).toUpperCase().padStart(2,'0'); }
function hex4(v) { return (v & 0xFFFF).toString(16).toUpperCase().padStart(4,'0'); }
function hex6(v) { return v.toString(16).toUpperCase().padStart(6,'0'); }
function sizeFromN(n) { return 128 << Math.min(n, 8); }

// ── Status register ───────────────────────────────────────────────────────────

function renderMSR(msr) {
    let html = \`<span class="sr-hex mono">0x\${hex2(msr)}</span>\`;
    for (const b of [...MSR_BITS].reverse())
        html += \`<span class="sr-bit \${((msr >> b.bit) & 1) ? 'set' : 'clear'}" title="\${b.desc}">\${b.label}</span>\`;
    document.getElementById('msrRow').innerHTML = html;
}

// ── Track map ─────────────────────────────────────────────────────────────────

function buildTrackMap(sectors) {
    const map = document.createElement('div');
    map.className = 'track-map';
    if (!sectors || !sectors.length) { map.style.background='var(--bg-input)'; return map; }
    const n = sectors.length;
    sectors.forEach((s, i) => {
        const left  = (i / n * 100).toFixed(2) + '%';
        const width = (1 / n * 100).toFixed(2) + '%';
        const cls   = s.deleted ? 'seg-deleted' : (!s.hdrCrc || !s.dataCrc ? 'seg-err' : 'seg-ok');
        const seg = document.createElement('div');
        seg.className = \`track-map-seg \${cls}\`;
        seg.style.left = left; seg.style.width = width;
        seg.title = \`Sct R=0x\${hex2(s.r)}: C\${s.c} H\${s.h} N\${s.n}\${(!s.hdrCrc||!s.dataCrc)?' CRC ERR':''}\${s.deleted?' DEL':''}\`;
        const lbl = document.createElement('div');
        lbl.className='seg-label'; lbl.style.left=left; lbl.style.width=width;
        lbl.textContent = '0x'+hex2(s.r);
        map.appendChild(seg); map.appendChild(lbl);
    });
    return map;
}

// ── Sectors table ─────────────────────────────────────────────────────────────

function buildSectorsTable(sectors) {
    if (!sectors || !sectors.length) {
        const d=document.createElement('div');
        d.className='dim'; d.style.fontSize='.85em'; d.textContent='No sectors.'; return d;
    }
    const table = document.createElement('table');
    table.className = 'sectors-table';
    table.innerHTML = '<thead><tr><th>#</th><th>C</th><th>H</th><th>R</th><th>N</th><th>Size</th><th>Hdr</th><th>Data</th><th>Flags</th></tr></thead>';
    const tbody = document.createElement('tbody');
    sectors.forEach((s,i) => {
        const hasErr = !s.hdrCrc || !s.dataCrc;
        const tr = document.createElement('tr');
        if (s.deleted) tr.className='deleted'; else if (hasErr) tr.className='crc-err';
        let flags = '';
        for (const b of ST1_BITS) if ((s.st1 >> b.bit)&1) flags += \`<span class="st-flag" title="\${b.desc}">\${b.label}</span>\`;
        for (const b of ST2_BITS) if ((s.st2 >> b.bit)&1) flags += \`<span class="st-flag" title="\${b.desc}">\${b.label}</span>\`;
        if (s.deleted) flags += '<span class="st-flag">DEL</span>';
        const nom = sizeFromN(s.n);
        const sizeTxt = nom + (s.size !== nom ? \`<br><span style="color:#f48771">real:\${s.size}</span>\` : '');
        tr.innerHTML =
            \`<td>\${i+1}</td><td>\${s.c}</td><td>\${s.h}</td><td>0x\${hex2(s.r)}</td><td>\${s.n}</td>\` +
            \`<td>\${sizeTxt}</td>\` +
            \`<td class="\${s.hdrCrc?'crc-ok':'crc-err'}">\${s.hdrCrc?'✓':'✗'}</td>\` +
            \`<td class="\${s.dataCrc?'crc-ok':'crc-err'}">\${s.dataCrc?'✓':'✗'}</td>\` +
            \`<td>\${flags}</td>\`;
        tbody.appendChild(tr);
    });
    table.appendChild(tbody);
    return table;
}

// ── Drive cards ───────────────────────────────────────────────────────────────

function buildDriveCard(drv, idx, currentDrive, motorOn) {
    const letter = String.fromCharCode(65+idx);
    const card   = document.createElement('div');
    card.className = 'drive-card' + (idx===currentDrive?' active':'') + (!drv.present?' no-disk':'');

    const title = document.createElement('div');
    title.className='drive-title';
    title.innerHTML =
        \`<span style="font-family:var(--font);font-weight:bold">Drive \${letter}</span>\` +
        \`<span class="\${motorOn?'motor-on':'motor-off'}">\${motorOn?'⚙ motor ON':'■ motor off'}</span>\` +
        (drv.writeProtected ? '<span class="wp-badge">WP</span>' : '');
    card.appendChild(title);

    if (!drv.present) {
        const e=document.createElement('div'); e.className='dim'; e.style.fontSize='.85em';
        e.textContent='No disk'; card.appendChild(e); return card;
    }

    const info = document.createElement('div');
    info.innerHTML =
        \`<div class="drive-row"><span class="drive-lbl">Track</span><span class="mono">\${drv.track} / \${drv.nbTracks-1}</span></div>\` +
        \`<div class="drive-row"><span class="drive-lbl">Side</span><span class="mono">\${drv.side}</span></div>\` +
        \`<div class="drive-row"><span class="drive-lbl">Sector</span><span class="mono">0x\${hex2(drv.sector)}</span></div>\` +
        \`<div class="drive-row"><span class="drive-lbl">Sides</span><span class="mono">\${drv.nbSides}</span></div>\` +
        (drv.gap3>0 ? \`<div class="drive-row"><span class="drive-lbl">GAP3</span><span class="mono">\${drv.gap3}</span></div>\` : '');
    card.appendChild(info);

    if (drv.path) {
        const p=document.createElement('div'); p.className='drive-path';
        const parts=drv.path.replace(/\\\\/g,'/').split('/');
        p.textContent=parts[parts.length-1]; p.title=drv.path;
        card.appendChild(p);
    }

    const lbl=document.createElement('div');
    lbl.className='dim'; lbl.style.cssText='font-size:.75em;margin-top:6px;margin-bottom:2px;';
    lbl.textContent=\`Track \${drv.track} — \${drv.sectors.length} sector(s)\${drv.trackSize?' ('+drv.trackSize+' bits)':''}\`;
    card.appendChild(lbl);
    card.appendChild(buildTrackMap(drv.sectors));
    card.appendChild(buildSectorsTable(drv.sectors));
    return card;
}

// ── MFM raw helpers ───────────────────────────────────────────────────────────

// Unpack packed hex string → Uint8Array of bits (0/1 per element)
function unpackBits(hexStr, bitSize) {
    const bits = new Uint8Array(bitSize);
    for (let i = 0; i < hexStr.length; i += 2) {
        const b = parseInt(hexStr.substr(i, 2), 16);
        for (let bit = 0; bit < 8; bit++) {
            const pos = i * 4 + bit;
            if (pos < bitSize) bits[pos] = (b >> (7 - bit)) & 1;
        }
    }
    return bits;
}

// Decode one byte from bit stream (GetNextByte algorithm: skip clock, read data bits)
function decodeByte(bits, startOff, bitSize) {
    let result = 0;
    let off = (startOff + 1) % bitSize; // skip first clock bit
    for (let i = 0; i < 8; i++) {
        result = (result << 1) | bits[off % bitSize];
        off = (off + 2) % bitSize;
    }
    return result;
}

// Build region annotation array (one entry per decoded byte)
// Regions: 0=none 1=gap1(4E) 2=gap0(00) 3=sync 4=idam 5=chrn 6=hcrc 7=dam 8=data 9=dcrc
function buildRegionMap(bitSize, bitOff, sectors) {
    const nbBytes = Math.floor(bitSize / 16);
    const map = new Uint8Array(nbBytes);

    function setRange(startBit, lenBits, code) {
        // startBit is absolute bit position in bitfield
        let adjStart = ((startBit - bitOff) % bitSize + bitSize) % bitSize;
        const byteStart = Math.floor(adjStart / 16);
        const byteEnd   = Math.min(byteStart + Math.ceil(lenBits / 16), nbBytes - 1);
        for (let b = byteStart; b <= byteEnd; b++) map[b] = code;
    }

    for (const s of sectors) {
        // IDAM: A1 A1 A1 (48 bits) + FE (16 bits) + CHRN (64 bits) + hdr CRC (32 bits)
        setRange(s.idamOffset,       48, 3); // sync A1 A1 A1
        setRange(s.idamOffset + 48,  16, 4); // FE  mark
        setRange(s.idamOffset + 64,  64, 5); // CHRN
        setRange(s.idamOffset + 128, 32, 6); // header CRC
        // DAM: A1 A1 A1 (48 bits) + FB/F8 (16 bits) + data + data CRC
        setRange(s.damOffset,        48, 3); // sync A1 A1 A1
        setRange(s.damOffset + 48,   16, 7); // DAM mark (FB or F8)
        setRange(s.damOffset + 64,   s.realSize * 16, 8); // data
        setRange(s.damOffset + 64 + s.realSize * 16, 32, 9); // data CRC
    }
    return map;
}

// ── Hex view renderer ─────────────────────────────────────────────────────────

const REGION_CLASS = ['','h-gap1','h-gap0','h-sync','h-idam','h-chrn','h-hcrc','h-dam','h-data','h-dcrc'];
const REGION_TOOLTIP = ['','GAP 4E','GAP 00','Sync A1/C2','IDAM (FE)','CHRN','Hdr CRC','DAM (FB/F8)','Data','Data CRC'];

function byteClass(val) {
    if (val === 0x4E) return 'h-gap1';
    if (val === 0x00) return 'h-gap0';
    if (val === 0xA1 || val === 0xC2) return 'h-sync';
    if (val === 0xFE) return 'h-idam';
    if (val === 0xFB || val === 0xF8) return 'h-dam';
    return '';
}

function renderHexView(data, bitOff) {
    const bits    = unpackBits(data.bits, data.bitSize);
    const weakBits = data.weakBits ? unpackBits(data.weakBits, data.bitSize) : null;
    const nbBytes = Math.floor(data.bitSize / 16);
    const regMap  = buildRegionMap(data.bitSize, bitOff, data.sectors || []);
    const COLS    = 16;

    // Also build weak byte map: if any of the 16 bits for byte i contains a weak bit
    let weakByteMap = null;
    if (weakBits) {
        weakByteMap = new Uint8Array(nbBytes);
        for (let i = 0; i < nbBytes; i++) {
            let base = (bitOff + i * 16) % data.bitSize;
            for (let k = 0; k < 16; k++) {
                if (weakBits[(base + k) % data.bitSize]) { weakByteMap[i] = 1; break; }
            }
        }
    }

    // Build CRC error byte map: bytes inside a sector whose CRC is wrong
    const crcErrMap = new Uint8Array(nbBytes);
    for (const s of (data.sectors || [])) {
        if (!s.hdrCrc) {
            let adj = ((s.idamOffset - bitOff) % data.bitSize + data.bitSize) % data.bitSize;
            const b = Math.floor(adj / 16);
            for (let k = b; k < Math.min(b + 10, nbBytes); k++) crcErrMap[k] = 1;
        }
        if (!s.dataCrc) {
            let adj = ((s.damOffset + 64 - bitOff) % data.bitSize + data.bitSize) % data.bitSize;
            const b = Math.floor(adj / 16);
            for (let k = b; k < Math.min(b + s.realSize + 2, nbBytes); k++) crcErrMap[k] = 1;
        }
    }

    const container = document.getElementById('hexView');
    // Build HTML in chunks for performance
    const chunks = [];
    for (let row = 0; row < Math.ceil(nbBytes / COLS); row++) {
        const bitPos = (bitOff + row * COLS * 16) % data.bitSize;
        chunks.push(\`<div class="hex-row"><span class="hex-offset">\${hex6(bitPos)}:</span><span class="hex-bytes">\`);
        for (let col = 0; col < COLS; col++) {
            const i = row * COLS + col;
            if (i >= nbBytes) break;
            const val  = decodeByte(bits, (bitOff + i * 16) % data.bitSize, data.bitSize);
            const rCls = regMap[i] ? REGION_CLASS[regMap[i]] : byteClass(val);
            const wCls = (weakByteMap && weakByteMap[i]) ? ' h-weak' : '';
            const eCls = (crcErrMap[i] && regMap[i] >= 8) ? ' h-crcerr' : '';
            const tip  = regMap[i] ? REGION_TOOLTIP[regMap[i]] : '';
            const sp   = (col === 7) ? ' &nbsp;' : '';
            chunks.push(\`<span class="hb \${rCls}\${wCls}\${eCls}" title="\${tip}">\${hex2(val)}</span>\${sp} \`);
        }
        chunks.push('</span></div>');
    }
    container.innerHTML = chunks.join('');
}

// ── MFM canvas renderer ───────────────────────────────────────────────────────

// Colors (as CSS strings for typed array → we use pre-computed RGBA)
const C_ZERO     = [0x1e, 0x1e, 0x1e, 0xff];  // 0 bit
const C_CLK1     = [0x55, 0x55, 0x55, 0xff];  // clock 1
const C_DATA1    = [0xdd, 0xdd, 0xdd, 0xff];  // data 1
const C_VIOL     = [0xff, 0x30, 0x30, 0xff];  // MFM violation
const C_WEAK     = [0xff, 0xcc, 0x00, 0xff];  // weak bit
const C_SYNC_BG  = [0x1a, 0x2e, 0x3a, 0xff];  // sync region bg
const C_IDAM_BG  = [0x2e, 0x28, 0x0e, 0xff];  // IDAM/DAM region bg
const C_CHRN_BG  = [0x2e, 0x20, 0x08, 0xff];  // CHRN/CRC region bg
const C_DATA_BG  = [0x14, 0x14, 0x14, 0xff];  // data region bg

const REGION_BG = [
    [0x14,0x14,0x14,0xff], // 0 = none
    [0x10,0x18,0x10,0xff], // 1 = gap1
    [0x0e,0x0e,0x0e,0xff], // 2 = gap0
    [0x0a,0x20,0x30,0xff], // 3 = sync (blue tint)
    [0x30,0x28,0x06,0xff], // 4 = IDAM (amber tint)
    [0x28,0x18,0x04,0xff], // 5 = CHRN (orange tint)
    [0x20,0x10,0x28,0xff], // 6 = hdr CRC (purple tint)
    [0x30,0x20,0x06,0xff], // 7 = DAM (amber)
    [0x14,0x14,0x14,0xff], // 8 = data
    [0x20,0x10,0x28,0xff], // 9 = data CRC (purple)
];

function renderMfmCanvas(data) {
    const bits    = unpackBits(data.bits, data.bitSize);
    const weakBits = data.weakBits ? unpackBits(data.weakBits, data.bitSize) : null;
    const regMap  = buildRegionMap(data.bitSize, 0, data.sectors || []);

    const BITS_PER_ROW = 512;  // 32 decoded bytes per row
    const BIT_W = 1.5;         // px per bit (canvas px = physical)
    const ROW_H = 5;
    const nbRows = Math.ceil(data.bitSize / BITS_PER_ROW);
    const canvasW = Math.floor(BITS_PER_ROW * BIT_W);
    const canvasH = nbRows * ROW_H;

    const canvas = document.getElementById('mfmCanvas');
    canvas.width  = canvasW;
    canvas.height = canvasH;
    const ctx = canvas.getContext('2d');

    const imgData = ctx.createImageData(canvasW, canvasH);
    const d = imgData.data;

    for (let row = 0; row < nbRows; row++) {
        for (let col = 0; col < BITS_PER_ROW; col++) {
            const bitIdx = row * BITS_PER_ROW + col;
            if (bitIdx >= data.bitSize) break;

            const byteIdx = Math.floor(bitIdx / 16);
            const bg = REGION_BG[regMap[byteIdx] || 0];

            const val  = bits[bitIdx];
            const prev = bitIdx > 0 ? bits[bitIdx - 1] : 0;
            const isWeak = weakBits ? weakBits[bitIdx] : false;
            const isViol = (val === 1 && prev === 1);
            const isClockPos = (bitIdx % 2 === 0);

            let color;
            if (isWeak)       color = C_WEAK;
            else if (isViol)  color = C_VIOL;
            else if (val === 0) color = bg;   // 0 = use background tint
            else if (isClockPos) color = C_CLK1;
            else color = C_DATA1;

            // Draw BIT_W × ROW_H rectangle
            const x0 = Math.floor(col * BIT_W);
            const x1 = Math.floor((col + 1) * BIT_W);
            for (let y = row * ROW_H; y < (row + 1) * ROW_H; y++) {
                for (let x = x0; x < x1; x++) {
                    const px = (y * canvasW + x) * 4;
                    d[px]   = color[0];
                    d[px+1] = color[1];
                    d[px+2] = color[2];
                    d[px+3] = color[3];
                }
            }
        }
    }
    ctx.putImageData(imgData, 0, 0);
}

// ── Tab switching ─────────────────────────────────────────────────────────────

function switchTab(tab) {
    currentTab = tab;
    document.getElementById('tabHex').classList.toggle('active', tab === 'hex');
    document.getElementById('tabMfm').classList.toggle('active', tab === 'mfm');
    document.getElementById('hexView').style.display     = tab === 'hex' ? 'block' : 'none';
    document.getElementById('hexControls').style.display = tab === 'hex' ? 'flex'  : 'none';
    document.getElementById('mfmView').style.display     = tab === 'mfm' ? 'block' : 'none';
    if (rawData) {
        if (tab === 'hex') renderHexView(rawData, bitOffset);
        if (tab === 'mfm') renderMfmCanvas(rawData);
    }
}

// ── Apply states ──────────────────────────────────────────────────────────────

function applyFdcState(state) {
    document.getElementById('errorMsg').style.display = 'none';
    renderMSR(state.mainStatus ?? 0);
    const grid = document.getElementById('drivesGrid');
    grid.innerHTML = '';
    (state.drives ?? []).forEach((drv, i) =>
        grid.appendChild(buildDriveCard(drv, i, state.currentDrive ?? 0, state.motorOn ?? false))
    );
    // Sync raw viewer drive selector defaults to current drive
    drivePresent = (state.drives ?? []).map(drv => !!drv.present);
    const curDrive = state.currentDrive ?? 0;
    document.getElementById('rawDrive').value = String(curDrive);
    const d = (state.drives ?? [])[curDrive];
    if (d) {
        document.getElementById('rawTrack').value = String(d.track);
        document.getElementById('rawSide').value  = String(d.side);
    }
    if (!drivePresent[curDrive]) {
        document.getElementById('rawStatus').textContent =
            \`No disk in drive \${String.fromCharCode(65 + curDrive)}\`;
    }
}

function applyRawTrack(data) {
    rawData = data;
    const bits = data.bitSize;
    const nb   = Math.floor(bits / 16);
    const revs = data.nbRevs || 1;
    const weak = data.weakBits ? ' — weak bits detected' : '';
    document.getElementById('rawStatus').textContent =
        \`\${bits} bits ≈ \${nb} bytes  ·  \${(data.sectors||[]).length} sectors  ·  \${revs} rev\${revs>1?'s':''}\${weak}\`;
    if (currentTab === 'hex') renderHexView(data, bitOffset);
    if (currentTab === 'mfm') renderMfmCanvas(data);
}

// ── Event listeners ───────────────────────────────────────────────────────────

document.getElementById('btnRefresh').addEventListener('click', () => vscode.postMessage({ type: 'refresh' }));

document.getElementById('rawDrive').addEventListener('change', function() {
    const drive = parseInt(this.value, 10);
    if (!drivePresent[drive])
        document.getElementById('rawStatus').textContent =
            \`No disk in drive \${String.fromCharCode(65 + drive)}\`;
    else if (document.getElementById('rawStatus').textContent.startsWith('No disk'))
        document.getElementById('rawStatus').textContent = '';
});

document.getElementById('btnLoadRaw').addEventListener('click', () => {
    const drive = parseInt(document.getElementById('rawDrive').value, 10);
    const track = parseInt(document.getElementById('rawTrack').value, 10);
    const side  = parseInt(document.getElementById('rawSide').value,  10);
    if (!drivePresent[drive]) {
        document.getElementById('rawStatus').textContent =
            \`No disk in drive \${String.fromCharCode(65 + drive)}\`;
        return;
    }
    document.getElementById('rawStatus').textContent = 'Loading…';
    vscode.postMessage({ type: 'loadRaw', drive, side, track });
});

document.getElementById('bitOffset').addEventListener('input', function() {
    bitOffset = parseInt(this.value, 10);
    document.getElementById('bitOffsetVal').textContent = String(bitOffset);
    if (rawData && currentTab === 'hex') renderHexView(rawData, bitOffset);
});

document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => switchTab(btn.dataset.tab));
});

window.addEventListener('message', e => {
    const msg = e.data;
    switch (msg.type) {
        case 'fdcState':  applyFdcState(msg.state); break;
        case 'rawLoading': document.getElementById('rawStatus').textContent = 'Loading…'; break;
        case 'rawTrack':  applyRawTrack(msg.data); break;
        case 'rawError':
            document.getElementById('rawStatus').textContent = 'Error: ' + msg.message;
            break;
        case 'error':
            document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
            document.getElementById('errorMsg').style.display = 'block';
            break;
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.FdcPanel = FdcPanel;
//# sourceMappingURL=FdcPanel.js.map

/***/ }),
/* 37 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.TapePanel = void 0;
const vscode = __importStar(__webpack_require__(1));
const HardwarePanel_1 = __webpack_require__(31);
// TZX block type names
const BLOCK_NAMES = {
    0x10: "Standard",
    0x11: "Turbo",
    0x12: "Pure Tone",
    0x13: "Pulse Seq",
    0x14: "Pure Data",
    0x15: "Direct Rec",
    0x18: "CSW",
    0x20: "Pause",
    0x2B: "Set Level",
};
// Block type colors for waveform (HIGH signal)
const BLOCK_COLOR = {
    0x10: "#4fc3f7", // standard: light blue
    0x11: "#26c6da", // turbo: cyan
    0x12: "#66bb6a", // pure tone: green
    0x13: "#a5d6a7", // pulse seq: light green
    0x14: "#ffb74d", // pure data: orange
    0x15: "#ff8a65", // direct rec: deep orange
    0x18: "#f06292", // CSW: pink
    0x20: "#444444", // pause: dark
    0x2B: "#ba68c8", // set level: purple
};
const DEFAULT_COLOR = "#888888";
class TapePanel extends HardwarePanel_1.HardwarePanel {
    static createOrShow() {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside : vscode.ViewColumn.One;
        if (TapePanel.currentPanel) {
            TapePanel.currentPanel._panel.reveal(column);
            TapePanel.currentPanel.refresh().catch(() => { });
            return;
        }
        const panel = vscode.window.createWebviewPanel("z80tapePanel", "Cassette", column, { enableScripts: true, retainContextWhenHidden: true });
        TapePanel.currentPanel = new TapePanel(panel);
    }
    constructor(panel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
            else if (msg.type === "loadSignal") {
                await this._loadSignal();
            }
        });
    }
    onDispose() { TapePanel.currentPanel = undefined; }
    async refresh() {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getTapeState", {});
            if (result?.error)
                this._panel.webview.postMessage({ type: "error", message: result.error });
            else
                this._panel.webview.postMessage({ type: "tapeState", state: result });
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }
    async _loadSignal() {
        const session = vscode.debug.activeDebugSession;
        if (!session)
            return;
        try {
            this._panel.webview.postMessage({ type: "signalLoading" });
            const result = await session.customRequest("getTapeSignal", {});
            if (result?.error)
                this._panel.webview.postMessage({ type: "signalError", message: result.error });
            else
                this._panel.webview.postMessage({ type: "tapeSignal", data: result });
        }
        catch (e) {
            this._panel.webview.postMessage({ type: "signalError", message: String(e) });
        }
    }
    _buildHtml() {
        const blockNames = JSON.stringify(BLOCK_NAMES);
        const blockColors = JSON.stringify(BLOCK_COLOR);
        const defColor = JSON.stringify(DEFAULT_COLOR);
        return /* html */ `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel_1.HardwarePanel.commonCss()}

  /* Status bar */
  .tape-status {
    display: flex;
    flex-wrap: wrap;
    gap: 10px 20px;
    margin-top: 4px;
    font-size: .9em;
  }
  .ts-item { display: flex; align-items: center; gap: 5px; }
  .ts-label { color: var(--fg-dim); }
  .indicator {
    display: inline-block;
    width: 10px; height: 10px;
    border-radius: 50%;
    border: 1px solid var(--border);
  }
  .ind-on  { background: #73c991; border-color: #73c991; }
  .ind-off { background: var(--bg-input); }
  .ind-rec { background: #f48771; border-color: #f48771; }

  /* Progress bar */
  .tape-progress-wrap {
    margin: 8px 0 4px;
    background: var(--bg-input);
    border-radius: 3px;
    height: 8px;
    border: 1px solid var(--border);
    overflow: hidden;
  }
  .tape-progress-bar {
    height: 100%;
    background: linear-gradient(90deg, #4fc3f7, #26c6da);
    transition: width .2s;
  }
  .tape-time { font-size: .8em; color: var(--fg-dim); text-align: right; }

  /* Block list */
  .blocks-table { width: 100%; border-collapse: collapse; font-size: .82em; margin-top: 4px; }
  .blocks-table th { color: var(--fg-dim); font-weight: normal; text-align: left;
                     padding: 1px 6px 3px 0; border-bottom: 1px solid var(--border); }
  .blocks-table td { padding: 2px 6px 2px 0; font-family: var(--font); }
  .blocks-table tr.current-block td { background: rgba(79,195,247,.12); color: #4fc3f7; }
  .block-type-dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; margin-right: 4px; }

  /* Waveform */
  .wave-controls {
    display: flex; flex-wrap: wrap; align-items: center; gap: 8px;
    margin: 6px 0 4px; font-size: .8em;
  }
  .wave-controls label { color: var(--fg-dim); }
  .wave-controls input[type=range] { width: 130px; }
  .wave-status { color: var(--fg-dim); font-size: .8em; }
  .wave-wrap {
    overflow-x: auto;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 2px;
    padding: 2px 0;
  }
  #waveCanvas { display: block; cursor: crosshair; }
  .wave-legend {
    display: flex; flex-wrap: wrap; gap: 8px; margin-top: 4px; font-size: .75em;
  }
  .leg-item { display: flex; align-items: center; gap: 3px; }
  .leg-dot  { width: 10px; height: 10px; border-radius: 2px; }

  /* Axis ruler */
  #waveRuler { display: block; font-size: .7em; font-family: var(--font); }
</style>
</head>
<body>

<div class="toolbar">
  <span id="badge" class="badge">Cassette</span>
  <button id="btnRefresh">&#x21BA; Refresh</button>
</div>
<div id="errorMsg" class="error"></div>

<div class="section-title">Status</div>
<div class="tape-status" id="tapeStatus"></div>
<div class="tape-progress-wrap"><div class="tape-progress-bar" id="progressBar" style="width:0%"></div></div>
<div class="tape-time" id="tapeTime"></div>

<div class="section-title">Blocks</div>
<table class="blocks-table">
  <thead><tr><th>#</th><th>Pos (s)</th><th>Type</th><th>Description</th></tr></thead>
  <tbody id="blocksBody"></tbody>
</table>

<div class="section-title">Signal Viewer</div>
<div class="wave-controls">
  <button id="btnLoadSignal">&#x2193; Load Signal</button>
  <label>Zoom
    <input id="zoomSlider" type="range" min="0" max="100" value="50">
  </label>
  <span id="zoomLabel" style="font-family:var(--font);color:var(--fg-dim);min-width:80px;"></span>
  <button id="btnCenter">&#x25CE; Re-center</button>
  <span id="waveStatus" class="wave-status"></span>
</div>
<canvas id="waveRuler"  width="900" height="18"></canvas>
<div class="wave-wrap"><canvas id="waveCanvas" width="900" height="100"></canvas></div>
<div class="wave-legend" id="waveLegend"></div>

<script>
const vscode       = acquireVsCodeApi();
const BLOCK_NAMES  = ${blockNames};
const BLOCK_COLORS = ${blockColors};
const DEF_COLOR    = ${defColor};
const TAPE_FREQ    = 4_000_000;  // T-states per second

// ── State ─────────────────────────────────────────────────────────────────────
let tapeState  = null;
let signalData = null;

// Zoom: T-states per pixel on canvas. Logarithmic slider 0-100 → 20 to 200000
function sliderToTspp(v) {
    return Math.round(Math.pow(10, 1.3 + v * 3.0 / 100));
}
let tspp = sliderToTspp(50);  // T-states per pixel
let viewCenterTs = 0;         // T-states of viewport center

// ── Helpers ───────────────────────────────────────────────────────────────────
function fmtTime(sec) {
    const m = Math.floor(sec / 60), s = sec % 60;
    return String(m).padStart(2,'0') + ':' + String(s).padStart(2,'0');
}
function fmtTs(ts) {
    if (ts < TAPE_FREQ) return (ts / 1000).toFixed(1) + ' kT';
    return (ts / TAPE_FREQ * 1000).toFixed(1) + ' ms';
}
function blockColor(bt) { return BLOCK_COLORS[bt] || DEF_COLOR; }
function blockName(bt)  { return BLOCK_NAMES[bt]  || ('0x' + (bt||0).toString(16).toUpperCase()); }

// ── Status ────────────────────────────────────────────────────────────────────
function renderStatus(state) {
    const fname = state.path ? state.path.replace(/\\\\/g,'/').split('/').pop() : '—';
    const pct = state.length > 0 ? (state.counter / state.length * 100).toFixed(1) : 0;

    document.getElementById('tapeStatus').innerHTML =
        \`<div class="ts-item"><span class="ts-label">File</span><span title="\${state.path}">\${fname || '(none)'}</span></div>\` +
        \`<div class="ts-item"><span class="ts-label">Motor</span><span class="indicator \${state.motor ? 'ind-on' : 'ind-off'}"></span><span>\${state.motor ? 'ON' : 'off'}</span></div>\` +
        \`<div class="ts-item"><span class="ts-label">Play</span><span class="indicator \${state.play ? 'ind-on' : 'ind-off'}"></span></div>\` +
        \`<div class="ts-item"><span class="ts-label">Record</span><span class="indicator \${state.record ? 'ind-rec' : 'ind-off'}"></span></div>\` +
        \`<div class="ts-item"><span class="ts-label">Pulses</span><span class="mono">\${(state.nbInversions||0).toLocaleString()}</span></div>\`;

    document.getElementById('progressBar').style.width = pct + '%';
    document.getElementById('tapeTime').textContent = fmtTime(state.counter) + ' / ' + fmtTime(state.length);
}

// ── Block list ────────────────────────────────────────────────────────────────
function renderBlocks(state) {
    const cur   = state.currentBlock ?? -1;
    const tbody = document.getElementById('blocksBody');
    tbody.innerHTML = '';
    for (const blk of (state.blocks || [])) {
        const tr = document.createElement('tr');
        if (blk.index === cur) tr.className = 'current-block';
        const col = blockColor(state.currentBlockType);
        const dot = blk.index === cur
            ? \`<span class="block-type-dot" style="background:\${col}"></span>\` : '';
        tr.innerHTML =
            \`<td>\${blk.index}</td>\` +
            \`<td class="mono">\${fmtTime(blk.position)}</td>\` +
            \`<td>\${dot}\${blockName(state.currentBlockType)}</td>\` +
            \`<td>\${blk.text || ''}</td>\`;
        tbody.appendChild(tr);
    }
    if (cur >= 0) {
        const rows = tbody.querySelectorAll('tr');
        if (rows[cur]) rows[cur].scrollIntoView({ block: 'nearest' });
    }
}

// ── Waveform ──────────────────────────────────────────────────────────────────

const CANVAS_W = 900;
const CANVAS_H = 100;
const MID_Y    = 50;    // center line (ground)
const HIGH_Y   = 5;     // top of HIGH rect
const LOW_Y    = 55;    // top of LOW rect
const SIG_H    = 40;    // height of signal rectangle

function renderWaveLegend() {
    const legend = document.getElementById('waveLegend');
    legend.innerHTML = '';
    const types = [0x10, 0x11, 0x12, 0x13, 0x14, 0x20, 0x2B];
    for (const bt of types) {
        const d = document.createElement('div');
        d.className = 'leg-item';
        d.innerHTML = \`<div class="leg-dot" style="background:\${blockColor(bt)}"></div>\${blockName(bt)}\`;
        legend.appendChild(d);
    }
}

function renderRuler(viewStartTs, viewEndTs) {
    const canvas = document.getElementById('waveRuler');
    canvas.width = CANVAS_W;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, CANVAS_W, 18);
    ctx.fillStyle = '#555';
    ctx.font = '9px monospace';

    // Place ~6 ticks
    const span = viewEndTs - viewStartTs;
    const rawStep = span / 6;
    // Round step to nice value
    const mag = Math.pow(10, Math.floor(Math.log10(rawStep)));
    let step = mag;
    if (rawStep / mag > 5) step = mag * 5;
    else if (rawStep / mag > 2) step = mag * 2;

    const firstTick = Math.ceil(viewStartTs / step) * step;
    for (let ts = firstTick; ts <= viewEndTs; ts += step) {
        const x = Math.round((ts - viewStartTs) / tspp);
        if (x < 0 || x > CANVAS_W) continue;
        ctx.fillStyle = '#555';
        ctx.fillRect(x, 13, 1, 5);
        const label = (ts / TAPE_FREQ * 1000).toFixed(ts < 1e6 ? 2 : 1) + 'ms';
        ctx.fillStyle = '#888';
        ctx.fillText(label, x + 2, 11);
    }
}

function renderWaveform() {
    if (!signalData) return;

    const pulses = signalData.pulses;
    if (!pulses || pulses.length === 0) {
        document.getElementById('waveStatus').textContent = 'No signal data.';
        return;
    }

    const viewHalfW   = (CANVAS_W / 2) * tspp;
    const viewStartTs = viewCenterTs - viewHalfW;
    const viewEndTs   = viewCenterTs + viewHalfW;

    renderRuler(viewStartTs, viewEndTs);

    const canvas = document.getElementById('waveCanvas');
    canvas.width  = CANVAS_W;
    canvas.height = CANVAS_H;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#1a1a1a';
    ctx.fillRect(0, 0, CANVAS_W, CANVAS_H);

    // Ground line
    ctx.strokeStyle = '#333';
    ctx.beginPath();
    ctx.moveTo(0, MID_Y);
    ctx.lineTo(CANVAS_W, MID_Y);
    ctx.stroke();

    // Draw pulses
    let nDrawn = 0;
    for (const p of pulses) {
        const pStart = Number(p.at);
        const pEnd   = pStart + Number(p.len);

        if (pEnd < viewStartTs || pStart > viewEndTs) continue;

        const x0 = Math.max(0,       (pStart - viewStartTs) / tspp);
        const x1 = Math.min(CANVAS_W,(pEnd   - viewStartTs) / tspp);
        const w  = Math.max(1, x1 - x0);

        const col = blockColor(p.bt);
        ctx.fillStyle = p.hi ? col : shadeColor(col, -60);

        const y = p.hi ? HIGH_Y : LOW_Y;
        ctx.fillRect(x0, y, w, SIG_H);
        nDrawn++;
    }

    // Current position marker (vertical red line at center)
    const cx = CANVAS_W / 2;
    ctx.strokeStyle = '#e01b24';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(cx, 0);
    ctx.lineTo(cx, CANVAS_H);
    ctx.stroke();
    ctx.lineWidth = 1;

    document.getElementById('waveStatus').textContent =
        \`\${nDrawn} pulses visible  ·  center: \${(viewCenterTs / TAPE_FREQ * 1000).toFixed(2)} ms\`;
}

function shadeColor(hex, amount) {
    // Darken/lighten a hex color
    const r = Math.max(0, Math.min(255, parseInt(hex.slice(1,3),16) + amount));
    const g = Math.max(0, Math.min(255, parseInt(hex.slice(3,5),16) + amount));
    const b = Math.max(0, Math.min(255, parseInt(hex.slice(5,7),16) + amount));
    return '#' + [r,g,b].map(v => v.toString(16).padStart(2,'0')).join('');
}

function applySignal(data) {
    signalData = data;
    // Center on current pulse
    viewCenterTs = Number(data.currentAt || 0);
    renderWaveform();
    renderWaveLegend();
    const nbPulses = (data.pulses || []).length;
    document.getElementById('waveStatus').textContent =
        \`\${data.total?.toLocaleString() || '?'} total pulses · window: \${nbPulses} · current idx: \${data.tapePos}\`;
}

// ── Zoom & pan ────────────────────────────────────────────────────────────────

const zoomSlider = document.getElementById('zoomSlider');
const zoomLabel  = document.getElementById('zoomLabel');

function updateZoom() {
    tspp = sliderToTspp(parseInt(zoomSlider.value, 10));
    const msPerPx = (tspp / TAPE_FREQ * 1000);
    zoomLabel.textContent = msPerPx >= 1
        ? msPerPx.toFixed(1) + ' ms/px'
        : (tspp).toFixed(0) + ' T/px';
    renderWaveform();
}
zoomSlider.addEventListener('input', updateZoom);
updateZoom();

// Pan by dragging
let dragStart = null;
let dragStartCenter = null;
const waveCanvas = document.getElementById('waveCanvas');
waveCanvas.addEventListener('mousedown', e => {
    dragStart = e.clientX;
    dragStartCenter = viewCenterTs;
});
waveCanvas.addEventListener('mousemove', e => {
    if (dragStart === null) return;
    const dx = e.clientX - dragStart;
    viewCenterTs = dragStartCenter - dx * tspp;
    renderWaveform();
});
waveCanvas.addEventListener('mouseup',   () => { dragStart = null; });
waveCanvas.addEventListener('mouseleave',() => { dragStart = null; });

// Mouse wheel zoom
waveCanvas.addEventListener('wheel', e => {
    e.preventDefault();
    const delta = e.deltaY > 0 ? 5 : -5;
    zoomSlider.value = String(Math.max(0, Math.min(100, parseInt(zoomSlider.value,10) + delta)));
    updateZoom();
}, { passive: false });

// ── Buttons ───────────────────────────────────────────────────────────────────

document.getElementById('btnRefresh').addEventListener('click', () =>
    vscode.postMessage({ type: 'refresh' }));

document.getElementById('btnLoadSignal').addEventListener('click', () =>
    vscode.postMessage({ type: 'loadSignal' }));

document.getElementById('btnCenter').addEventListener('click', () => {
    if (signalData) {
        viewCenterTs = Number(signalData.currentAt || 0);
        renderWaveform();
    }
});

// ── Messages ──────────────────────────────────────────────────────────────────

window.addEventListener('message', e => {
    const msg = e.data;
    switch (msg.type) {
        case 'tapeState':
            tapeState = msg.state;
            renderStatus(msg.state);
            renderBlocks(msg.state);
            break;
        case 'tapeSignal':
            applySignal(msg.data);
            break;
        case 'signalLoading':
            document.getElementById('waveStatus').textContent = 'Loading…';
            break;
        case 'signalError':
            document.getElementById('waveStatus').textContent = 'Error: ' + msg.message;
            break;
        case 'error':
            document.getElementById('errorMsg').textContent = 'Error: ' + msg.message;
            document.getElementById('errorMsg').style.display = 'block';
            break;
    }
});

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
exports.TapePanel = TapePanel;
//# sourceMappingURL=TapePanel.js.map

/***/ }),
/* 38 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.HardwarePanelTreeProvider = exports.HardwarePanelItem = void 0;
const vscode = __importStar(__webpack_require__(1));
const PANELS = [
    { label: "CRTC / ASIC", command: "z80debug.showCrtcPanel", ready: true },
    { label: "Gate Array", command: "z80debug.showGateArrayPanel", ready: true },
    { label: "PSG (AY-3-8912)", command: "z80debug.showPsgPanel", ready: true },
    { label: "FDC (µPD765)", command: "z80debug.showFdcPanel", ready: true },
    { label: "PPI (8255)", command: "z80debug.showPpiPanel", ready: true },
    { label: "Cassette", command: "z80debug.showTapePanel", ready: true },
];
class HardwarePanelItem extends vscode.TreeItem {
    constructor(entry) {
        super(entry.label, vscode.TreeItemCollapsibleState.None);
        this.iconPath = new vscode.ThemeIcon(entry.ready ? "circuit-board" : "watch");
        this.tooltip = entry.ready ? `Open ${entry.label} panel` : "Not yet implemented";
        if (entry.ready) {
            this.command = {
                command: entry.command,
                title: `Open ${entry.label}`,
            };
        }
        this.contextValue = entry.ready ? "hardwarePanel" : "hardwarePanelPending";
    }
}
exports.HardwarePanelItem = HardwarePanelItem;
class HardwarePanelTreeProvider {
    constructor() {
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
    }
    getTreeItem(item) { return item; }
    getChildren() {
        return PANELS.map(p => new HardwarePanelItem(p));
    }
    refresh() { this._onDidChangeTreeData.fire(); }
}
exports.HardwarePanelTreeProvider = HardwarePanelTreeProvider;
//# sourceMappingURL=HardwarePanelTreeProvider.js.map

/***/ }),
/* 39 */
/***/ (function(__unused_webpack_module, exports, __webpack_require__) {

"use strict";

var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", ({ value: true }));
exports.initI18n = initI18n;
exports.t = t;
const vscode = __importStar(__webpack_require__(1));
const fs = __importStar(__webpack_require__(2));
const nodePath = __importStar(__webpack_require__(3));
let _messages = {};
/**
 * Call once in activate() — loads the JSON bundle that matches VS Code's
 * display language, falling back to English when no translation is found.
 * Adding a new language is as simple as dropping `i18n/<lang>.json` next
 * to the extension and reloading.
 */
function initI18n(extensionPath) {
    const locale = vscode.env.language.toLowerCase(); // e.g. "fr-fr", "en"
    const lang = locale.split("-")[0]; // "fr", "en"
    for (const candidate of [locale, lang, "en"]) {
        const p = nodePath.join(extensionPath, "i18n", `${candidate}.json`);
        if (fs.existsSync(p)) {
            try {
                _messages = JSON.parse(fs.readFileSync(p, "utf8"));
                return;
            }
            catch { /* try next candidate */ }
        }
    }
    // No bundle found — t() will return the key itself (always readable)
}
/**
 * Translate a key.  Falls back to the key string if not found.
 * Positional placeholders: {0}, {1}, …
 *
 * @example t("cmd.addBreakpoint.failed", err.message)
 */
function t(key, ...args) {
    let msg = _messages[key] ?? key;
    for (let i = 0; i < args.length; i++) {
        msg = msg.split(`{${i}}`).join(String(args[i]));
    }
    return msg;
}
//# sourceMappingURL=i18n.js.map

/***/ })
/******/ 	]);
/************************************************************************/
/******/ 	// The module cache
/******/ 	var __webpack_module_cache__ = {};
/******/ 	
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/ 		// Check if module is in cache
/******/ 		var cachedModule = __webpack_module_cache__[moduleId];
/******/ 		if (cachedModule !== undefined) {
/******/ 			return cachedModule.exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = __webpack_module_cache__[moduleId] = {
/******/ 			// no module.id needed
/******/ 			// no module.loaded needed
/******/ 			exports: {}
/******/ 		};
/******/ 	
/******/ 		// Execute the module function
/******/ 		__webpack_modules__[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/ 	
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/ 	
/************************************************************************/
/******/ 	
/******/ 	// startup
/******/ 	// Load entry module and return exports
/******/ 	// This entry module is referenced by other modules so it can't be inlined
/******/ 	var __webpack_exports__ = __webpack_require__(0);
/******/ 	module.exports = __webpack_exports__;
/******/ 	
/******/ })()
;
//# sourceMappingURL=main.js.map