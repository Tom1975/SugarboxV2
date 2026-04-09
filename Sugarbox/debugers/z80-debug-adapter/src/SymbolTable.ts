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

    get size(): number { return this.symbols.length; }

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

    /** Merge all entries from another SymbolTable into this one. */
    merge(other: SymbolTable): void {
        for (const entry of other.symbols) {
            this.addEntry(entry);
        }
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

    // ─── Super snapshot (REMU chunk) loader ─────────────────────────────────

    /**
     * Extract symbols and breakpoint addresses from the REMU chunk of a RASM
     * super-snapshot (.sna v3).  Returns an empty result if the file cannot be
     * read or contains no REMU chunk.
     *
     * REMU is pure ASCII, semicolon-separated tags:
     *   brk ADDR BANK          — exec BP in RAM
     *   rombrk ADDR ROM        — exec BP in ROM (stored but not yet used)
     *   label NAME ADDR BANK   — RAM symbol
     *   romlabel NAME ADDR ROM — ROM symbol
     *   alias NAME VALUE       — constant
     *   comz / romcomz         — comments, ignored
     */
    static fromSnapshotRemu(snapshotPath: string): { table: SymbolTable; breakpoints: number[] } {
        const empty = { table: new SymbolTable(), breakpoints: [] as number[] };
        let buf: Buffer;
        try {
            buf = fs.readFileSync(snapshotPath);
        } catch {
            return empty;
        }

        // SNA v3: 256-byte header, then chunks of [4-byte id][4-byte LE size][data]
        let offset = 256;
        while (offset + 8 <= buf.length) {
            const chunkId  = buf.toString("ascii", offset, offset + 4);
            const chunkSize = buf.readUInt32LE(offset + 4);
            offset += 8;
            if (offset + chunkSize > buf.length) break;

            if (chunkId === "REMU") {
                const text = buf.toString("ascii", offset, offset + chunkSize);
                return SymbolTable._parseRemuText(text);
            }
            offset += chunkSize;
        }
        return empty;
    }

    private static _parseRemuText(text: string): { table: SymbolTable; breakpoints: number[] } {
        const table = new SymbolTable();
        const breakpoints: number[] = [];

        for (const raw of text.split(";")) {
            const token = raw.trim();
            if (!token) continue;
            const parts = token.split(/\s+/);
            const tag = parts[0];

            if (tag === "brk" && parts.length >= 2) {
                const addr = parseInt(parts[1], 10);
                if (!isNaN(addr)) breakpoints.push(addr);

            } else if (tag === "label" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr)) table.addEntry({ name: parts[1], address: addr, bank });

            } else if (tag === "romlabel" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr)) table.addEntry({ name: parts[1], address: addr, bank });

            } else if (tag === "alias" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                if (!isNaN(addr)) table.addEntry({ name: parts[1], address: addr });
            }
            // comz, romcomz — ignored
        }

        console.log(`SymbolTable: REMU — ${table.symbols.length} symbols, ${breakpoints.length} breakpoints`);
        return { table, breakpoints };
    }

    // ─── Future loaders ─────────────────────────────────────────────────────
    // static fromSjasmplus(filePath: string): SymbolTable { ... }
    // static fromPasmo(filePath: string): SymbolTable { ... }
}
