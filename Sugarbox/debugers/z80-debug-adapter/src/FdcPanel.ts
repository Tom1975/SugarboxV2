import * as vscode from "vscode";
import { HardwarePanel } from "./HardwarePanel";

const MSR_BITS = [
    { bit: 7, label: "RQM",  desc: "Request for Master" },
    { bit: 6, label: "DIO",  desc: "Data direction (1=to CPU)" },
    { bit: 5, label: "NDM",  desc: "Non-DMA mode" },
    { bit: 4, label: "CB",   desc: "FDC busy" },
    { bit: 3, label: "D3B",  desc: "Drive 3 busy" },
    { bit: 2, label: "D2B",  desc: "Drive 2 busy" },
    { bit: 1, label: "D1B",  desc: "Drive 1 busy" },
    { bit: 0, label: "D0B",  desc: "Drive 0 busy" },
];
const ST1_BITS = [
    { bit: 7, label: "EN",  desc: "End of cylinder" },
    { bit: 5, label: "DE",  desc: "Data error (CRC)" },
    { bit: 4, label: "OR",  desc: "Overrun" },
    { bit: 2, label: "ND",  desc: "No data" },
    { bit: 1, label: "NW",  desc: "Not writable" },
    { bit: 0, label: "MA",  desc: "Missing address mark" },
];
const ST2_BITS = [
    { bit: 6, label: "CM",  desc: "Control mark (deleted)" },
    { bit: 5, label: "DD",  desc: "Data CRC error" },
    { bit: 4, label: "WC",  desc: "Wrong cylinder" },
    { bit: 2, label: "BC",  desc: "Bad cylinder" },
    { bit: 1, label: "SNS", desc: "Scan not satisfied" },
    { bit: 0, label: "MAM", desc: "Missing AM in data" },
];

export class FdcPanel extends HardwarePanel {
    static currentPanel: FdcPanel | undefined;

    static createOrShow(): void {
        const column = vscode.window.activeTextEditor
            ? vscode.ViewColumn.Beside : vscode.ViewColumn.One;
        if (FdcPanel.currentPanel) {
            FdcPanel.currentPanel._panel.reveal(column);
            FdcPanel.currentPanel.refresh().catch(() => {});
            return;
        }
        const panel = vscode.window.createWebviewPanel(
            "z80fdcPanel", "FDC (µPD765)", column,
            { enableScripts: true, retainContextWhenHidden: true }
        );
        FdcPanel.currentPanel = new FdcPanel(panel);
    }

    private constructor(panel: vscode.WebviewPanel) {
        super(panel);
        this._panel.webview.html = this._buildHtml();
        this._panel.webview.onDidReceiveMessage(async (msg) => {
            if (msg.type === "ready" || msg.type === "refresh") {
                await this.refresh();
            } else if (msg.type === "loadRaw") {
                await this._loadRaw(msg.drive, msg.side, msg.track);
            } else if (msg.type === "insertDisk") {
                await this._insertDisk(msg.drive);
            }
        });
    }

    protected override onDispose(): void { FdcPanel.currentPanel = undefined; }

