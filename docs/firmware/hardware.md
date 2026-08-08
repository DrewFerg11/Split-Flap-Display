# Supported Hardware

Which Espressif chips this project targets, and the specific boards it has actually been built and tested on. For the firmware environment each board maps to, see the [Supported boards](index.md#supported-boards) table.

## What makes a chip a candidate for this project

Any future board is judged against one bar:

1. **Two cores** — the dual-bus movement engine pins one FreeRTOS task per bus to its own core. Beyond that, a dual-core part is measurably more stable and responsive here _even driving a single row_.
2. **Two hardware I²C controllers** — two independent buses is what a 16-module dual display requires.
3. **Wi-Fi** — the entire UI, MQTT, and OTA path runs over it.

Everything else (clock speed, Bluetooth flavour, 802.15.4, PSRAM, flash size, Wi-Fi 6, Ethernet) is irrelevant to this project and appears in the table below only as context.

## Espressif SoC lineup

Verified against [espressif.com/en/products/socs](https://www.espressif.com/en/products/socs) as of 2026-07-31. Sorted so the table answers "what could I use?" before "what exists?" — passing parts first, then near-misses, then disqualified.

| Series | SoC           | Architecture         | Cores | Max clock | Wireless                                          |   Native USB   | Meets the bar?                                      |
| ------ | ------------- | -------------------- | :---: | --------- | ------------------------------------------------- | :------------: | --------------------------------------------------- |
| ESP32  | **ESP32**     | Xtensa LX6           | **2** | 240 MHz   | Wi-Fi 4, BT 4.2 Classic + BLE                     |       ❌       | ✅ **shipping** — the dual-row board                |
| S      | **ESP32-S3**  | Xtensa LX7           | **2** | 240 MHz   | Wi-Fi 4, BLE 5                                    |     ✅ OTG     | ✅ **shipping** — meets it; dual-row unbuilt so far |
| S      | **ESP32-S31** | RISC-V               | **2** | 320 MHz   | Wi-Fi 6, BT 5.4, 802.15.4, Gigabit Ethernet       |       ✅       | ⚠️ **only future candidate** — I²C count unverified |
| S      | **ESP32-S2**  | Xtensa LX7           |   1   | 240 MHz   | Wi-Fi 4 only                                      |     ✅ OTG     | ❌ single core                                      |
| C      | **ESP32-C3**  | RISC-V               |   1   | 160 MHz   | Wi-Fi 4, BLE 5                                    | ✅ Serial/JTAG | ⚠️ **shipping** — 1 core, 1× I²C; single row only   |
| C      | **ESP32-C2**  | RISC-V               |   1   | 120 MHz   | Wi-Fi 4, BLE 5                                    |       ❌       | ❌ single core                                      |
| C      | **ESP32-C5**  | RISC-V (+LP core)    | 1 HP  | 240 MHz   | Dual-band Wi-Fi 6, BLE 5, 802.15.4                |       ✅       | ❌ single HP core                                   |
| C      | **ESP32-C6**  | RISC-V (+LP core)    | 1 HP  | 160 MHz   | Wi-Fi 6, BLE 5.3, 802.15.4 (Thread/Zigbee/Matter) |       ✅       | ❌ single HP core                                   |
| C      | **ESP32-C61** | RISC-V               |   1   | 160 MHz   | Wi-Fi 6, BLE 5                                    |       ✅       | ❌ single core                                      |
| H      | **ESP32-H4**  | RISC-V               |   2   | 96 MHz    | BLE 5.4, 802.15.4 — **no Wi-Fi**                  |       ✅       | ❌ no Wi-Fi                                         |
| H      | **ESP32-H2**  | RISC-V               |   1   | 96 MHz    | BLE 5, 802.15.4 — **no Wi-Fi**                    |       ✅       | ❌ no Wi-Fi                                         |
| H      | **ESP32-H21** | RISC-V               |   1   | 96 MHz    | BLE, 802.15.4 — **no Wi-Fi**                      |       ✅       | ❌ no Wi-Fi                                         |
| P      | **ESP32-P4**  | RISC-V (2 HP + 1 LP) |   2   | 400 MHz   | **none integrated**                               |     ✅ OTG     | ❌ no radio                                         |
| E      | **ESP32-E22** | RISC-V               |   2   | 500 MHz   | Tri-band Wi-Fi 6E, BT 5.4                         |       —        | ❌ co-processor — needs a host MCU                  |
| legacy | **ESP8266**   | Xtensa L106          |   1   | 160 MHz   | Wi-Fi 4 only                                      |       ❌       | ❌ legacy; Espressif points to C2                   |

The whole C-series is structurally out — every C part is single-HP-core, and that's a design choice of the line, not a temporary gap. In practice this project is an ESP32/S3 project, with the C3 supported as a legacy single-row option and **ESP32-S31 the only plausible future addition** in the entire current lineup.

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
