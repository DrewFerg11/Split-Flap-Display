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
            d1_enabled: true, // Default to enabled
            d2_enabled: true, // Default to enabled
        },
        errors: {},
        timezones: {},

        // Control page specific - Display 1
        display1Mode: 6, // Default to custom text
        display1Text: "",
        display1SingleMode: true,
        display1CenterText: false,

        // Control page specific - Display 2
        display2Mode: 6, // Default to custom text
        display2Text: "",
        display2SingleMode: true,
        display2CenterText: false,

        // Settings page specific - Advanced settings toggles
        showD1Advanced: false,
        showD2Advanced: false,

        // Legacy control page variables (kept for compatibility)
        selectedDisplay: "both",
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

        // Display 1 helpers
        get d1_addressArray() {
            const val = this.settings.d1_modAddrs;
            if (Array.isArray(val)) return val;
            if (typeof val === "string")
                return val.split(",").map((v) => parseInt(v.trim()));
            return [];
        },
        setD1Address(index, value) {
            const arr = [...this.d1_addressArray];
            arr[index] = parseInt(value);
            this.settings.d1_modAddrs = arr.join(",");
        },

        get d1_offsetArray() {
            const val = this.settings.d1_modOffs;
            if (Array.isArray(val)) return val;
            if (typeof val === "string")
                return val.split(",").map((v) => parseInt(v.trim()));
            return [];
        },
        setD1Offset(index, value) {
            const arr = [...this.d1_offsetArray];
            arr[index] = parseInt(value);
            this.settings.d1_modOffs = arr.join(",");
        },

        // Display 2 helpers
        get d2_addressArray() {
            const val = this.settings.d2_modAddrs;
            if (Array.isArray(val)) return val;
            if (typeof val === "string")
                return val.split(",").map((v) => parseInt(v.trim()));
            return [];
        },
        setD2Address(index, value) {
            const arr = [...this.d2_addressArray];
            arr[index] = parseInt(value);
            this.settings.d2_modAddrs = arr.join(",");
        },

        get d2_offsetArray() {
            const val = this.settings.d2_modOffs;
            if (Array.isArray(val)) return val;
            if (typeof val === "string")
                return val.split(",").map((v) => parseInt(v.trim()));
            return [];
        },
        setD2Offset(index, value) {
            const arr = [...this.d2_offsetArray];
            arr[index] = parseInt(value);
            this.settings.d2_modOffs = arr.join(",");
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
                    // Convert integer booleans to actual booleans for checkboxes
                    // Backend stores booleans as integers (0/1), but Alpine checkboxes need true/false
                    if (data.d1_enabled !== undefined) {
                        data.d1_enabled = Boolean(data.d1_enabled);
                    } else {
                        data.d1_enabled = true; // Default if not present
                    }
                    if (data.d2_enabled !== undefined) {
                        data.d2_enabled = Boolean(data.d2_enabled);
                    } else {
                        data.d2_enabled = true; // Default if not present
                    }
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
                        display: this.selectedDisplay, // Add display selector
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

        // New functions for dual display control
        updateDisplay1() {
            if (this.display1Mode === 6) {
                if (this.display1Text.trim() === "") {
                    return this.showDialog(
                        "Display 1 text cannot be empty.",
                        "error",
                    );
                }

                fetch("/text", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        mode: "single",
                        words: [this.display1Text],
                        display: "1",
                        center: this.display1CenterText,
                        delay: 1,
                    }),
                })
                    .then((res) => res.json())
                    .then((res) =>
                        this.showDialog("Display 1: " + res.message, res.type),
                    )
                    .catch((err) =>
                        this.showDialog(
                            "Display 1 error: " + err.message,
                            "error",
                        ),
                    );
            } else {
                // Update mode for display 1
                fetch("/settings", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        d1_mode: this.display1Mode,
                    }),
                })
                    .then(() =>
                        this.showDialog("Display 1 mode updated.", "success"),
                    )
                    .catch((err) =>
                        this.showDialog(
                            "Display 1 error: " + err.message,
                            "error",
                        ),
                    );
            }
        },

        updateDisplay2() {
            if (this.display2Mode === 6) {
                if (this.display2Text.trim() === "") {
                    return this.showDialog(
                        "Display 2 text cannot be empty.",
                        "error",
                    );
                }

                fetch("/text", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        mode: "single",
                        words: [this.display2Text],
                        display: "2",
                        center: this.display2CenterText,
                        delay: 1,
                    }),
                })
                    .then((res) => res.json())
                    .then((res) =>
                        this.showDialog("Display 2: " + res.message, res.type),
                    )
                    .catch((err) =>
                        this.showDialog(
                            "Display 2 error: " + err.message,
                            "error",
                        ),
                    );
            } else {
                // Update mode for display 2
                fetch("/settings", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        d2_mode: this.display2Mode,
                    }),
                })
                    .then(() =>
                        this.showDialog("Display 2 mode updated.", "success"),
                    )
                    .catch((err) =>
                        this.showDialog(
                            "Display 2 error: " + err.message,
                            "error",
                        ),
                    );
            }
        },

        updateBothDisplays() {
            // Validate both text inputs
            if (this.display1Text.trim() === "") {
                return this.showDialog(
                    "Display 1 text cannot be empty.",
                    "error",
                );
            }
            if (this.display2Text.trim() === "") {
                return this.showDialog(
                    "Display 2 text cannot be empty.",
                    "error",
                );
            }

            // Send both requests in parallel
            const display1Request = fetch("/text", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    mode: "single",
                    words: [this.display1Text],
                    display: "1",
                    center: this.display1CenterText,
                    delay: 1,
                }),
            });

            const display2Request = fetch("/text", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({
                    mode: "single",
                    words: [this.display2Text],
                    display: "2",
                    center: this.display2CenterText,
                    delay: 1,
                }),
            });

            // Wait for both to complete
            Promise.all([display1Request, display2Request])
                .then(() =>
                    this.showDialog(
                        "Both displays updated successfully!",
                        "success",
                    ),
                )
                .catch((err) =>
                    this.showDialog(
                        "Error updating displays: " + err.message,
                        "error",
                    ),
                );
        },

        save() {
            this.saving = true;
            this.errors = {};

            // Convert boolean checkboxes back to integers for backend
            const settingsToSave = { ...this.settings };
            if (settingsToSave.d1_enabled !== undefined) {
                settingsToSave.d1_enabled = settingsToSave.d1_enabled ? 1 : 0;
            }
            if (settingsToSave.d2_enabled !== undefined) {
                settingsToSave.d2_enabled = settingsToSave.d2_enabled ? 1 : 0;
            }

            fetch("/settings", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(settingsToSave),
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

        resetDisplay1() {
            if (
                confirm(
                    "Are you sure you want to reset Display 1 settings to defaults?",
                )
            ) {
                // Reset Display 1 specific settings to defaults
                this.settings.d1_magnetPos = 730;
                this.settings.d1_sdaPin = 21;
                this.settings.d1_sclPin = 22;
                this.settings.d1_dispOffs = 0;
                this.settings.d1_stepsRot = 2048;
                this.settings.d1_maxVel = 15;
                this.settings.d1_modCnt = 8;
                this.settings.d1_modAddrs = "32,33,34,35,36,37,38,39";
                this.settings.d1_modOffs = "0,0,0,0,0,0,0,0";
                this.showDialog("Display 1 reset to defaults", "success");
            }
        },

        resetDisplay2() {
            if (
                confirm(
                    "Are you sure you want to reset Display 2 settings to defaults?",
                )
            ) {
                // Reset Display 2 specific settings to defaults
                this.settings.d2_magnetPos = 730;
                this.settings.d2_sdaPin = 16;
                this.settings.d2_sclPin = 17;
                this.settings.d2_dispOffs = 0;
                this.settings.d2_stepsRot = 2048;
                this.settings.d2_maxVel = 15;
                this.settings.d2_modCnt = 8;
                this.settings.d2_modAddrs = "32,33,34,35,36,37,38,39";
                this.settings.d2_modOffs = "0,0,0,0,0,0,0,0";
                this.showDialog("Display 2 reset to defaults", "success");
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
