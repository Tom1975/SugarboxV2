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
    // name → address (reverse lookup for label breakpoints)
    private nameToAddress: Map<string, number> = new Map();
    private symbols: SymbolEntry[] = [];

    get size(): number { return this.symbols.length; }

    /** Returns all label names defined at a given address. */
    getLabelsAt(address: number): string[] {
        return this.addressToNames.get(address) ?? [];
    }

    /** Resolves a label name to its address, or undefined if not found. */
    resolveLabel(name: string): number | undefined {
        return this.nameToAddress.get(name);
    }

    /** Returns all known label names (for completions). */
    getAllNames(): string[] {
        return Array.from(this.nameToAddress.keys());
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
        // First definition wins for the reverse map (aliases don't override labels)
        if (!this.nameToAddress.has(entry.name)) {
            this.nameToAddress.set(entry.name, entry.address);
        }
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

            const parts = token.split(/\s+/);
            const tag   = parts[0];

            // label NAME ADDR BANK  — RAM label (regular assembler label)
            // romlabel NAME ADDR BANK — ROM label
            if ((tag === "label" || tag === "romlabel") && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                const bank = parts.length >= 4 ? parseInt(parts[3], 10) : undefined;
                if (!isNaN(addr)) {
                    table.addEntry({ name: parts[1], address: addr, bank });
                }
                continue;
            }

            // alias NAME VALUE — EQU constant
            if (tag === "alias" && parts.length >= 3) {
                const addr = parseInt(parts[2], 10);
                if (!isNaN(addr)) {
                    table.addEntry({ name: parts[1], address: addr });
                }
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

        const snaVersion = buf[16];  // byte 16 of SNA header = version (0/1=v1, 2=v2, 3=v3)
        console.log(`SymbolTable: SNA file size=${buf.length}, header version byte=${snaVersion}`);

        // SNA v3: 256-byte header, then chunks of [4-byte id][4-byte LE size][data].
        // A v3 SNA can be much smaller than 65792 bytes (no full RAM dump needed),
        // so do NOT short-circuit on file size — just scan chunks from offset 256.
        let offset = 256;
        const foundChunks: string[] = [];
        while (offset + 8 <= buf.length) {
            const chunkId   = buf.toString("ascii", offset, offset + 4);
            const chunkSize = buf.readUInt32LE(offset + 4);
            foundChunks.push(`${chunkId}(${chunkSize})`);
            offset += 8;
            if (offset + chunkSize > buf.length) {
                console.log(`SymbolTable: chunk ${chunkId} size=${chunkSize} overflows file — stopping`);
                break;
            }

            if (chunkId === "REMU") {
                const text = buf.toString("ascii", offset, offset + chunkSize);
                console.log(`SymbolTable: found REMU chunk (${chunkSize} bytes)`);
                return SymbolTable._parseRemuText(text);
            }
            offset += chunkSize;
        }
        console.log(`SymbolTable: no REMU chunk found. Chunks: ${foundChunks.join(", ")}`);
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
