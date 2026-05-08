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
        },
        saving: false,
        dialog: {
            show: false,
            message: "",
            type: null,
        },
        settings: {
            mode: 2,
            dateFormat: "ddd dd/MM",
            timeFormat: "HH:mm",
        },
        errors: {},
        timezones: {},

        // Control page specific
        singleMode: true,
        singleWord: "",
        multiWord: "",
        multiWords: [],
        delay: 1,
        centerText: false,

        get processing() {
            return (
                this.saving || this.loading.settings || this.loading.timezones
            );
        },

        get isDualI2C() {
            return this.settings.wire1Addresses !== undefined;
        },

        // Bus 1 (Wire)
        get wireAddressArray() {
            return (
                this.settings.wireAddresses?.split(",").map((s) => s.trim()) ||
                []
            );
        },
        setWireAddress(index, value) {
            const arr = this.wireAddressArray;
            arr[index] = value;
            this.settings.wireAddresses = arr.join(",");
        },
        get wireOffsetArray() {
            return (
                this.settings.wireOffsets?.split(",").map((s) => s.trim()) || []
            );
        },
        setWireOffset(index, value) {
            const arr = this.wireOffsetArray;
            arr[index] = value;
            this.settings.wireOffsets = arr.join(",");
        },
        get wireCount() {
            return this.wireAddressArray.length;
        },
        addWireModule() {
            const addr = (32 + this.wireCount).toString();
            this.settings.wireAddresses = [...this.wireAddressArray, addr].join(
                ",",
            );
            this.settings.wireOffsets = [...this.wireOffsetArray, "0"].join(
                ",",
            );
        },
        removeWireModule() {
            if (this.wireCount > 1) {
                this.settings.wireAddresses = this.wireAddressArray
                    .slice(0, -1)
                    .join(",");
                this.settings.wireOffsets = this.wireOffsetArray
                    .slice(0, -1)
                    .join(",");
            }
        },

        // Bus 2 (Wire1)
        get wire1AddressArray() {
            return (
                this.settings.wire1Addresses?.split(",").map((s) => s.trim()) ||
                []
            );
        },
        setWire1Address(index, value) {
            const arr = this.wire1AddressArray;
            arr[index] = value;
            this.settings.wire1Addresses = arr.join(",");
        },
        get wire1OffsetArray() {
            return (
                this.settings.wire1Offsets?.split(",").map((s) => s.trim()) ||
                []
            );
        },
        setWire1Offset(index, value) {
            const arr = this.wire1OffsetArray;
            arr[index] = value;
            this.settings.wire1Offsets = arr.join(",");
        },
        get wire1Count() {
            return this.wire1AddressArray.length;
        },
        addWire1Module() {
            const addr = (32 + this.wire1Count).toString();
            this.settings.wire1Addresses = [
                ...this.wire1AddressArray,
                addr,
            ].join(",");
            this.settings.wire1Offsets = [...this.wire1OffsetArray, "0"].join(
                ",",
            );
        },
        removeWire1Module() {
            if (this.wire1Count > 1) {
                this.settings.wire1Addresses = this.wire1AddressArray
                    .slice(0, -1)
                    .join(",");
                this.settings.wire1Offsets = this.wire1OffsetArray
                    .slice(0, -1)
                    .join(",");
            }
        },

        init() {
            this.loadSettings();
            if (type === "Settings") {
                this.loadTimezones();
            }
        },

        loadSettings() {
            fetch("/settings")
                .then((res) => res.json())
                .then((data) => {
                    Object.assign(this.settings, data);
                })
                .catch(() =>
                    this.showDialog("Failed to load settings", "error", true),
                )
                .finally(() => {
                    this.loading.settings = false;
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