    async refresh(): Promise<void> {
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
        } catch (e) {
            this._panel.webview.postMessage({ type: "error", message: String(e) });
        }
    }

    private async _insertDisk(drive: number): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) {
            this._panel.webview.postMessage({ type: "insertResult", error: "No active debug session" });
            return;
        }
        const driveLetter = drive === 0 ? "A" : "B";
        const picked = await vscode.window.showOpenDialog({
            title: `Insert disk in drive ${driveLetter}`,
            canSelectMany: false,
            filters: { "Disk images": ["dsk", "DSK"] }
        });
        if (!picked) return;
        const path = picked[0].fsPath;
        try {
            const result = await session.customRequest("insertDisk", { drive, path });
            if (result?.error) {
                this._panel.webview.postMessage({ type: "insertResult", error: result.error });
            } else {
                this._panel.webview.postMessage({ type: "insertResult", drive, path });
                await this.refresh();
            }
        } catch (e) {
            this._panel.webview.postMessage({ type: "insertResult", error: String(e) });
        }
    }

    private async _loadRaw(drive: number, side: number, track: number): Promise<void> {
        const session = vscode.debug.activeDebugSession;
        if (!session) return;
        try {
            this._panel.webview.postMessage({ type: "rawLoading" });
            const result = await session.customRequest("getTrackRaw", { drive, side, track });
            if (result?.error)
                this._panel.webview.postMessage({ type: "rawError", message: result.error });
            else
                this._panel.webview.postMessage({ type: "rawTrack", data: result });
        } catch (e) {
            this._panel.webview.postMessage({ type: "rawError", message: String(e) });
        }
    }

    private _buildHtml(): string {
        const msrBits = JSON.stringify(MSR_BITS);
        const st1Bits = JSON.stringify(ST1_BITS);
        const st2Bits = JSON.stringify(ST2_BITS);

        return /* html */`<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline';">
<style>
${HardwarePanel.commonCss()}

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
  .h-pre0  { color:#6a7fb0; }
  .h-sync  { color:#4fc3f7; font-weight:bold; }
  .h-idam  { color:#f6d32d; font-weight:bold; }
  .h-chrn  { color:#ff9800; }
  .h-hcrc  { color:#ce93d8; }
  .h-dam    { color:#ffab40; font-weight:bold; }
  .h-dam-del{ color:#ff7043; font-weight:bold; }
  .h-data   { color:var(--fg); }
  .h-dcrc   { color:#ce93d8; }
  .h-crcerr { color:#e01b24; font-weight:bold; }
  .h-weak   { background:rgba(246,211,45,.18); text-decoration:underline dotted #f6d32d; }

  /* Hex legend */
  .hex-legend { display:flex; flex-wrap:wrap; gap:4px 12px; margin-top:6px; font-size:.75em; }
  .hex-leg-item { display:flex; align-items:center; gap:4px; white-space:nowrap; }
  .hex-leg-dot  { width:10px; height:10px; border-radius:2px; flex-shrink:0; }

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
<div style="display:flex;gap:8px;margin-top:6px;">
  <button id="btnInsertA">&#x1F4BE; Insert disk → A</button>
  <button id="btnInsertB">&#x1F4BE; Insert disk → B</button>
</div>
<div id="insertStatus" style="font-size:.8em;color:var(--fg-dim);margin-top:4px;min-height:1.2em;"></div>

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
  <div id="hexLegend" class="hex-legend" style="display:flex">
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#4e9a06"></div><span style="color:#4e9a06">GAP 4E</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#6a7fb0"></div><span style="color:#6a7fb0">Sync 00</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#4fc3f7"></div><span style="color:#4fc3f7">Sync</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#f6d32d"></div><span style="color:#f6d32d">IDAM</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#ff9800"></div><span style="color:#ff9800">CHRN</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#ce93d8"></div><span style="color:#ce93d8">CRC</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#ffab40"></div><span style="color:#ffab40">DAM FB</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#ff7043"></div><span style="color:#ff7043">DAM F8</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:#e01b24"></div><span style="color:#e01b24">CRC erreur</span></div>
    <div class="hex-leg-item"><div class="hex-leg-dot" style="background:rgba(246,211,45,.4)"></div><span style="color:#f6d32d">Weak</span></div>
  </div>
  <div id="mfmView"  style="display:none">
    <div class="mfm-wrap"><canvas id="mfmCanvas" width="768" height="4"></canvas></div>
    <div class="mfm-legend">
      <div class="leg-item"><div class="leg-swatch" style="background:#ffcc00"></div> Weak bit</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#ff3030"></div> Violation (11)</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#55aa22;border:1px solid #0e1a0e"></div> GAP 4E</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#4466bb;border:1px solid #0c1022"></div> Sync 00</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#22aaee;border:1px solid #082238"></div> Sync</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#eebb22;border:1px solid #382804"></div> IDAM</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#dd8818;border:1px solid #301a04"></div> CHRN</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#aa55dd;border:1px solid #220e2e"></div> CRC</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#ddaa22;border:1px solid #302204"></div> DAM FB</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#dd6618;border:1px solid #381406"></div> DAM F8</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#ee2222;border:1px solid #3a0808"></div> CRC erreur</div>
      <div class="leg-item"><div class="leg-swatch" style="background:#bbbbbb;border:1px solid #161616"></div> Data</div>
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

// CRC-CCITT (poly 0x1021, init 0xFFFF) — used by 765 FDC
function crc16(bytes) {
    let crc = 0xFFFF;
    for (const b of bytes) {
        crc ^= (b << 8);
        for (let j = 0; j < 8; j++)
            crc = (crc & 0x8000) ? (((crc << 1) ^ 0x1021) & 0xFFFF) : ((crc << 1) & 0xFFFF);
    }
    return crc;
}

function decodeBytesAt(bits, startBit, count, bitSize) {
    const out = new Uint8Array(count);
    for (let i = 0; i < count; i++)
        out[i] = decodeByte(bits, (startBit + i * 16) % bitSize, bitSize);
    return out;
}

// Full region map — one entry per decoded MFM byte.
// Pass 1: classify by decoded value — 0x4E=GAP4E(1), 0x00=sync-preamble(2), A1/C2=sync(3).
// Pass 2: overlay sector structure from Sugarbox offsets (takes priority).
// Pass 3: compute CRC-CCITT ourselves; mark CRC bytes as error (10) when wrong.
// Pass 4: fill remaining unknowns in inter-sector gap areas.
function buildFullRegionMap(bits, bitSize, bitOff, sectors) {
    const nbBytes = Math.floor(bitSize / 16);
    const map = new Uint8Array(nbBytes);

    // setRange: exclusive end — exactly Math.ceil(lenBits/16) bytes marked
    function setRange(startBit, lenBits, code) {
        const adj = ((startBit - bitOff) % bitSize + bitSize) % bitSize;
        const b0 = Math.floor(adj / 16);
        const b1 = Math.min(b0 + Math.ceil(lenBits / 16), nbBytes);
        for (let b = b0; b < b1; b++) map[b] = code;
    }
    function fillGap(startBit, lenBits) {
        const adj = ((startBit - bitOff) % bitSize + bitSize) % bitSize;
        const b0 = Math.floor(adj / 16);
        const b1 = Math.min(b0 + Math.ceil(lenBits / 16), nbBytes);
        for (let b = b0; b < b1; b++) if (map[b] === 0) map[b] = 1;
    }

    // Pass 1: classify bytes by decoded MFM value
    for (let i = 0; i < nbBytes; i++) {
        const val = decodeByte(bits, (bitOff + i * 16) % bitSize, bitSize);
        if      (val === 0x4E)                   map[i] = 1; // GAP 4E
        else if (val === 0x00)                   map[i] = 2; // sync preamble 00
        else if (val === 0xA1 || val === 0xC2)   map[i] = 3; // sync mark
    }

    // Pass 2: sector structure (overrides pass 1)
    for (const s of (sectors || [])) {
        setRange(s.idamOffset,                        48,              3); // sync A1×3
        setRange(s.idamOffset + 48,                   16,              4); // IDAM FE
        setRange(s.idamOffset + 64,                   64,              5); // CHRN
        setRange(s.idamOffset + 128,                  32,              6); // hdr CRC
        setRange(s.damOffset,                         48,              3); // sync A1×3
        setRange(s.damOffset  + 48,                   16,              s.deleted ? 11 : 7); // DAM FB/F8
        setRange(s.damOffset  + 64,                   s.realSize * 16, 8); // data
        setRange(s.damOffset  + 64 + s.realSize * 16, 32,              9); // data CRC
    }

    // Pass 3: compute CRC ourselves — mark CRC bytes red if mismatch
    for (const s of (sectors || [])) {
        // Header CRC: covers A1 A1 A1 FE C H R N (8 bytes)
        const hdrData    = decodeBytesAt(bits, s.idamOffset, 8, bitSize);
        const hdrCrcBytes = decodeBytesAt(bits, s.idamOffset + 128, 2, bitSize);
        if (crc16(hdrData) !== ((hdrCrcBytes[0] << 8) | hdrCrcBytes[1]))
            setRange(s.idamOffset + 128, 32, 10);
        // Data CRC: covers A1 A1 A1 DAM + all data bytes (4 + realSize bytes)
        const dataBytes   = decodeBytesAt(bits, s.damOffset, 4 + s.realSize, bitSize);
        const dataCrcBytes = decodeBytesAt(bits, s.damOffset + 64 + s.realSize * 16, 2, bitSize);
        if (crc16(dataBytes) !== ((dataCrcBytes[0] << 8) | dataCrcBytes[1]))
            setRange(s.damOffset + 64 + s.realSize * 16, 32, 10);
    }

    // Pass 4: fill unknown bytes in inter-sector gaps
    const sorted = [...(sectors || [])].sort((a, b) => a.idamOffset - b.idamOffset);
    if (sorted.length > 0) fillGap(0, sorted[0].idamOffset);
    for (let i = 0; i < sorted.length; i++) {
        const gapStart = sorted[i].indexEnd ?? (sorted[i].damOffset + 64 + sorted[i].realSize * 16 + 32);
        const gapEnd   = i + 1 < sorted.length ? sorted[i + 1].idamOffset : bitSize;
        if (gapEnd > gapStart) fillGap(gapStart, gapEnd - gapStart);
    }

    return map;
}

// ── Hex view renderer ─────────────────────────────────────────────────────────

// 0=unknown 1=GAP4E 2=sync-preamble(00) 3=sync 4=IDAM 5=CHRN 6=hdrCRC
// 7=DAM(FB) 8=data 9=dataCRC 10=CRCerr 11=DAM(F8,deleted)
const REGION_CLASS   = ['','h-gap1','h-pre0','h-sync','h-idam','h-chrn','h-hcrc','h-dam','h-data','h-dcrc','h-crcerr','h-dam-del'];
const REGION_TOOLTIP = ['','GAP 4E','Sync 00','Sync A1/C2','IDAM (FE)','CHRN','Hdr CRC','DAM (FB)','Data','Data CRC','CRC Error','DAM (F8) Deleted'];

function renderHexView(data, bitOff) {
    const bits     = unpackBits(data.bits, data.bitSize);
    const weakBits = data.weakBits ? unpackBits(data.weakBits, data.bitSize) : null;
    const nbBytes  = Math.floor(data.bitSize / 16);
    const regMap   = buildFullRegionMap(bits, data.bitSize, bitOff, data.sectors);
    const COLS     = 16;

    // Weak byte map: byte is weak if any of its 16 MFM bits is a weak bit
    let weakByteMap = null;
    if (weakBits) {
        weakByteMap = new Uint8Array(nbBytes);
        for (let i = 0; i < nbBytes; i++) {
            const base = (bitOff + i * 16) % data.bitSize;
            for (let k = 0; k < 16; k++) {
                if (weakBits[(base + k) % data.bitSize]) { weakByteMap[i] = 1; break; }
            }
        }
    }

    const container = document.getElementById('hexView');
    const chunks = [];
    for (let row = 0; row < Math.ceil(nbBytes / COLS); row++) {
        const bitPos = (bitOff + row * COLS * 16) % data.bitSize;
        chunks.push(\`<div class="hex-row"><span class="hex-offset">\${hex6(bitPos)}:</span><span class="hex-bytes">\`);
        for (let col = 0; col < COLS; col++) {
            const i = row * COLS + col;
            if (i >= nbBytes) break;
            const val  = decodeByte(bits, (bitOff + i * 16) % data.bitSize, data.bitSize);
            const rCls = REGION_CLASS[regMap[i]] || '';
            const wCls = (weakByteMap && weakByteMap[i]) ? ' h-weak' : '';
            const tip  = REGION_TOOLTIP[regMap[i]] || '';
            const sp   = (col === 7) ? ' &nbsp;' : '';
            chunks.push(\`<span class="hb \${rCls}\${wCls}" title="\${tip}">\${hex2(val)}</span>\${sp} \`);
        }
        chunks.push('</span></div>');
    }
    container.innerHTML = chunks.join('');
}

// ── MFM canvas renderer ───────────────────────────────────────────────────────

const C_VIOL = [0xff, 0x30, 0x30, 0xff];  // MFM violation (11) — priority
const C_WEAK = [0xff, 0xcc, 0x00, 0xff];  // weak bit — priority

// Per-region colors: BG = 0-bit (dark), FG = 1-bit (bright same hue)
const REGION_BG = [
    [0x1e, 0x1e, 0x1e, 0xff], // 0  unknown
    [0x0e, 0x1a, 0x0e, 0xff], // 1  GAP 4E
    [0x0c, 0x10, 0x22, 0xff], // 2  sync 00
    [0x08, 0x22, 0x38, 0xff], // 3  sync
    [0x38, 0x28, 0x04, 0xff], // 4  IDAM
    [0x30, 0x1a, 0x04, 0xff], // 5  CHRN
    [0x22, 0x0e, 0x2e, 0xff], // 6  hdr CRC
    [0x30, 0x22, 0x04, 0xff], // 7  DAM FB
    [0x16, 0x16, 0x16, 0xff], // 8  data
    [0x22, 0x0e, 0x2e, 0xff], // 9  data CRC
    [0x3a, 0x08, 0x08, 0xff], // 10 CRC error
    [0x38, 0x14, 0x06, 0xff], // 11 DAM F8
];
const REGION_FG = [
    [0x77, 0x77, 0x77, 0xff], // 0  unknown
    [0x55, 0xaa, 0x22, 0xff], // 1  GAP 4E
    [0x44, 0x66, 0xbb, 0xff], // 2  sync 00
    [0x22, 0xaa, 0xee, 0xff], // 3  sync
    [0xee, 0xbb, 0x22, 0xff], // 4  IDAM
    [0xdd, 0x88, 0x18, 0xff], // 5  CHRN
    [0xaa, 0x55, 0xdd, 0xff], // 6  hdr CRC
    [0xdd, 0xaa, 0x22, 0xff], // 7  DAM FB
    [0xbb, 0xbb, 0xbb, 0xff], // 8  data
    [0xaa, 0x55, 0xdd, 0xff], // 9  data CRC
    [0xee, 0x22, 0x22, 0xff], // 10 CRC error
    [0xdd, 0x66, 0x18, 0xff], // 11 DAM F8
];

function renderMfmCanvas(data) {
    const bits       = unpackBits(data.bits, data.bitSize);
    const weakBits   = data.weakBits ? unpackBits(data.weakBits, data.bitSize) : null;
    // Use bitOff=0: index 0 of bitfield = physical track start (index hole)
    const byteRegMap = buildFullRegionMap(bits, data.bitSize, 0, data.sectors);
    // Expand byte-level map to bit-level (16 MFM bits per decoded byte)
    const bitRegMap  = new Uint8Array(data.bitSize);
    for (let i = 0; i < data.bitSize; i++) bitRegMap[i] = byteRegMap[Math.floor(i / 16)];

    const BITS_PER_ROW = 512;  // 32 decoded bytes per row
    const BIT_W = 1.5;
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

            const val  = bits[bitIdx];
            const prev = bitIdx > 0 ? bits[bitIdx - 1] : 0;
            // Weak bits take priority: a weak bit can cause an apparent 11 violation
            const isWeak = weakBits ? weakBits[bitIdx] : false;
            const isViol = !isWeak && (val === 1 && prev === 1);

            const reg = bitRegMap[bitIdx];
            let color;
            if (isWeak)         color = C_WEAK;
            else if (isViol)    color = C_VIOL;
            else if (val === 0) color = REGION_BG[reg];
            else                color = REGION_FG[reg];

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
    document.getElementById('hexLegend').style.display   = tab === 'hex' ? 'flex'  : 'none';
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
document.getElementById('btnInsertA').addEventListener('click', () => vscode.postMessage({ type: 'insertDisk', drive: 0 }));
document.getElementById('btnInsertB').addEventListener('click', () => vscode.postMessage({ type: 'insertDisk', drive: 1 }));

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
        case 'insertResult': {
            const el = document.getElementById('insertStatus');
            if (msg.error) {
                el.style.color = '#f48771';
                el.textContent = '✗ ' + msg.error;
            } else {
                const letter = msg.drive === 0 ? 'A' : 'B';
                const base = msg.path.replace(/.*[\\\\/]/, '');
                el.style.color = '#73c991';
                el.textContent = '✓ Drive ' + letter + ': ' + base;
            }
            break;
        }
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
