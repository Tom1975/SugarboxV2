import * as net from "net";

export class EmulatorClient {
    private socket!: net.Socket;
    private buffer = "";

    onEvent?: (evt: any) => void;

    connect(port = 1234, host = "127.0.0.1") {
        return new Promise<void>((resolve, reject) => {
            this.socket = net.createConnection(port, host, () => {
                resolve();
            });

            this.socket.on("data", data => {
                this.onData(data);
            });

            this.socket.on("close", () => {
                console.log("Emulator disconnected");
            });

            this.socket.on("error", err => {
                console.error("Socket error", err);
                reject(err);
            });
        });
    }

    private onData(data: Buffer) {
        this.buffer += data.toString();

        let idx;
        while ((idx = this.buffer.indexOf("\n")) !== -1) {
            const line = this.buffer.slice(0, idx);
            this.buffer = this.buffer.slice(idx + 1);

            const msg = JSON.parse(line);

            if (msg.type === "event") {
                this.onEvent?.(msg);
            } else {
                this.pendingResolve?.(msg);
                this.pendingResolve = undefined;
            }
        }
    }

    private pendingResolve?: (msg: any) => void;

    send(cmd: any): Promise<any> {
        if (!this.socket) {
            throw new Error("Not connected");
        }

        this.socket.write(JSON.stringify(cmd) + "\n");

        return new Promise(resolve => {
            this.pendingResolve = resolve;
        });
    }

}
