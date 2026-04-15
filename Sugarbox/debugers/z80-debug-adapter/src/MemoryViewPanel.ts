import * as vscode from "vscode";

const BYTES_PER_ROW = 16;
const ROW_COUNT     = 16;   // lines visible by default (one "page" = 256 bytes)

interface MemSource {
    type:    string;   // "read"|"write"|"ram"|"rom"|"cart"
    bank:    number;   // -1 = no bank (lower bank / mapped)
    label:   string;
    maxAddr: number;   // 0xFFFF or 0x3FFF depending on source
}

export class MemoryViewPanel {
    static currentPanel: MemoryViewPanel | undefined;

    private readonly _panel: vscode.WebviewPanel;
    private _address:  number    = 0;
    private _source:   MemSource = { type: "read", bank: -1, label: "Memory (Read)", maxAddr: 0xFFFF };

    // ── Static factory ────────────────────────────────────────────────────────

    static createOrShow(address?: number): void {
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

        const panel = vscode.window.createWebviewPanel(
            "z80memoryView",
            "Z80 Memory",
            column,
            { enableScripts: true, retainContextWhenHidden: true }
        );

        MemoryViewPanel.currentPanel = new MemoryViewPanel(panel, address ?? 0);
    }

    // ── Constructor ───────────────────────────────────────────────────────────

    private constructor(panel: vscode.WebviewPanel, address: number) {
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
                    this._address = (msg.address as number) & 0xFFFF;
                    if (msg.source) this._source = msg.source as MemSource;
                    await this._loadMemory();
                    break;
            }
        });
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private _sendAddress(): void {
        this._panel.webview.postMessage({ type: "setAddress", address: this._address });
    }

    private _defaultSources(): MemSource[] {
        return [
            { type: "read",  bank: -1, label: "Memory (Read)",  maxAddr: 0xFFFF },
            { type: "write", bank: -1, label: "Memory (Write)", maxAddr: 0xFFFF },
            { type: "ram",   bank: -1, label: "RAM lower bank", maxAddr: 0xFFFF },
        ];
    }

    private async _loadSources(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }
        let sources: MemSource[];
        try {
            const result = await session.customRequest("getMemBanks", {});
            // result.sources === null signals "emulator binary too old, no getMemBanks"
            if (result?.sources === null) {
                sources = this._defaultSources();
            } else {
                sources = Array.isArray(result?.sources) && result.sources.length > 0
                    ? result.sources as MemSource[]
                    : this._defaultSources();
            }
        } catch {
            sources = this._defaultSources();
        }
        this._panel.webview.postMessage({ type: "memSources", sources });
        await this._loadMemory();
    }

    private async _loadMemory(): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "error", message: "No active debug session" });
            return;
        }

        const count = BYTES_PER_ROW * ROW_COUNT;
        const src   = this._source;
        try {
            const result = await session.customRequest("readMemoryEx", {
                address: this._address,
                count,
                memType: src.type,
                bank:    src.bank
            });
            const bytes = result?.bytes as number[] ?? [];
            this._panel.webview.postMessage({ type: "memoryData", address: this._address, bytes });
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    // ── HTML ──────────────────────────────────────────────────────────────────

    private _buildHtml(): string {
        return /* html */`<!DOCTYPE html>
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
