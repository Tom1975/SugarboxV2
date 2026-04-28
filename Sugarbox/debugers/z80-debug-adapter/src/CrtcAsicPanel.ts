import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

const CRTC_TYPE_NAMES = ["HD6845S/UM6845", "UM6845R", "MC6845", "AMS40489", "AMS40226"];

const CRTC_REG_NAMES: string[] = [
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

export class CrtcAsicPanel extends HardwarePanel {
    static currentPanel: CrtcAsicPanel | undefined;

    // ── Static factory ────────────────────────────────────────────────────────

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;

        if (CrtcAsicPanel.currentPanel) {
            CrtcAsicPanel.currentPanel._panel.reveal(column);
            CrtcAsicPanel.currentPanel.refresh().catch(() => {});
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            "z80crtcPanel",
            "CRTC",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        CrtcAsicPanel.currentPanel = new CrtcAsicPanel(panel);
    }

    // ── Constructor ───────────────────────────────────────────────────────────

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
        CrtcAsicPanel.currentPanel = undefined;
    }

    // ── Refresh ───────────────────────────────────────────────────────────────

    async refresh(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getCrtcState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
                return;
            }
            this._panel.webview.postMessage({ type: "crtcState", state: result });
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    // ── HTML ──────────────────────────────────────────────────────────────────

    private _buildHtml(): string {
        const regNames = JSON.stringify(CRTC_REG_NAMES);
        const typeNames = JSON.stringify(CRTC_TYPE_NAMES);

        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}
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

<script>
const vscode = acquireVsCodeApi();
const REG_NAMES  = ${regNames};
const TYPE_NAMES = ${typeNames};

let prevRegs = null;
let prevCounters = null;

function hex2(v)  { return (v & 0xFF).toString(16).toUpperCase().padStart(2,'0'); }
function hex4(v)  { return (v & 0xFFFF).toString(16).toUpperCase().padStart(4,'0'); }

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
    const regs  = state.registers;   // array[18]
    const masks = state.masks;       // array[18]
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

function applyState(state) {
    const crtcType = state.crtcType ?? 0;
    const isPlus   = state.isPlus  ?? false;
    const badge    = document.getElementById('badge');

    if (isPlus) {
        badge.textContent = 'ASIC (CPC+)';
        // TODO: switch to ASIC view when implemented
    } else {
        badge.textContent = 'CRTC ' + crtcType + ' — ' + (TYPE_NAMES[crtcType] ?? '?');
    }

    document.getElementById('errorMsg').style.display = 'none';
    renderRegisters(state);
    renderCounters(state);
}

document.getElementById('btnRefresh').addEventListener('click', () => {
    vscode.postMessage({ type: 'refresh' });
});

window.addEventListener('message', e => {
    const msg = e.data;
    switch (msg.type) {
        case 'crtcState':
            applyState(msg.state);
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
