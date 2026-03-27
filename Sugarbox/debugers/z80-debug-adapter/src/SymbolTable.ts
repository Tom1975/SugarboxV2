import * as fs from "fs";

export interface SymbolEntry {
    name: string;
    address: number;
    bank?: number;
}

/**
 * Symbol table built from assembler output files.
 * Supports multiple loaders (RASM for now).
 */
export class SymbolTable {
    // address → list of symbol names (multiple labels can share an address)
    private addressToNames: Map<number, string[]> = new Map();
    private symbols: SymbolEntry[] = [];

    /** Returns all label names defined at a given address. */
    getLabelsAt(address: number): string[] {
        return this.addressToNames.get(address) ?? [];
    }

    /** True if any label exists in [startAddr, endAddr). */
    hasLabelsInRange(startAddr: number, endAddr: number): boolean {
        for (const addr of this.addressToNames.keys()) {
            if (addr >= startAddr && addr < endAddr) return true;
        }
        return false;
    }

    private addEntry(entry: SymbolEntry): void {
        this.symbols.push(entry);
        const existing = this.addressToNames.get(entry.address) ?? [];
        existing.push(entry.name);
        this.addressToNames.set(entry.address, existing);
    }

    // ─── RASM loader ────────────────────────────────────────────────────────

    /**
     * Parse a RASM super-symbol file (.rasm).
     * Format (all on one line, semicolon-separated):
     *   romlabel NAME DECIMAL_ADDR BANK
     *   alias NAME DECIMAL_ADDR
     */
    static fromRasm(filePath: string): SymbolTable {
        const table = new SymbolTable();
        let content: string;
        try {
            content = fs.readFileSync(filePath, "utf-8");
        } catch (e) {
            console.error("SymbolTable: cannot read", filePath, e);
            return table;
        }

        // Entries are separated by semicolons (file is typically one long line)
        const entries = content.split(";");
        for (const raw of entries) {
            const token = raw.trim();
            if (!token) continue;

            // romlabel NAME ADDR BANK
            const romlabelMatch = token.match(/^romlabel\s+(\S+)\s+(\d+)\s+(\d+)$/);
            if (romlabelMatch) {
                table.addEntry({
                    name: romlabelMatch[1],
                    address: parseInt(romlabelMatch[2], 10),
                    bank: parseInt(romlabelMatch[3], 10),
                });
                continue;
            }

            // alias NAME ADDR
            const aliasMatch = token.match(/^alias\s+(\S+)\s+(\d+)$/);
            if (aliasMatch) {
                table.addEntry({
                    name: aliasMatch[1],
                    address: parseInt(aliasMatch[2], 10),
                });
                continue;
            }
        }

        console.log(`SymbolTable: loaded ${table.symbols.length} symbols from ${filePath}`);
        return table;
    }

    // ─── Future loaders ─────────────────────────────────────────────────────
    // static fromSjasmplus(filePath: string): SymbolTable { ... }
    // static fromPasmo(filePath: string): SymbolTable { ... }
}
