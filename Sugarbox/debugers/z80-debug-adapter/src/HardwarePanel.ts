import * as vscode from "vscode";

// Base class for all hardware debug panels.
// Each subclass implements refresh() to fetch state via customRequest and update its webview.
// HardwarePanel.refreshAll() is called on every StoppedEvent.
export abstract class HardwarePanel {
    private static readonly _registry: Set<HardwarePanel> = new Set();

    protected readonly _panel: vscode.WebviewPanel;
    private _disposed = false;

    protected constructor(panel: vscode.WebviewPanel) {
        this._panel = panel;
        HardwarePanel._registry.add(this);

        this._panel.onDidDispose(() => {
            HardwarePanel._registry.delete(this);
            this._disposed = true;
            this.onDispose();
        });
    }

    protected onDispose(): void {}

    get isDisposed(): boolean { return this._disposed; }

    abstract refresh(): Promise<void>;

    static async refreshAll(): Promise<void> {
        for (const panel of HardwarePanel._registry) {
            try { await panel.refresh(); } catch { /* ignore — session may be gone */ }
        }
    }

    // ── Shared CSS ────────────────────────────────────────────────────────────

    protected static commonCss(): string {
        return /* css */`
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
