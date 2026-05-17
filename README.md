# Split Flap Display Firmware

![format badge](https://github.com/jhoff/Split-Flap-Display/actions/workflows/format-check.yml/badge.svg?branch=main)
[![](https://dcbadge.limes.pink/api/server/https://discord.gg/RCvks4XXXH?style=flat)](https://discord.gg/RCvks4XXXH)

Looking for support? Find help on discord ☝️

Firmware for the modular Split Flap Display created by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display)

- [Instructables](https://www.instructables.com/Split-Flap-Display-3D-Printed-Modular-Compact-Encl/) ( Original )
- [Print Files](https://makerworld.com/en/models/1116618#profileId-1114192) ( Original, 37 characters version )
- [Print Files](https://makerworld.com/en/models/1296793-split-flap-display-extended-charset-48-flaps#profileId-1328346) ( Extended, 48 characters version )
- [Original firmware](https://github.com/ManlyMorgan/Split-Flap-Display)

## Features

- Fully 3D Printed Modular Split Flap Display with 37 or 48 Characters Per Module
- Small Size, 8 Modules are 320mm, 3 Modules are 130mm Wide. 80mm tall (original version) or 94mm (extended version) tall
- Fully configurable and controllable via Web Interface
    - Switch Between Operation Modes, modes include custom input, date mode, and time mode
    - Configure WiFi, Timezone, and hardware settings
- MQTT Support
- OTA Firmware / Filesystem updating

## Supported boards

| Environment          | Processor     | Tested Boards                                                            | Dual Compatible |
| -------------------- | ------------- | ------------------------------------------------------------------------ | --------------- |
| `esp32_c3` (default) | ESP32-C3FN4   | Teyleten Robot ESP32-C3-SuperMini<br>Waveshare ESP32-C3-Zero             | N               |
| `esp32_s3`           | ESP32-S3FH4R2 | Waveshare ESP32-S3-Zero<sup>\*</sup><br>ESP32-S3 Super Mini<sup>\*</sup> | N               |
| `esp32_wroom`        | ESP32         | ESP32-WROOM DevKit V1                                                    | Y               |

<sub>\* Requires manually resetting the board into firmware upload mode by holding BOOT, pressing & releasing RESET, then releasing BOOT prior to upload. After uploading is successful, either press & release RESET or power cycle the board to put it in normal operation mode.</sub>

## Setup Instructions

1. Install dependencies
    - [PlatformIO Core CLI](https://platformio.org/install/cli)
    - [Node Version Mananger](https://github.com/nvm-sh/nvm)
    - [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) (Only required if contributing code)
1. [Download](https://github.com/jhoff/Split-Flap-Display/archive/refs/heads/main.zip) or clone this [git repository](https://github.com/jhoff/Split-Flap-Display).
1. Open a terminal and `cd` to the project root
1. Install required version of npm for the project - `nvm install`
1. Install build dependencies - `npm install`
1. Connect the ESP32 to your computer using a USB cable.
1. Build everything and upload to the board - `npm run build`

- Automatically formats all source code ( `npm run format` - if needed )
- Compiles and minifies all frontend assets ( `npm run assets` )
- Downloads all required arduino / esp32 libraries
- Compiles and uploads the esp32 firmware ( `npm run pio:firmware` or `pio run -t upload -e <environment>` )
- Compiles and uploads the littlefs filesystem ( `npm run pio:filesystem` or `pio run -t uploadfs -e <environment>` )

1. Enjoy!

### Using OTA to update the firmware

On the settings page set an OTA password to enable OTA updatable firmware. Use this same password for your `auth` flag in `platformio.ini`, and then use a device environment with `*_ota` appended (ie `esp32_s3_ota`) to upload a new firmware and/or filesystem

## Contributing

PRs and issues welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for the
issue/PR flow, formatting requirements, and a note on the project's licensing
situation.

## License

- **3D model files** (`STL/`, `3MF/`, `step/`) are licensed under
  CC BY-NC-SA 4.0 — see [LICENSE-HARDWARE.md](LICENSE-HARDWARE.md).
  Original design by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display).
- **Firmware source code** does not yet have an explicit license. This fork
  inherits that from [jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display)
  and its own upstream. The intent is to move to a permissive open-source
  license (likely MIT) once upstream adopts one. Contributions are accepted
  under that understanding — see [CONTRIBUTING.md](CONTRIBUTING.md).
- **Custom PCB design** (`custom-pcb/`) — license TBD.
