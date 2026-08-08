# Split Flap Display

[![Latest release](https://img.shields.io/github/v/release/DrewFerg11/Split-Flap-Display?include_prereleases&sort=semver&logo=github&label=release)](https://github.com/DrewFerg11/Split-Flap-Display/releases)
[![CI build](https://github.com/DrewFerg11/Split-Flap-Display/actions/workflows/ci-build.yml/badge.svg?branch=main)](https://github.com/DrewFerg11/Split-Flap-Display/actions/workflows/ci-build.yml)
[![Format](https://github.com/DrewFerg11/Split-Flap-Display/actions/workflows/ci-format.yml/badge.svg?branch=main)](https://github.com/DrewFerg11/Split-Flap-Display/actions/workflows/ci-format.yml)
[![GitHub stars](https://img.shields.io/github/stars/DrewFerg11/Split-Flap-Display?style=flat&logo=github)](https://github.com/DrewFerg11/Split-Flap-Display/stargazers)
[![GitHub issues](https://img.shields.io/github/issues/DrewFerg11/Split-Flap-Display?style=flat)](https://github.com/DrewFerg11/Split-Flap-Display/issues)
[![Discord](https://dcbadge.limes.pink/api/server/https://discord.gg/RCvks4XXXH?style=flat)](https://discord.gg/RCvks4XXXH)

![Split-flap display in action](docs/assets/display/split-flap.gif)

**Full documentation:** [drewferg11.github.io/Split-Flap-Display](https://drewferg11.github.io/Split-Flap-Display/)

A fully 3D-printed, modular split-flap display with an ESP32-based web interface. Forked from [jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display), which builds on the original hardware design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display). This fork adds extended firmware, in-depth documentation, a firmware web flasher, and support for a 16-module dual I²C configuration.

## Flash Firmware

You don't need PlatformIO or a build environment to get started. **[Flash the latest firmware straight from your browser](https://drewferg11.github.io/Split-Flap-Display/firmware/install-web-flasher/)** over USB. No code, no compilers, just a data cable and Chrome, Edge, or Opera on desktop. The installer auto-detects your board (ESP32 DevKit, ESP32-C3 SuperMini / C3-Zero, or ESP32-S3 SuperMini / S3-Zero) and walks you through Wi-Fi setup right after flashing.

Prefer to build it yourself? See the [manual install guide](https://drewferg11.github.io/Split-Flap-Display/firmware/install-manual/).

## Features

- **8 or 16-module display:** single row of 8 modules, or dual row of 16 over an I²C bus(es)
- **Web interface:** configure and control the display from any browser on your network
- **Multiple display modes:** custom text, multi-word cycling, date, time, and random test
- **37 or 48 character support:** standard or extended character set per module
- **WiFi configuration:** set up via access point on first boot, then connects to your network
- **MQTT support:** integrate with Home Assistant or other MQTT brokers
- **OTA updates:** update firmware and filesystem over the air without USB
- **Multiple ESP32 boards supported:** ESP32, ESP32-C3, and ESP32-S3 — see [supported hardware](https://drewferg11.github.io/Split-Flap-Display/firmware/hardware/) for specific boards and chip details

## Contributing

PRs and issues welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for the issue/PR flow, formatting requirements, and a note on the project's licensing situation.

## License

- **3D model files** (`STL/`, `3MF/`, `step/`) are licensed under CC BY-NC-SA 4.0. See [LICENSE-HARDWARE.md](LICENSE-HARDWARE.md). Original design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display).
- **Firmware source code** does not yet have an explicit license. This fork inherits that from [jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display) and its own upstream. The intent is to move to a permissive open-source license (likely MIT) once upstream adopts one. Contributions are accepted under that understanding. See [CONTRIBUTING.md](CONTRIBUTING.md).
- **Custom PCB design** (`custom-pcb/`): license TBD.
