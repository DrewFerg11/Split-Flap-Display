export function diagnostics() {
    let stream = null;
    let timer = null;
    let controller = null;
    let generation = 0;
    let cursor = 0;

    return {
        expanded: false,
        paused: false,
        status: null,
        statusError: "",
        logError: "",
        text: "",

        async toggle() {
            this.expanded = !this.expanded;
            if (!this.expanded) {
                this.disconnect();
                return;
            }
            const current = ++generation;
            controller = new AbortController();
            this.refreshStatus(current);
            this.logError = "";
            try {
                const response = await fetch("/log", {
                    cache: "no-store",
                    signal: controller.signal,
                });
                if (!response.ok) throw new Error("Log request failed");
                const text = await response.text();
                if (current !== generation) return;
                cursor = Number(response.headers.get("X-Log-Cursor") || 0);
                this.setText(text);
                stream = new EventSource("/log/stream");
                stream.addEventListener("snapshot", (event) => {
                    if (current !== generation) return;
                    const packet = JSON.parse(event.data);
                    cursor = packet.end;
                    this.setText(packet.text);
                    this.logError = "";
                });
                stream.addEventListener("log", (event) => {
                    if (current !== generation) return;
                    this.accept(JSON.parse(event.data));
                });
                stream.onerror = () => {
                    if (current === generation) {
                        this.logError =
                            "Log stream disconnected; reconnecting…";
                    }
                };
            } catch (error) {
                if (current === generation && error.name !== "AbortError") {
                    this.logError =
                        "Unable to load logs. Close and reopen to retry.";
                }
            }
        },

        async refreshStatus(current) {
            try {
                const response = await fetch("/status", {
                    cache: "no-store",
                    signal: controller.signal,
                });
                if (!response.ok) throw new Error("Status request failed");
                const status = await response.json();
                if (current !== generation) return;
                this.status = status;
                this.statusError = "";
            } catch (error) {
                if (current === generation && error.name !== "AbortError") {
                    this.statusError = "Status unavailable; retrying…";
                }
            }
            if (current === generation) {
                timer = setTimeout(() => this.refreshStatus(current), 5000);
            }
        },

        accept(packet) {
            if (packet.end <= cursor) return;
            if (packet.start > cursor) {
                this.logError = "Some log data was lost; reconnecting…";
                // A new connection requests a fresh snapshot from the firmware.
                stream.close();
                this.disconnect();
                this.expanded = false;
                this.toggle();
                return;
            }
            const bytes = new TextEncoder().encode(packet.text);
            const tail = bytes.slice(Math.max(0, cursor - packet.start));
            cursor = packet.end;
            this.setText(this.text + new TextDecoder().decode(tail));
            this.logError = "";
        },

        setText(text) {
            // Bound both line count and a pathological single very long line.
            this.text = text.slice(-256000).split("\n").slice(-2000).join("\n");
            this.scroll();
        },

        scroll() {
            if (this.paused) return;
            this.$nextTick(() => {
                const box = this.$refs.logBox;
                if (box) box.scrollTop = box.scrollHeight;
            });
        },

        clear() {
            this.text = "";
        },

        download() {
            const url = URL.createObjectURL(
                new Blob([this.text], { type: "text/plain" }),
            );
            const link = document.createElement("a");
            link.href = url;
            link.download = "split-flap-log.txt";
            link.click();
            setTimeout(() => URL.revokeObjectURL(url), 1000);
        },

        disconnect() {
            generation++;
            controller?.abort();
            controller = null;
            stream?.close();
            stream = null;
            clearTimeout(timer);
            timer = null;
        },

        destroy() {
            this.disconnect();
        },

        get uptime() {
            const seconds = Math.floor((this.status?.uptimeMs || 0) / 1000);
            return `${Math.floor(seconds / 3600)}h ${Math.floor((seconds % 3600) / 60)}m ${seconds % 60}s`;
        },

        get abnormalReset() {
            return /brownout|panic|watchdog/i.test(
                this.status?.resetReason || "",
            );
        },
    };
}
