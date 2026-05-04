import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

// Port B bit descriptions (CPC hardware)
const PORT_B_BITS = [
    { bit: 0, label: "VSYNC",        desc: (v: number) => v ? "in VSYNC" : "no VSYNC" },
    { bit: 1, label: "Type bit 1",   desc: (v: number) => v ? "1" : "0" },
    { bit: 2, label: "Type bit 2",   desc: (v: number) => v ? "1" : "0" },
    { bit: 3, label: "Type bit 3",   desc: (v: number) => v ? "1" : "0" },
    { bit: 4, label: "Screen (LK4)", desc: (v: number) => v ? "60 Hz" : "50 Hz" },
    { bit: 5, label: "/EXP",         desc: (v: number) => v ? "inactive" : "active" },
    { bit: 6, label: "Printer BUSY", desc: (v: number) => v ? "busy" : "ready" },
    { bit: 7, label: "CAS.IN",       desc: (v: number) => v ? "high" : "low" },
];

const PSG_MODES = ["Inactive", "Read register", "Write register", "Latch address"];

export class PpiPanel extends HardwarePanel {
    static currentPanel: PpiPanel | undefined;

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside
            : vscode.ViewColumn.One;

        if (PpiPanel.currentPanel) {
            PpiPanel.currentPanel._panel.reveal(column);
            PpiPanel.currentPanel.refresh().catch(() => {});
            return;
        }

        const panel = vscode.window.createWebviewPanel(
            "z80ppiPanel",
            "PPI (8255)",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        PpiPanel.currentPanel = new PpiPanel(panel);
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
        PpiPanel.currentPanel = undefined;
    }

    async refresh(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        try {
            const result = await session.customRequest("getPpiState", {});
            if (result?.error) {
                this._panel.webview.postMessage({ type: "error", message: result.error });
            } else {
                this._panel.webview.postMessage({ type: "ppiState", state: result });
            }
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private _buildHtml(): string {
        const portBBits  = JSON.stringify(PORT_B_BITS.map(b => ({ bit: b.bit, label: b.label })));
        const psgModes   = JSON.stringify(PSG_MODES);

        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}
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
