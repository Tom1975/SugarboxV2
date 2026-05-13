import * as vscode from "vscode";
import * as fs from "fs";

export class ConfigPanel {
    static readonly viewType = "z80debug.config";
    private static _current: ConfigPanel | undefined;

    private readonly _panel: vscode.WebviewPanel;
    private readonly _context: vscode.ExtensionContext;
    private _disposables: vscode.Disposable[] = [];

    static createOrShow(context: vscode.ExtensionContext): void {
        if (ConfigPanel._current) {
            ConfigPanel._current._panel.reveal();
            return;
        }
        const panel = vscode.window.createWebviewPanel(
            ConfigPanel.viewType,
            "Z80 Debug — Configuration",
            vscode.ViewColumn.Active,
            { enableScripts: true, retainContextWhenHidden: true }
        );
        ConfigPanel._current = new ConfigPanel(panel, context);
    }

    private constructor(panel: vscode.WebviewPanel, context: vscode.ExtensionContext) {
        this._panel = panel;
        this._context = context;

        this._panel.webview.html = this._getHtml();
        this._panel.onDidDispose(() => this._dispose(), null, this._disposables);

        this._panel.webview.onDidReceiveMessage(async (msg) => {
            switch (msg.type) {
                case "ready":
                    this._sendCurrentSettings();
                    break;
                case "browse": {
                    const isWin = process.platform === "win32";
                    const picked = await vscode.window.showOpenDialog({
                        title: msg.field === "sugarbox"
                            ? "Sélectionner l'exécutable Sugarbox"
                            : "Sélectionner l'exécutable RASM",
                        canSelectMany: false,
                        filters: isWin
                            ? { "Exécutables": ["exe"] }
                            : { "Tous les fichiers": ["*"] }
                    });
                    if (picked) {
                        this._panel.webview.postMessage({
                            type: "setPath",
                            field: msg.field,
                            value: picked[0].fsPath
                        });
                    }
                    break;
                }
                case "save": {
                    const cfg = vscode.workspace.getConfiguration("z80debug");
                    const target = vscode.workspace.workspaceFolders
                        ? vscode.ConfigurationTarget.Workspace
                        : vscode.ConfigurationTarget.Global;
                    await cfg.update("sugarbox",     msg.sugarbox,     target);
                    await cfg.update("rasm",         msg.rasm,         target);
                    await cfg.update("hideEmulator", msg.hideEmulator, target);
                    vscode.window.showInformationMessage("Configuration sauvegardée.");
                    this._panel.dispose();
                    break;
                }
                case "cancel":
                    this._panel.dispose();
                    break;
            }
        }, null, this._disposables);
    }

    private _sendCurrentSettings(): void {
        const cfg = vscode.workspace.getConfiguration("z80debug");
        this._panel.webview.postMessage({
            type: "init",
            sugarbox:     cfg.get<string>("sugarbox", ""),
            rasm:         cfg.get<string>("rasm", "rasm"),
            hideEmulator: cfg.get<boolean>("hideEmulator", false),
        });
    }

    private _dispose(): void {
        ConfigPanel._current = undefined;
        this._panel.dispose();
        this._disposables.forEach(d => d.dispose());
        this._disposables = [];
    }

