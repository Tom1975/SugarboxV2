import * as vscode from "vscode";
import * as fs from "fs";
import * as nodePath from "path";

type Messages = Record<string, string>;

let _messages: Messages = {};

/**
 * Call once in activate() — loads the JSON bundle that matches VS Code's
 * display language, falling back to English when no translation is found.
 * Adding a new language is as simple as dropping `i18n/<lang>.json` next
 * to the extension and reloading.
 */
export function initI18n(extensionPath: string): void {
    const locale = vscode.env.language.toLowerCase(); // e.g. "fr-fr", "en"
    const lang   = locale.split("-")[0];              // "fr", "en"

    for (const candidate of [locale, lang, "en"]) {
        const p = nodePath.join(extensionPath, "i18n", `${candidate}.json`);
        if (fs.existsSync(p)) {
            try {
                _messages = JSON.parse(fs.readFileSync(p, "utf8"));
                return;
            } catch { /* try next candidate */ }
        }
    }
    // No bundle found — t() will return the key itself (always readable)
}

/**
 * Translate a key.  Falls back to the key string if not found.
 * Positional placeholders: {0}, {1}, …
 *
 * @example t("cmd.addBreakpoint.failed", err.message)
 */
export function t(key: string, ...args: (string | number)[]): string {
    let msg = _messages[key] ?? key;
    for (let i = 0; i < args.length; i++) {
        msg = msg.split(`{${i}}`).join(String(args[i]));
    }
    return msg;
}
