import * as net from "net";

export type EmulatorEvent = {
    type: "event";
    event: string;
    body?: any;
};

interface QueueEntry {
    msg:     string;
    resolve: (v: any)    => void;
    reject:  (e: Error)  => void;
}

/**
 * TCP client for the Sugarbox debug server.
 *
 * Commands are serialised through an internal queue: only one command is
 * in-flight at a time.  This prevents the earlier single-pendingResolve
 * design from misfiring when several callers (e.g. hardware-panel refresh)
 * issue concurrent send() calls.
 *
 * Emulator events (type === "event") are dispatched independently via
 * onEvent and never go through the queue.
 */
export class EmulatorClient {
    private socket!: net.Socket;
    private buffer  = "";
    private queue: QueueEntry[] = [];
    private inflight: ((msg: any) => void) | null = null;
    public  onEvent?: (evt: EmulatorEvent) => void;

    connect(port = 1234, host = "127.0.0.1") {
        return new Promise<void>((resolve, reject) => {
            this.socket = net.createConnection(port, host, () => { resolve(); });
            this.socket.on("data",  data => this.onData(data));
            this.socket.on("error", err  => { reject(err); });
            this.socket.on("close", ()   => console.log(`EmulatorClient: socket closed (port ${port})`));
        });
    }

    disconnect() {
        if (this.socket && !this.socket.destroyed) {
            this.socket.destroy();
        }
    }

    send(cmd: any): Promise<any> {
        return new Promise<any>((resolve, reject) => {
            if (!this.socket || this.socket.destroyed) {
                reject(new Error("Socket not connected"));
                return;
            }
            this.queue.push({ msg: JSON.stringify(cmd) + "\n", resolve, reject });
            this.flush();
        });
    }

    // ── Internal ─────────────────────────────────────────────────────────────

    private flush(): void {
        if (this.inflight || this.queue.length === 0) return;

        const { msg, resolve, reject } = this.queue.shift()!;

        let settled = false;

        const timer = setTimeout(() => {
            if (settled) return;
            settled         = true;
            this.inflight   = null;
            reject(new Error("Emulator did not respond in time"));
            this.flush();   // continue with next queued command
        }, 10_000);

        this.inflight = (response: any) => {
            if (settled) return;
            settled       = true;
            clearTimeout(timer);
            this.inflight = null;
            resolve(response);
            this.flush();   // continue with next queued command
        };

        this.socket.write(msg);
    }

    private onData(data: Buffer) {
        this.buffer += data.toString();
        let idx: number;
        while ((idx = this.buffer.indexOf("\n")) !== -1) {
            const line = this.buffer.slice(0, idx);
            this.buffer = this.buffer.slice(idx + 1);
            if (!line.trim()) continue;
            try {
                const msg = JSON.parse(line);
                if (msg.type === "event") {
                    this.onEvent?.(msg);
                } else if (this.inflight) {
                    this.inflight(msg);
                } else {
                    console.warn("EmulatorClient: received response with no pending command:", line);
                }
            } catch (e) {
                console.error("EmulatorClient: invalid JSON from emulator:", line);
            }
        }
    }
}
