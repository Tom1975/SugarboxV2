import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

export class GateArrayPanel extends HardwarePanel {
    static currentPanel: GateArrayPanel | undefined;

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;

        if (GateArrayPanel.currentPanel) {
            GateArrayPanel.currentPanel._panel.reveal(column);
            GateArrayPanel.currentPanel.refresh().catch(() => {});
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            "z80gaPanel",
            "Gate Array",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        GateArrayPanel.currentPanel = new GateArrayPanel(panel);
    }

    private constructor(panel: vscode.WebviewPanel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            }
        });
    }

    protected override onDispose(): void {
        GateArrayPanel.currentPanel = undefined;
    }

    async refresh(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getGateArrayState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            } else {
                this._panel.webview.postMessage({ type: "gaState", state: result });
            }
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private _buildHtml(): string {
        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}
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
