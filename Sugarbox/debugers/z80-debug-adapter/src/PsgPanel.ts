import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

const AY_CLOCK = 1_000_000; // CPC AY clock = 1 MHz

// Envelope shape descriptions + ASCII waveform (4-bit shape value)
const ENV_SHAPES: { name: string; wave: string }[] = [
    { name: "Fall×1",       wave: "\\" },   // 0
    { name: "Fall×1",       wave: "\\" },   // 1
    { name: "Fall×1",       wave: "\\" },   // 2
    { name: "Fall×1",       wave: "\\" },   // 3
    { name: "Fall-Hold↓",   wave: "\\_" },  // 4
    { name: "Fall-Rise×∞",  wave: "\\/~" }, // 5
    { name: "Fall-Hold↑",   wave: "\\‾" },  // 6
    { name: "Fall×1",       wave: "\\" },   // 7
    { name: "Rise×∞",       wave: "///~" }, // 8
    { name: "Rise-Hold↑",   wave: "/‾" },   // 9
    { name: "Rise-Fall×∞",  wave: "/\\/~" },// 10
    { name: "Rise-Hold↓",   wave: "/_" },   // 11
    { name: "Rise×∞",       wave: "///~" }, // 12
    { name: "Rise-Hold↑",   wave: "/‾" },   // 13
    { name: "Rise-Fall×∞",  wave: "/\\/~" },// 14
    { name: "Rise-Hold↓",   wave: "/_" },   // 15
];

export class PsgPanel extends HardwarePanel {
    static currentPanel: PsgPanel | undefined;

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;

        if (PsgPanel.currentPanel) {
            PsgPanel.currentPanel._panel.reveal(column);
            PsgPanel.currentPanel.refresh().catch(() => {});
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            "z80psgPanel",
            "PSG (AY-3-8912)",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        PsgPanel.currentPanel = new PsgPanel(panel);
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
        PsgPanel.currentPanel = undefined;
    }

    async refresh(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getPsgState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            } else {
                this._panel.webview.postMessage({ type: "psgState", state: result });
            }
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private _buildHtml(): string {
        const envShapesJson = JSON.stringify(ENV_SHAPES);
        const ayClockJson   = JSON.stringify(AY_CLOCK);

        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}
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
