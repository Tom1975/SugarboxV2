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
            "CRTC / ASIC",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        CrtcAsicPanel.currentPanel = new CrtcAsicPanel(panel);
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
        CrtcAsicPanel.currentPanel = undefined;
    }

    async refresh(): Promise<void> {
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
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private _buildHtml(): string {
        const regNames  = JSON.stringify(CRTC_REG_NAMES);
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
