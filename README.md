# Split Flap Display

![format badge](https://github.com/DrewFerg11/Split-Flap-Display/actions/workflows/format-check.yml/badge.svg?branch=main)
[![](https://dcbadge.limes.pink/api/server/https://discord.gg/RCvks4XXXH?style=flat)](https://discord.gg/RCvks4XXXH)

**Full documentation:** [drewferg11.github.io/Split-Flap-Display](https://drewferg11.github.io/Split-Flap-Display/)

![Split-flap display in action](docs/assets/split-flap.gif)

A fully 3D-printed, modular split-flap display with an ESP32-based web interface. Forked from [jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display), which builds on the original hardware design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display). This fork adds extended firmware, an improved custom PCB, and support for a 16-module dual I²C configuration.

## Features

- **8 or 16-module display** — single row of 8 modules, or dual row of 16 over an I²C bus(es)
- **Web interface** — configure and control the display from any browser on your network
- **Multiple display modes** — custom text, multi-word cycling, date, time, and random test
- **37 or 48 character support** — standard or extended character set per module
- **WiFi configuration** — set up via access point on first boot, then connects to your network
- **MQTT support** — integrate with Home Assistant or other MQTT brokers
- **OTA updates** — update firmware and filesystem over the air without USB

## Contributing

PRs and issues welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for the issue/PR flow, formatting requirements, and a note on the project's licensing situation.

## License

- **3D model files** (`STL/`, `3MF/`, `step/`) are licensed under CC BY-NC-SA 4.0 — see [LICENSE-HARDWARE.md](LICENSE-HARDWARE.md). Original design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display).
- **Firmware source code** does not yet have an explicit license. This fork inherits that from [jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display) and its own upstream. The intent is to move to a permissive open-source license (likely MIT) once upstream adopts one. Contributions are accepted under that understanding — see [CONTRIBUTING.md](CONTRIBUTING.md).
- **Custom PCB design** (`custom-pcb/`) — license TBD.
