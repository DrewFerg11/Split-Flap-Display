# Supported Hardware

Which Espressif chips this project targets, and the specific boards it has actually been built and tested on. For the firmware environment each board maps to, see the [Supported boards](index.md#supported-boards) table.

## Naming convention

Firmware environments — the `esp32_n4`, `esp32c3_n4`, `esp32s3_n4r2` names used throughout this page — follow one pattern:

```
esp32[<chip-suffix>]_n<flashMB>[r<psramMB>][_ota]
```

- **Chip suffix** — the Espressif family token, lowercase, no separator (`c3`, `s3`, ...). The original ESP32 has no suffix.
- **`n<MB>`** — flash size, always present. This selects the partition table (`n4` → `partitions_4MB.csv`).
- **`r<MB>`** — PSRAM size, only present when PSRAM is enabled. This selects the board definition / bootloader memory mode (quad vs. octal), not the partition table.
- **`_ota`** — the OTA-upload variant of the same board; everything else about the build is identical.

**Why chip + memory instead of a brand name?** Carrier-board brands are the least stable identifier in the stack — the same silicon gets resold under different names by different sellers (see [Boards this project has been built on](#boards-this-project-has-been-built-on) below). Packaging names don't even uniquely identify a chip: `ESP32-WROOM-32` and `ESP32-S3-WROOM-1` are both "WROOM" parts, and the S3-WROOM-1 module alone ships in several flash/PSRAM configurations that each need their own partition table or bootloader. Naming envs after what actually determines the build — chip family, flash, PSRAM — means a new board with a different outline or brand but the same chip and memory is just a new row in the boards table below, not a new env.

## What makes a chip a candidate for this project

Any future board is judged against one bar:

1. **Two cores** — the dual-bus movement engine pins one FreeRTOS task per bus to its own core. Beyond that, a dual-core part is measurably more stable and responsive here _even driving a single row_.
2. **Two hardware I²C controllers** — two independent buses is what a 16-module dual display requires.
3. **Wi-Fi** — the entire UI, MQTT, and OTA path runs over it.

Everything else (clock speed, Bluetooth flavour, 802.15.4, PSRAM, flash size, Wi-Fi 6, Ethernet) is irrelevant to this project and appears in the table below only as context.

## Chip families this project supports

Verified against [espressif.com/en/products/socs](https://www.espressif.com/en/products/socs) as of 2026-07-31.

| SoC           | Architecture | Cores | Max clock | Wireless                      |   Native USB   | I²C buses | Original support | Dual support |
| ------------- | ------------ | :---: | --------- | ------------------------------ | :------------: | :-------: | :---------------: | :-----------: |
| **ESP32**     | Xtensa LX6   | **2** | 240 MHz   | Wi-Fi 4, BT 4.2 Classic + BLE | ❌              | 2         | ✅                 | ✅            |
| **ESP32-C3**  | RISC-V       | 1     | 160 MHz   | Wi-Fi 4, BLE 5                | ✅ Serial/JTAG | 1         | ✅                 | ❌            |
| **ESP32-S3**  | Xtensa LX7   | **2** | 240 MHz   | Wi-Fi 4, BLE 5                | ✅ OTG         | 2         | ✅                 | ⚠️[^s3-dual]  |

[^s3-dual]: The S3 has two I²C controllers, same as the ESP32, so dual mode is possible in principle — but it isn't planned for the **ESP32-S3 SuperMini / S3-Zero** (`esp32s3_n4r2`): in testing that board has been less reliable and slower than the ESP32, so it's not a good candidate for the dual-bus build regardless of what the silicon can do. The **ESP32-S3-WROOM-1 44-pin board** (`esp32s3_n16r8`) is a different, more capable carrier design and will get dual-bus support once it's added to the project.

## Boards this project has been built on

Ordered by firmware environment so the many-boards-to-one-build relationship is visible at a glance, and carrying the sled tag so the cradle, the docs, and the web flasher all say the same word for a given board.

| Board (as sold)                             | Sled tag       | Silicon                | Flash / PSRAM      | Pins | USB       | Env             | Source                                              |
| -------------------------------------------- | -------------- | ----------------------- | ------------------- | :--: | --------- | --------------- | ---------------------------------------------------- |
| AITRIP ESP32 DevKit, Type-C, CP2102         | `ESP32 30-PIN` | ESP32-WROOM-32          | 4 MB / —            |  30  | USB-C     | `esp32_n4`      | [B0CR5Y2JVD](https://www.amazon.com/dp/B0CR5Y2JVD) |
| Flutesan ESP-WROOM-32 DevKit                | `ESP32 30-PIN` | ESP32-WROOM-32          | 4 MB / —            |  30  | Micro-USB | `esp32_n4`      | [B09GK74F7N](https://www.amazon.com/dp/B09GK74F7N) |
| ESP32-C3 SuperMini (Teyleten et al.)        | `C3 SUPERMINI` | ESP32-C3FN4             | 4 MB / —            |  16  | USB-C     | `esp32c3_n4`    | [B0D5XYBVKY](https://www.amazon.com/dp/B0D5XYBVKY) |
| Waveshare "ESP32-S3 Mini" (= ESP32-S3-Zero) | `S3 ZERO`      | ESP32-S3FH4R2           | 4 MB / 2 MB quad    |  27  | USB-C     | `esp32s3_n4r2`  | [B0CHYHGYRH](https://www.amazon.com/dp/B0CHYHGYRH) |
| SANXIXING ESP32-S3 44-pin (DevKitC-1 clone) | `S3 44-PIN`    | ESP32-S3-WROOM-1 N16R8  | 16 MB / 8 MB octal  |  44  | 2× USB-C  | not yet supported | [B0DB1WK3CW](https://www.amazon.com/dp/B0DB1WK3CW) |

Two boards share `esp32_n4` despite different USB connectors — this is the model working, not a gap. A reader adding a board appends a row and picks the env whose chip + flash + PSRAM matches theirs.

??? note "Chip vs. module vs. board — three layers, only one of them stable"

    | Layer              | Example                                                                                          | Named by                       | Varies by                                        |
    | ------------------ | ------------------------------------------------------------------------------------------------ | ------------------------------- | ------------------------------------------------- |
    | Silicon            | `ESP32-S3`                                                                                       | Espressif                       | fixed                                             |
    | Chip / module part | `ESP32-S3FH4R2` (bare, memory in-package) vs `ESP32-S3-WROOM-1-N16R8` (metal can, external dies) | Espressif                        | flash + PSRAM size and mode                       |
    | Carrier board      | "S3-Zero", "S3 SuperMini", "DevKitC-1 clone"                                                     | the reseller — **unregulated**  | outline, pin count, USB port, antenna, bridge IC  |

    This is why env names use chip + memory and never a brand: the brand layer is the one nobody controls. Same board, three sellers, three names.

??? note "Three memories, and why only two of them are in the env name"

    A spec sheet says "520 KB SRAM" while the board listing says "4 MB", and both are right about different things:

    |                    | **Internal SRAM**                                            | **Flash** (`N`)                                                          | **PSRAM** (`R`)               |
    | ------------------ | ------------------------------------------------------------- | -------------------------------------------------------------------------- | ------------------------------ |
    | Where              | on the die                                                     | separate chip / in-package                                                 | separate chip / in-package     |
    | Analogy            | the RAM you run in                                             | the disk                                                                    | optional extra RAM             |
    | Survives power-off | ❌                                                              | ✅                                                                          | ❌                              |
    | Holds              | variables, stack, heap, Wi-Fi buffers                          | bootloader, partition table, firmware, web UI (littlefs), settings (NVS)    | nothing permanent               |
    | Typical size       | ESP32 520 KB · C3 400 KB · S3 512 KB                           | 4 MB or 16 MB                                                               | 0, 2 MB quad, 8 MB octal        |
    | Varies per board?  | **no — fixed by the chip**                                     | ✅ yes                                                                      | ✅ yes                          |
    | In the env name?   | **no** — implied by the chip token                             | ✅ `n4` / `n16`                                                             | ✅ `r2` / `r8`                  |
    | This project       | what actually constrains runtime; check `ESP.getFreeHeap()`    | ~1.17 MB firmware in a 1.5 MB app slot                                      | unused — zero bytes allocated   |

    Only the memories that _vary between boards of the same chip_ appear in an env name. SRAM is silicon, so `esp32s3` already implies 512 KB and repeating it would be noise.

    Two follow-on points worth stating plainly:

    - **The 520 KB is not all yours.** Part is instruction RAM, and Wi-Fi plus the async web server claim a substantial share. The free-heap figure at runtime is the honest number, not the datasheet headline.
    - **PSRAM being unused is not a bug.** It is bonus memory for framebuffers, camera images, and audio buffers — none of which this firmware has. It appears in env names only because **quad (`R2`) and octal (`R8`) parts need different bootloader images**, so they are not interchangeable builds even though neither one's memory is touched.
