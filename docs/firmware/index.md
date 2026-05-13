# Firmware

ESP32-based firmware for driving the split-flap modules over I²C. Built on PlatformIO with a web UI for configuration.

## Sections

- [Build & Flash](setup.md) — installing dependencies, building, and uploading to the ESP32.

## Supported boards

| Environment          | Processor     | Tested Boards                                                            |
| -------------------- | ------------- | ------------------------------------------------------------------------ |
| `esp32_c3` (default) | ESP32-C3FN4   | Teyleten Robot ESP32-C3-SuperMini, Waveshare ESP32-C3-Zero               |
| `esp32_s3`           | ESP32-S3FH4R2 | Waveshare ESP32-S3-Zero[^boot], ESP32-S3 Super Mini[^boot]               |

[^boot]: Requires manually resetting the board into firmware upload mode by holding BOOT, pressing & releasing RESET, then releasing BOOT prior to upload.
