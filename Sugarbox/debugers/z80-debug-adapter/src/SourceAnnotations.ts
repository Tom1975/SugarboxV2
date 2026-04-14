import * as fs from "fs";

export interface LabelAnnotation {
    /** Inline comment on the label line, e.g. "; prints a null-terminated string" */
    comment: string;
    /** Standalone comment/blank lines immediately preceding the label, trimmed */
    preamble: string[];
}

/**
 * Extracts source-level annotations (comments) from a Z80 assembly file.
 *
 * For each label found in the source, records:
 *   - the block of comment/blank lines immediately before it (preamble)
 *   - the inline comment on the label line itself
 *
 * These annotations are used to enrich the disassembly view shown in VS Code.
 */
export class SourceAnnotations {
    private byLabel: Map<string, LabelAnnotation> = new Map();

    getAnnotation(labelName: string): LabelAnnotation | undefined {
        return this.byLabel.get(labelName);
    }

    static fromFile(filePath: string): SourceAnnotations {
        const ann = new SourceAnnotations();
        let content: string;
        try {
            content = fs.readFileSync(filePath, "utf-8");
        } catch (e) {
            console.warn(`SourceAnnotations: cannot read ${filePath}:`, e);
            return ann;
        }

        const lines = content.split(/\r?\n/);
        let preamble: string[] = [];

        for (const line of lines) {
            const trimmed = line.trim();

            // Standalone comment or blank line — accumulate as potential preamble
            if (trimmed.startsWith(";") || trimmed === "") {
                preamble.push(line.trimEnd());
                continue;
            }

            // Label at column 0 (RASM convention): "labelname:" optionally followed
            // by an instruction and/or a comment.
            // We only match labels starting at column 0 (no leading whitespace).
            const labelMatch = line.match(/^(\w+)\s*:(.*)/);
            if (labelMatch) {
                const labelName = labelMatch[1];
                const rest = labelMatch[2].trim();

                // Extract inline comment (everything from the first ";" onward)
                let inlineComment = "";
                const semicolonIdx = rest.indexOf(";");
                if (semicolonIdx !== -1) {
                    inlineComment = rest.slice(semicolonIdx).trim();
                }

                ann.byLabel.set(labelName, {
                    comment: inlineComment,
                    preamble: trimPreamble(preamble),
                });

                // Reset preamble after consuming it
                preamble = [];
                continue;
            }

            // Any other line (instruction, directive) — reset preamble accumulator
            preamble = [];
        }

        console.log(`SourceAnnotations: ${ann.byLabel.size} annotated label(s) from ${filePath}`);
        return ann;
    }
}

/** Remove leading and trailing blank lines from a preamble block. */
function trimPreamble(lines: string[]): string[] {
    let start = 0;
    while (start < lines.length && lines[start].trim() === "") start++;
    let end = lines.length - 1;
    while (end >= start && lines[end].trim() === "") end--;
    return lines.slice(start, end + 1);
}
