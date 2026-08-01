# Firmware

ESP32-based firmware for driving the split-flap modules over I²C. Built on PlatformIO with a web UI for configuration.

## Sections

- [Install (Web Flasher)](install-web-flasher.md) — flash pre-built firmware straight from your browser, no toolchain required.
- [Troubleshooting](install-troubleshooting.md) — fixes for common web-flasher problems.
- [Install (Manual)](install-manual.md) — build from source and flash locally with PlatformIO.

## Supported boards

| Environment            | Processor     | Board                                                        | Dual Display (16 modules) |
| ---------------------- | ------------- | ------------------------------------------------------------ | :---: |
| `esp32_n4` (default)   | ESP32         | ESP32 DevKit (30-pin)                                        | ✅ |
| `esp32c3_n4`           | ESP32-C3FN4   | ESP32-C3 SuperMini / C3-Zero                                 | ❌ |
| `esp32s3_n4r2`         | ESP32-S3FH4R2 | ESP32-S3 SuperMini / S3-Zero[^boot]                          | ❌ |

The dual display configuration requires two independent I²C peripherals to drive both rows simultaneously. Only the ESP32 (`esp32_n4`) is built with this enabled today — the S3 has the same dual-I²C hardware capability but that build hasn't been made yet, and the C3 supports only a single bus.

[^boot]: Requires manually resetting the board into firmware upload mode by holding BOOT, pressing & releasing RESET, then releasing BOOT prior to upload.
