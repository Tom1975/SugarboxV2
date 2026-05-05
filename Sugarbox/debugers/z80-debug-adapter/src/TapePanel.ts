import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

// TZX block type names
const BLOCK_NAMES: Record<number, string> = {
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
const BLOCK_COLOR: Record<number, string> = {
    0x10: "#4fc3f7",  // standard: light blue
    0x11: "#26c6da",  // turbo: cyan
    0x12: "#66bb6a",  // pure tone: green
    0x13: "#a5d6a7",  // pulse seq: light green
    0x14: "#ffb74d",  // pure data: orange
    0x15: "#ff8a65",  // direct rec: deep orange
    0x18: "#f06292",  // CSW: pink
    0x20: "#444444",  // pause: dark
    0x2B: "#ba68c8",  // set level: purple
};
const DEFAULT_COLOR = "#888888";

export class TapePanel extends HardwarePanel {
    static currentPanel: TapePanel | undefined;

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside : vscode.ViewColumn.One;
        if (TapePanel.currentPanel) {
            TapePanel.currentPanel._panel.reveal(column);
            TapePanel.currentPanel.refresh().catch(() => {});
            return;
        }
        const panel = vscode.window.createWebviewPanel(
            "z80tapePanel", "Cassette", column,
            { enableScripts: true, retainContextWhenHidden: true }
        );
        TapePanel.currentPanel = new TapePanel(panel);
    }

    private constructor(panel: vscode.WebviewPanel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            } else if (msg.type === "loadSignal") {
                await this._loadSignal();
            }
        });
    }

    protected override onDispose(): void { TapePanel.currentPanel = undefined; }

    async refresh(): Promise<void> {
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
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private async _loadSignal(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) return;
        try {
            this._panel.webview.postMessage({ type: "signalLoading" });
            const result = await session.customRequest("getTapeSignal", {});
            if (result?.error)
                this._panel.webview.postMessage({ type: "signalError", message: result.error });
            else
                this._panel.webview.postMessage({ type: "tapeSignal", data: result });
        } catch (e) {
            this._panel.webview.postMessage({ type: "signalError", message: String(e) });
        }
    }

    private _buildHtml(): string {
        const blockNames  = JSON.stringify(BLOCK_NAMES);
        const blockColors = JSON.stringify(BLOCK_COLOR);
        const defColor    = JSON.stringify(DEFAULT_COLOR);

        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}

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
