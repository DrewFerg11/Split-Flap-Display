# Split Flap Display

A fully 3D-printed, modular split-flap display with an ESP32-based web interface. This project builds on the original hardware design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display) with extended firmware, an improved custom PCB, and support for a **16-module dual I²C configuration**.

<div class="grid cards" markdown>

- :material-printer-3d: **Build & Assembly**

    ---

    Step-by-step guides for the original single display or the 16-module dual display setup.

    [:octicons-arrow-right-24: Start building](build/index.md)

- :material-code-tags: **Firmware**

    ---

    Build, flash, and configure the ESP32-based firmware.

    [:octicons-arrow-right-24: Firmware docs](firmware/index.md)

- :fontawesome-brands-discord: **Community**

    ---

    Help, build threads, and shared lessons.

    [:octicons-arrow-right-24: Join the Discord](https://discord.gg/RCvks4XXXH)

- :fontawesome-brands-github: **Source**

    ---

    Firmware, PCB files, and print files on GitHub.

    [:octicons-arrow-right-24: View repository](https://github.com/DrewFerg11/Split-Flap-Display)

</div>

---

## Features

- Fully 3D-printed modular enclosure — 37 or 48 characters per module
- Small footprint: 8 modules span 320mm, 80mm (original) or 94mm (extended) tall
- Web interface for configuration and control
    - Modes: custom text, date, time
    - Configure WiFi, timezone, and hardware settings
- MQTT support
- OTA firmware and filesystem updates
- **Dual I²C bus support** for up to 16 modules from a single ESP32

## Supported ESP32 boards

| Environment          | Processor     | Tested Boards                                                             |
|----------------------|---------------|---------------------------------------------------------------------------|
| `esp32_c3` (default) | ESP32-C3FN4   | Teyleten Robot ESP32-C3-SuperMini, Waveshare ESP32-C3-Zero                |
| `esp32_s3`           | ESP32-S3FH4R2 | Waveshare ESP32-S3-Zero[^boot], ESP32-S3 Super Mini[^boot]                |
| `esp32_wroom`        | ESP32         | ESP32-WROOM DevKit V1                                                     |

[^boot]: Requires manually entering upload mode: hold BOOT, press+release RESET, release BOOT. After uploading, press RESET or power-cycle.

## Links

| Resource | Link |
|---|---|
| Original hardware & Instructables | [instructables.com](https://www.instructables.com/Split-Flap-Display-3D-Printed-Modular-Compact-Encl/) |
| Print files — original 37-char | [MakerWorld](https://makerworld.com/en/models/1116618#profileId-1114192) |
| Print files — extended 48-char | [MakerWorld](https://makerworld.com/en/models/1296793-split-flap-display-extended-charset-48-flaps#profileId-1328346) |
| Original firmware (upstream) | [github.com/jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display) |
| Custom PCB (dowjames v5.1) | [custom-pcb/dowjames-v5.1/](https://github.com/DrewFerg11/Split-Flap-Display/tree/main/custom-pcb/dowjames-v5.1) |
