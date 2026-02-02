import Alpine from "alpinejs";
window.Alpine = Alpine;

document.addEventListener("alpine:init", () => {
    Alpine.data("page", (type) => ({
        get header() {
            return this.settings.name || "Split Flap";
        },

        loading: {
            settings: true,
            timezones: true,
            displayConfig: true,
        },
        saving: false,
        dialog: {
            show: false,
            message: "",
            type: null,
        },
        settings: {
            mode: 7, // Default to Per-Display Text mode
            dateFormat: "ddd dd/MM",
            timeFormat: "HH:mm",
        },
        errors: {},
        timezones: {},
        displayConfig: [], // Will be populated from /api/display-config

        // Control page specific
        singleMode: true,
        singleWord: "",
        multiWord: "",
        multiWords: [],
        delay: 1,
        centerText: false,
        centerDisplayText: false, // Center text for per-display mode
        displayTexts: {}, // Will be dynamically populated based on displayConfig

        // Mode 8: All Display Test
        testModeDelay: 5, // seconds between characters
        testModeSkip: 1, // characters to skip each cycle
        testModeCurrentChar: " ",
        testModeCycleCount: 0,
        testModePollIntervalId: null,

        get processing() {
            return (
                this.saving ||
                this.loading.settings ||
                this.loading.timezones ||
                this.loading.displayConfig
            );
        },

        get addressArray() {
            return (
                this.settings.moduleAddresses
                    ?.split(",")
                    .map((s) => s.trim()) || []
            );
        },
        setAddress(index, value) {
            const arr = this.addressArray;
            arr[index] = value;
            this.settings.moduleAddresses = arr.join(",");
        },

        get offsetArray() {
            return (
                this.settings.moduleOffsets?.split(",").map((s) => s.trim()) ||
                []
            );
        },
        setOffset(index, value) {
            const arr = this.offsetArray;
            arr[index] = value;
            this.settings.moduleOffsets = arr.join(",");
        },

        // Multi-mux helpers
        get muxAddresses() {
            if (!this.settings.muxAddrs) return [112]; // Default to 0x70 (112)
            return this.settings.muxAddrs
                .split(",")
                .map((s) => parseInt(s.trim()))
                .filter((n) => !isNaN(n));
        },

        addMux() {
            const muxes = this.muxAddresses;
            // Find next available address (112-119 = 0x70-0x77)
            let nextAddr = 112;
            while (muxes.includes(nextAddr) && nextAddr <= 119) {
                nextAddr++;
            }
            if (nextAddr <= 119) {
                muxes.push(nextAddr);
                this.settings.muxAddrs = muxes.join(",");
                // Initialize empty channel config for new mux
                this.settings[`chModAddrs${nextAddr}`] = ";;;;;;;";
            }
        },

        updateMuxAddress(oldAddr, newAddr) {
            newAddr = parseInt(newAddr);
            if (isNaN(newAddr) || newAddr < 112 || newAddr > 119) return;
            if (this.muxAddresses.includes(newAddr) && newAddr !== oldAddr) {
                // Address already in use
                return;
            }

            const muxes = this.muxAddresses.map((a) =>
                a === oldAddr ? newAddr : a,
            );
            this.settings.muxAddrs = muxes.join(",");

            // Move channel config to new key
            const oldKey = `chModAddrs${oldAddr}`;
            const newKey = `chModAddrs${newAddr}`;
            if (this.settings[oldKey]) {
                this.settings[newKey] = this.settings[oldKey];
                if (oldAddr !== newAddr) {
                    delete this.settings[oldKey];
                }
            }
        },

        removeMux(addr) {
            const muxes = this.muxAddresses.filter((a) => a !== addr);
            this.settings.muxAddrs = muxes.join(",");
            // Remove corresponding channel config
            delete this.settings[`chModAddrs${addr}`];
        },

        getMuxChannelConfig(muxAddr) {
            const key = `chModAddrs${muxAddr}`;
            if (!this.settings[key]) return Array(8).fill("");
            return this.settings[key].split(";").slice(0, 8);
        },

        setMuxChannelConfig(muxAddr, channelConfigs) {
            const key = `chModAddrs${muxAddr}`;
            // Ensure we have exactly 8 channel configs (semicolon-separated)
            while (channelConfigs.length < 8) channelConfigs.push("");
            this.settings[key] = channelConfigs.slice(0, 8).join(";");
        },

        getMuxChannelAddresses(muxAddr, ch) {
            const configs = this.getMuxChannelConfig(muxAddr);
            if (!configs[ch] || configs[ch] === "") return [];
            return configs[ch]
                .split(",")
                .map((s) => s.trim())
                .filter((s) => s !== "");
        },

        setMuxChannelAddresses(muxAddr, ch, addresses) {
            const configs = this.getMuxChannelConfig(muxAddr);
            configs[ch] = addresses.filter((a) => a !== "").join(",");
            this.setMuxChannelConfig(muxAddr, configs);
        },

        addModuleToMuxChannel(muxAddr, ch) {
            const addrs = this.getMuxChannelAddresses(muxAddr, ch);
            if (addrs.length >= 8) return; // Max 8 modules per channel

            // Find next available address starting from 32
            let nextAddr = 32;
            const usedAddrs = addrs.map((a) => parseInt(a));
            while (usedAddrs.includes(nextAddr) && nextAddr <= 119) {
                nextAddr++;
            }

            addrs.push(String(nextAddr));
            this.setMuxChannelAddresses(muxAddr, ch, addrs);
        },

        addChannelToMux(muxAddr) {
            // Find first empty channel and add a default module
            const configs = this.getMuxChannelConfig(muxAddr);
            for (let ch = 0; ch < 8; ch++) {
                if (!configs[ch] || configs[ch] === "") {
                    this.addModuleToMuxChannel(muxAddr, ch);
                    return;
                }
            }
        },

        channelHasModules(muxAddr, ch) {
            return this.getMuxChannelAddresses(muxAddr, ch).length > 0;
        },

        removeChannel(muxAddr, ch) {
            const configs = this.getMuxChannelConfig(muxAddr);
            configs[ch] = "";
            this.setMuxChannelConfig(muxAddr, configs);
        },

        removeModuleFromMuxChannel(muxAddr, ch, index) {
            const addrs = this.getMuxChannelAddresses(muxAddr, ch);
            addrs.splice(index, 1);
            this.setMuxChannelAddresses(muxAddr, ch, addrs);
        },

        setModuleAddress(muxAddr, ch, index, value) {
            const addrs = this.getMuxChannelAddresses(muxAddr, ch);
            addrs[index] = value;
            this.setMuxChannelAddresses(muxAddr, ch, addrs);
        },

        // Per-channel configuration helpers (DEPRECATED - kept for compatibility)
        getChannelModuleCount(ch) {
            if (!this.settings.chModCount) return 0;
            const counts = this.settings.chModCount
                .split(",")
                .map((s) => parseInt(s.trim()) || 0);
            return counts[ch] || 0;
        },

        setChannelModuleCount(ch, value) {
            const counts = this.settings.chModCount
                ? this.settings.chModCount
                      .split(",")
                      .map((s) => parseInt(s.trim()) || 0)
                : [0, 0, 0, 0, 0, 0, 0, 0];
            while (counts.length < 8) counts.push(0);
            counts[ch] = parseInt(value) || 0;
            this.settings.chModCount = counts.join(",");
        },

        getChannelAddress(ch, moduleIdx) {
            if (!this.settings.chModAddrs) return "";
            const channelAddrs = this.settings.chModAddrs.split(";");
            if (!channelAddrs[ch]) return "";
            const addrs = channelAddrs[ch].split(",").map((s) => s.trim());
            return addrs[moduleIdx] || "";
        },

        setChannelAddress(ch, moduleIdx, value) {
            const channelAddrs = this.settings.chModAddrs
                ? this.settings.chModAddrs.split(";")
                : ["", "", "", "", "", "", "", ""];
            while (channelAddrs.length < 8) channelAddrs.push("");

            const addrs = channelAddrs[ch]
                ? channelAddrs[ch].split(",").map((s) => s.trim())
                : [];
            while (addrs.length < 8) addrs.push("");
            addrs[moduleIdx] = value;
            channelAddrs[ch] = addrs
                .filter((a, i) => i < this.getChannelModuleCount(ch) && a)
                .join(",");

            this.settings.chModAddrs = channelAddrs.join(";");
        },

        init() {
            this.loadSettings();
            if (type === "Settings") {
                this.loadTimezones();
            }
            if (type === "Control") {
                this.loadDisplayConfig();
                this.startTestModePolling();
            }
        },

        loadSettings() {
            fetch("/settings")
                .then((res) => res.json())
                .then((data) => {
                    Object.assign(this.settings, data);
                    if (typeof data.mode === "number") {
                        this.settings.mode = data.mode;
                    }
                    if (this.settings.mode === 8) {
                        this.fetchTestModeStatus();
                    }
                })
                .catch(() =>
                    this.showDialog("Failed to load settings", "error", true),
                )
                .finally(() => {
                    this.loading.settings = false;
                });
        },

        loadDisplayConfig() {
            fetch("/api/display-config")
                .then((res) => res.json())
                .then((data) => {
                    this.displayConfig = data.displays || [];
                    // Initialize displayTexts with empty strings for each display
                    this.displayTexts = {};
                    this.displayConfig.forEach((disp) => {
                        this.displayTexts[`dis${disp.index + 1}`] = "";
                    });
                })
                .catch(() =>
                    this.showDialog(
                        "Failed to load display configuration",
                        "error",
                        true,
                    ),
                )
                .finally(() => {
                    this.loading.displayConfig = false;
                });
        },

        loadTimezones() {
            fetch("/timezones.json")
                .then((res) => res.json())
                .then((data) => {
                    this.timezones = data;
                })
                .catch(() =>
                    this.showDialog(
                        "Failed to load timezones. Refresh the page.",
                        "error",
                        true,
                    ),
                )
                .finally(() => (this.loading.timezones = false));
        },

        startTestModePolling() {
            if (this.testModePollIntervalId) {
                clearInterval(this.testModePollIntervalId);
            }
            this.testModePollIntervalId = setInterval(() => {
                if (this.settings.mode === 8) {
                    this.fetchTestModeStatus();
                }
            }, 1000);
        },

        fetchTestModeStatus() {
            fetch("/api/test-mode")
                .then((res) => res.json())
                .then((data) => {
                    if (typeof data.currentChar === "string") {
                        this.testModeCurrentChar = data.currentChar;
                    }
                    if (typeof data.cycleCount === "number") {
                        this.testModeCycleCount = data.cycleCount;
                    }
                })
                .catch(() => {});
        },

        updateDisplay() {
            if (this.settings.mode === 6) {
                if (this.delay < 1) {
                    return this.showDialog(
                        "Delay must be at least 1 second.",
                        "error",
                    );
                }

                if (this.singleMode && this.singleWord.trim() === "") {
                    return this.showDialog(
                        "Single word cannot be empty.",
                        "error",
                    );
                }

                if (!this.singleMode && this.multiWords.length === 0) {
                    return this.showDialog(
                        "Word list cannot be empty.",
                        "error",
                    );
                }
            }

            if (this.settings.mode === 7) {
                // Per-display text mode
                fetch("/api/displays", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        ...this.displayTexts,
                        center: this.centerDisplayText,
                    }),
                })
                    .then((res) => res.json())
                    .then((res) => {
                        if (res.success) {
                            this.showDialog(
                                "Displays updated successfully!",
                                "success",
                            );
                        } else {
                            this.showDialog(
                                res.error || "Failed to update displays",
                                "error",
                            );
                        }
                    })
                    .catch((err) =>
                        this.showDialog("Error: " + err.message, "error"),
                    );
                return;
            }

            if (this.settings.mode === 8) {
                // All Display Test mode
                fetch("/api/test-mode", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        delay: this.testModeDelay,
                        skip: this.testModeSkip,
                    }),
                })
                    .then((res) => res.json())
                    .then((res) => {
                        if (res.success) {
                            this.showDialog("Test mode started!", "success");
                            this.fetchTestModeStatus();
                        } else {
                            this.showDialog(
                                res.error || "Failed to start test mode",
                                "error",
                            );
                        }
                    })
                    .catch((err) =>
                        this.showDialog("Error: " + err.message, "error"),
                    );
                return;
            }

            fetch("/settings", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ mode: this.settings.mode }),
            });

            if (this.settings.mode === 6) {
                fetch("/text", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        mode: this.singleMode ? "single" : "multiple",
                        words: this.singleMode
                            ? [this.singleWord]
                            : this.multiWords,
                        delay: this.delay,
                        center: this.centerText,
                    }),
                })
                    .then((res) => res.json())
                    .then((res) => this.showDialog(res.message, res.type))
                    .catch((err) => this.showDialog(err.message, "error"));
            } else {
                this.showDialog("Mode updated successfully.", "success");
            }
        },

        addWord() {
            if (this.multiWord.trim() !== "") {
                this.multiWords.push(this.multiWord.trim());
            }
            this.multiWord = "";
        },

        removeWord(index) {
            this.multiWords.splice(index, 1);
        },

        save() {
            this.saving = true;
            this.errors = {};

            fetch("/settings", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(this.settings),
            })
                .then((res) => res.json())
                .then((data) => {
                    this.errors = data.errors || {};
                    this.showDialog(data.message, data.type, data.persistent);
                    if (data.redirect) {
                        setTimeout(() => {
                            window.location.href = data.redirect;
                        }, 10000);
                    }
                })
                .catch(() =>
                    this.showDialog("Failed to save settings.", "error"),
                )
                .finally(() => (this.saving = false));
        },

        reset() {
            if (
                confirm("Are you sure you want to reset settings to defaults?")
            ) {
                fetch("/settings/reset", { method: "POST" })
                    .then((res) => res.json())
                    .then((data) => {
                        this.showDialog(
                            data.message,
                            data.type,
                            data.persistent,
                        );
                        this.loadSettings();
                    })
                    .catch(() => {
                        this.showDialog("Failed to reset settings.", "error");
                    });
            }
        },

        showDialog(message, type = "success", persistent = false) {
            this.dialog.message = message;
            this.dialog.type = type;
            this.dialog.show = true;

            if (!persistent) {
                setTimeout(() => (this.dialog.show = false), 3000);
            }
        },
    }));

    Alpine.data("helpModal", () => ({
        visible: false,
        title: "",
        content: "",

        open({ title, content }) {
            this.title = title;
            this.content = content;
            this.visible = true;
        },

        close() {
            this.visible = false;
            this.title = "";
            this.content = "";
        },
    }));
});

Alpine.start();
