# Build & Flash

## Prerequisites

Install the following before getting started:

- [PlatformIO Core CLI](https://platformio.org/install/cli)
- [Node Version Manager (nvm)](https://github.com/nvm-sh/nvm)
- [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) — only needed if you plan to contribute code

## Setup

1. [Download](https://github.com/DrewFerg11/Split-Flap-Display/archive/refs/heads/main.zip) or clone the repository
2. Open a terminal and `cd` to the project root
3. Install the required Node version: `nvm install`
4. Install build dependencies: `npm install`
5. Connect the ESP32 via USB
6. Build and upload everything: `npm run build`

`npm run build` does the following in order:

- Compiles and minifies all frontend assets
- Downloads all required Arduino/ESP32 libraries
- Compiles and uploads the ESP32 firmware
- Compiles and uploads the LittleFS filesystem (web interface files)

!!! warning "Both uploads are required"
    The firmware and filesystem are two separate uploads. `npm run build` handles both. If you only flash the firmware, the web interface won't load.

## Environment Selection

By default, `npm run build` targets the `esp32_wroom` environment (required for dual display). To target a different board, pass the environment explicitly:

```bash
pio run -t upload -e esp32_c3
pio run -t uploadfs -e esp32_c3
```

See the [supported boards](index.md#supported-boards) table for available environments.

## Individual Commands

| Command | Description |
|---|---|
| `npm run build` | Full build and upload (firmware + filesystem) |
| `npm run pio:firmware` | Upload firmware only |
| `npm run pio:filesystem` | Upload filesystem only |
| `npm run assets` | Build frontend assets only |
| `npm run format` | Auto-format source code (contributors) |

## OTA Updates

Once the display is assembled and the enclosure is closed, you can update over the air without USB.

1. On the settings page, set an OTA password
2. Add the same password as the `auth` flag in `platformio.ini`
3. Upload using an OTA environment:

```bash
pio run -t upload -e esp32_wroom_ota
pio run -t uploadfs -e esp32_wroom_ota
```

Append `_ota` to any environment name to use OTA for that board.