    private _getHtml(): string {
        return `<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy"
      content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
  :root {
    --bg:       var(--vscode-editor-background);
    --fg:       var(--vscode-editor-foreground);
    --fg-dim:   var(--vscode-descriptionForeground);
    --border:   var(--vscode-panel-border, #444);
    --input-bg: var(--vscode-input-background);
    --input-fg: var(--vscode-input-foreground);
    --input-br: var(--vscode-input-border, #555);
    --btn-bg:   var(--vscode-button-background);
    --btn-fg:   var(--vscode-button-foreground);
    --btn-hov:  var(--vscode-button-hoverBackground);
    --font:     var(--vscode-editor-font-family, monospace);
  }
  * { box-sizing: border-box; }
  body {
    background: var(--bg);
    color: var(--fg);
    font-family: var(--vscode-font-family, sans-serif);
    font-size: var(--vscode-font-size, 13px);
    margin: 0;
    padding: 24px 32px;
    max-width: 640px;
  }
  h2 { font-size: 1.1em; font-weight: 600; margin: 0 0 20px; }
  .section { margin-bottom: 20px; }
  label { display: block; color: var(--fg-dim); font-size: .85em; margin-bottom: 5px; }
  .path-row {
    display: flex; gap: 6px; align-items: center;
  }
  .path-row input {
    flex: 1;
    background: var(--input-bg);
    color: var(--input-fg);
    border: 1px solid var(--input-br);
    padding: 5px 8px;
    font-family: var(--font);
    font-size: .9em;
    border-radius: 2px;
    outline: none;
  }
  .path-row input:focus { border-color: var(--vscode-focusBorder, #007acc); }
  .path-hint { font-size: .75em; color: var(--fg-dim); margin-top: 4px; }
  .toggle-row {
    display: flex; align-items: center; gap: 10px;
    padding: 10px 0;
    border-top: 1px solid var(--border);
    border-bottom: 1px solid var(--border);
    margin-bottom: 8px;
  }
  .toggle-row span { font-size: .9em; }
  .toggle-row small { color: var(--fg-dim); font-size: .8em; display: block; }
  input[type=checkbox] { width: 16px; height: 16px; accent-color: var(--btn-bg); cursor: pointer; }
  button {
    background: var(--btn-bg);
    color: var(--btn-fg);
    border: none;
    padding: 6px 14px;
    cursor: pointer;
    border-radius: 2px;
    font-size: .85em;
  }
  button:hover { background: var(--btn-hov); }
  button.secondary {
    background: transparent;
    color: var(--fg-dim);
    border: 1px solid var(--border);
  }
  button.secondary:hover { color: var(--fg); border-color: var(--fg-dim); }
  .actions { display: flex; gap: 8px; margin-top: 24px; }
  .warn { color: #f48771; font-size:.75em; margin-top:3px; display:none; }
</style>
</head>
<body>
<h2>&#9881; Z80 Debug — Configuration</h2>

<div class="section">
  <label for="sugarbox">Émulateur Sugarbox</label>
  <div class="path-row">
    <input type="text" id="sugarbox" placeholder="/chemin/vers/Sugarbox" spellcheck="false">
    <button onclick="browse('sugarbox')">Parcourir…</button>
  </div>
  <div class="path-hint">Exécutable de l'émulateur SugarboxV2</div>
  <div class="warn" id="warn-sugarbox">Fichier introuvable</div>
</div>

<div class="section">
  <label for="rasm">Assembleur RASM</label>
  <div class="path-row">
    <input type="text" id="rasm" placeholder="rasm  (ou chemin absolu)" spellcheck="false">
    <button onclick="browse('rasm')">Parcourir…</button>
  </div>
  <div class="path-hint">Laissez <code>rasm</code> si l'assembleur est dans le PATH</div>
</div>

<div class="toggle-row">
  <input type="checkbox" id="hideEmulator">
  <div>
    <span>Masquer l'émulateur au lancement</span>
    <small>L'émulateur tourne en arrière-plan sans fenêtre visible</small>
  </div>
</div>

<div class="actions">
  <button onclick="save()">&#10003; Sauvegarder</button>
  <button class="secondary" onclick="cancel()">Annuler</button>
</div>

<script>
const vscode = acquireVsCodeApi();

window.addEventListener('message', e => {
  const msg = e.data;
  if (msg.type === 'init') {
    document.getElementById('sugarbox').value     = msg.sugarbox     || '';
    document.getElementById('rasm').value         = msg.rasm         || '';
    document.getElementById('hideEmulator').checked = !!msg.hideEmulator;
  } else if (msg.type === 'setPath') {
    document.getElementById(msg.field).value = msg.value;
  }
});

function browse(field) {
  vscode.postMessage({ type: 'browse', field });
}
function save() {
  vscode.postMessage({
    type: 'save',
    sugarbox:     document.getElementById('sugarbox').value.trim(),
    rasm:         document.getElementById('rasm').value.trim() || 'rasm',
    hideEmulator: document.getElementById('hideEmulator').checked,
  });
}
function cancel() { vscode.postMessage({ type: 'cancel' }); }

vscode.postMessage({ type: 'ready' });
</script>
</body>
</html>`;
    }
}
