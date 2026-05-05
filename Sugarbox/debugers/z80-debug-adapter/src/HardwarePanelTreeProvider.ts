import * as vscode from "vscode";

interface PanelEntry {
    label: string;
    command: string;
    ready: boolean;  // false → "coming soon" placeholder
}

const PANELS: PanelEntry[] = [
    { label: "CRTC / ASIC",     command: "z80debug.showCrtcPanel",     ready: true  },
    { label: "Gate Array",       command: "z80debug.showGateArrayPanel", ready: true  },
    { label: "PSG (AY-3-8912)", command: "z80debug.showPsgPanel",      ready: true  },
    { label: "FDC (µPD765)",    command: "z80debug.showFdcPanel",      ready: true  },
    { label: "PPI (8255)",       command: "z80debug.showPpiPanel",      ready: true  },
    { label: "Cassette",         command: "z80debug.showTapePanel",     ready: true  },
];

export class HardwarePanelItem extends vscode.TreeItem {
    constructor(entry: PanelEntry) {
        super(entry.label, vscode.TreeItemCollapsibleState.None);
        this.iconPath = new vscode.ThemeIcon(
            entry.ready ? "circuit-board" : "watch"
        );
        this.tooltip = entry.ready ? `Open ${entry.label} panel` : "Not yet implemented";
        if (entry.ready) {
            this.command = {
                command: entry.command,
                title:   `Open ${entry.label}`,
            };
        }
        this.contextValue = entry.ready ? "hardwarePanel" : "hardwarePanelPending";
    }
}

export class HardwarePanelTreeProvider
    implements vscode.TreeDataProvider<HardwarePanelItem>
{
    private _onDidChangeTreeData = new vscode.EventEmitter<void>();
    readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

    getTreeItem(item: HardwarePanelItem): vscode.TreeItem { return item; }

    getChildren(): HardwarePanelItem[] {
        return PANELS.map(p => new HardwarePanelItem(p));
    }

    refresh(): void { this._onDidChangeTreeData.fire(); }
}
