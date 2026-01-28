import * as net from "net";

export type EmulatorEvent = {
    type: "event";
    event: string;
    body?: any;
};

export class EmulatorClient {
    private socket!: net.Socket;
    private buffer = "";
    private pendingResolve: ((msg: any) => void) | null = null;
    public onEvent?: (evt: EmulatorEvent) => void;

    connect(port = 1234, host = "127.0.0.1") {
        return new Promise<void>((resolve, reject) => {
            this.socket = net.createConnection(port, host, () => {
                resolve();
            });
            this.socket.on("data", data => this.onData(data));
            this.socket.on("error", reject);
            this.socket.on("close", () => console.log("Emulator socket closed"));
        });
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
                } else if (this.pendingResolve) {
                    this.pendingResolve(msg);
                    this.pendingResolve = null;
                }
            } catch (e) {
                console.error("Invalid JSON from emulator:", line);
            }
        }
    }

    send(cmd: any): Promise<any> {
        return new Promise((resolve, reject) => {
            if (!this.socket || this.socket.destroyed) {
                reject(new Error("Socket not connected"));
                return;
            }
            this.pendingResolve = resolve;
            const msg = JSON.stringify(cmd) + "\n";
            this.socket.write(msg);
            // Timeout de sécurité
            setTimeout(() => {
                if (this.pendingResolve) {
                    this.pendingResolve = null;
                    reject(new Error("Emulator did not respond in time"));
                }
            }, 1000);
        });
    }
}
