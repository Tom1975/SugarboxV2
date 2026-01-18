import * as net from "net";

export class EmulatorClient {
    private socket!: net.Socket;
    private buffer = "";

    connect(port = 1234, host = "127.0.0.1") {
        return new Promise<void>((resolve) => {
            this.socket = net.createConnection(port, host, () => {
                resolve();
            });

            this.socket.on("data", data => {
                this.buffer += data.toString();
            });
        });
    }

    async send(cmd: any): Promise<any> {
        const msg = JSON.stringify(cmd) + "\n";
        this.socket.write(msg);

        return new Promise(resolve => {
            const interval = setInterval(() => {
                const idx = this.buffer.indexOf("\n");
                if (idx !== -1) {
                    const line = this.buffer.slice(0, idx);
                    this.buffer = this.buffer.slice(idx + 1);
                    clearInterval(interval);
                    resolve(JSON.parse(line));
                }
            }, 1);
        });
    }
}
