# Firmware

ESP32-based firmware for driving the split-flap modules over I²C. Built on PlatformIO with a web UI for configuration.

## Sections

- [Install (Web Flasher)](install-web-flasher.md) — flash pre-built firmware straight from your browser, no toolchain required.
- [Troubleshooting](install-troubleshooting.md) — fixes for common web-flasher problems.
- [Install (Manual)](install-manual.md) — build from source and flash locally with PlatformIO.

## Supported boards

| Environment          | Processor     | Tested Boards                                                            | Dual Display (16 modules) |
| -------------------- | ------------- | ------------------------------------------------------------------------ | :---: |
| `esp32_wroom`        | ESP32         | ESP32 DevKit V1 (ESP-WROOM-32)                                           | ✅ |
| `esp32_c3` (default) | ESP32-C3FN4   | Teyleten Robot ESP32-C3-SuperMini, Waveshare ESP32-C3-Zero               | ❌ |
| `esp32_s3`           | ESP32-S3FH4R2 | Waveshare ESP32-S3-Zero[^boot], ESP32-S3 Super Mini[^boot]               | ❌ |

The dual display configuration requires two independent I²C peripherals to drive both rows simultaneously. Only the ESP32-WROOM has this capability — the C3 and S3 variants support a single bus and are limited to 8 modules.

[^boot]: Requires manually resetting the board into firmware upload mode by holding BOOT, pressing & releasing RESET, then releasing BOOT prior to upload.
