# FAQ

??? question "Something missing or wrong?"
    If your question isn't here or an answer is outdated, [open an issue on GitHub](https://github.com/DrewFerg11/Split-Flap-Display/issues) and it'll get addressed.

---

## Getting Started

??? question "What's the difference between the Original and Dual Display?"
    The [Original display](build/original/index.md) supports up to **8 modules** on a single I²C bus. The [Dual Display](build/dual-display/index.md) uses both of the ESP32's hardware [I²C](i2c.md) peripherals to support up to **16 modules** across two buses. The firmware is backwards compatible — a dual-bus firmware build works fine on an 8-module setup.

    See the [Build & Assembly overview](build/index.md) for a full comparison.

??? question "Do I need the custom PCB, or can I use the DIY boards?"
    Both work. The [custom PCB](module-boards/custom-pcb/index.md) arrives mostly assembled from JLCPCB — you just add [pull-up resistors](i2c.md#pull-up-resistors) and set the DIP switch. The [DIY board](module-boards/diy-board.md) uses discrete breakout modules you solder together yourself. For a full 16-module build, the custom PCB saves a significant amount of assembly time.

??? question "How many modules can I run?"
    - **Minimum:** 1 — the firmware works with any number from 1 to 16; configure the count in the settings page after flashing
    - **Original:** up to **8** on a single I²C bus
    - **Dual Display:** up to **16** total (8 per bus, two buses)
    - **Beyond 16:** possible but requires multiple ESP32s and significant additional work — see the [Expansion](#expansion) section below

??? question "37 flaps or 48 flaps — which should I use?"
    - **37 flaps:** the original character set — A–Z, 0–9, and a single space. Works with both the rounded and square enclosures.
    - **48 flaps:** an extended character set with additional symbols and punctuation. Currently only supported with the original (rounded) enclosure.

    For the [dual display](build/dual-display/index.md), only the **37-flap version** is supported (square enclosure). See the [Build & Assembly comparison table](build/index.md#which-build-is-right-for-you) for details.

??? question "Can I print everything in PLA?"
    Print the **flaps in PLA** (black and white) for crisp character contrast. Print the **structural parts in PETG** — the enclosure, end caps, mounts, and drums. PETG is recommended for structural parts because PLA can soften and warp from motor heat over long-running displays, and PETG is more dimensionally stable for press-fit parts like the heat-set insert mounts. See the [BOM](build/dual-display/bom.md#common) for the recommended filaments.

---

## Hardware & Wiring

??? question "Do I need pull-up resistors on every board?"
    No — only **one set per [I²C](i2c.md) bus**. One 4.7kΩ resistor from SDA to +5V, and one from SCL to +5V, on the **first board in each chain**. Adding pull-ups to every board overloads the bus. See the [I²C reference](i2c.md#pull-up-resistors) for details.

??? question "My display works with one module but breaks when I add a second."
    Almost always missing pull-up resistors. The [PCF8575](i2c.md#how-its-used-in-this-project) has a weak internal pull-up that can barely hold the bus with one device — adding a second loads it past the threshold. Add 4.7kΩ pull-ups on SDA and SCL on the first board in the chain. See the [I²C reference](i2c.md#it-worked-with-one-module-breaks-when-i-add-a-second) for the full explanation.

??? question "My modules stop responding when I add more modules."
    Usually a **power** issue, not I²C. As the daisy chain grows, voltage sag at the far end can drop below what the motors need. Check your power supply capacity, wire gauge, and consider feeding power from the middle of the chain. See the [I²C reference](i2c.md#adding-the-3rd--7th--8th-module-makes-the-display-freeze) for more.

??? question "Two modules share the same address and neither works — how do I fix it?"
    Each module must have a unique DIP switch setting. The [PCF8575](i2c.md#how-its-used-in-this-project) defaults to address `0x20` — if two modules share that setting, both fail. Set a different DIP combination (A0/A1/A2) on every module. See the [I²C address table](i2c.md#address-assignment-via-dip-switch).

??? question "Which power supply should I use?"
    It depends on your build:

    **Original (up to 8 modules):** The community recommends a **5V supply with at least 5A** — the **Raspberry Pi 5 official power supply** (5.1V, 5A USB-C) is widely confirmed to work. A 5V 8A brick has also been used successfully. A Raspberry Pi 4 supply (5V, 3A) is fine for **testing 2–3 modules** but isn't enough for a full 8-module chain — boot inrush plus steady-state current starts exceeding 3A at 5+ modules.

    **Dual Display (16 modules):** The **[MEAN WELL LRS-75-5](build/dual-display/power.md#option-a--integrated-psu-recommended)** (5V, 14A) is recommended. It's hardwired into the [printed PSU enclosure](build/dual-display/build-guide/printed-parts.md) and provides plenty of current headroom. A 5V 10A barrel jack adapter works in some setups but has [known cold-start reliability issues](build/dual-display/power.md#why-are-there-multiple-options) at full 16-module scale.

---

## Firmware

??? question "How do I build and flash the firmware?"
    See the [Firmware setup guide](firmware/setup.md) for step-by-step instructions. You'll need [PlatformIO](https://platformio.org/) and [Node.js](https://nodejs.org/) installed.

??? question "The web interface isn't loading after flashing."
    The firmware uploads in two parts — the firmware binary and the **LittleFS filesystem** (which contains the web interface). If the web interface is blank or missing, you likely only uploaded the firmware. Run the filesystem upload step as well. See the [firmware setup guide](firmware/setup.md).

??? question "How do I update the firmware over Wi-Fi (OTA)?"
    Set an OTA password in the settings page, then add that password as the `auth` flag in `platformio.ini` and use an `*_ota` environment (e.g. `esp32_wroom_ota`) for subsequent uploads.

??? question "Does it support MQTT / smart home integration?"
    Yes — the firmware has built-in MQTT support, including dual-bus awareness. You can publish display updates from Home Assistant, Node-RED, or any MQTT broker. Configure the broker connection in the settings page after flashing.

??? question "What display modes does the firmware support?"
    The firmware supports several modes, switchable from the web interface:

    - **Custom text** — type any message and the display flaps to show it
    - **Time** — clock mode, configurable timezone
    - **Date** — date display
    - **Date + Time** — combined mode
    - **Accuracy Test** — cycles all flaps with configurable timing, useful for calibration and detecting any mis-stepping modules

    For dual displays, custom text supports **per-row control** — each row can show independent content.

??? question "Can I show different content on each row of the dual display?"
    Yes — the dual display firmware supports per-row control in custom text mode. You can have row 1 show one message and row 2 show another, useful for things like "TIME / DATE" or "TEMP / HUMIDITY" splits when paired with MQTT integration.

---

## Troubleshooting

??? question "My motors are getting hot immediately after powering on."
    This is a known issue with the [PCF8575](i2c.md#how-its-used-in-this-project) — it pulls all output pins HIGH by default on power-up, energizing both motor coils continuously. **Flash the firmware before connecting motors.** With firmware running, the ESP32 de-energizes idle coils. See the [custom PCB common gotchas](module-boards/custom-pcb/index.md#common-gotchas).

??? question "My ESP32 won't boot reliably on a cold start."
    Likely an inrush current issue with the power supply. This is a known limitation of lower-quality or lower-rated 5V adapters at full 16-module scale. Switching to the [integrated MEAN WELL PSU (Option A)](build/dual-display/power.md#option-a--integrated-psu-recommended) resolves this. See the [Power page](build/dual-display/power.md#why-are-there-multiple-options) for the full explanation.

??? question "My I²C errors appear under load but go away when motors are idle."
    A noisy or undersized power supply can corrupt I²C signal edges when motors are drawing current. Try a higher-quality dedicated 5V supply for the module chain. See the [I²C reference](i2c.md#ic-error-codes-appear-under-load-but-vanish-on-a-different-power-supply).

---

## Expansion

??? question "Can I build more than 16 modules?"
    Yes, but it gets complicated fast. The 16-module ceiling is a hard limit of the ESP32's two I²C buses combined with the [PCF8575's](i2c.md#how-its-used-in-this-project) 8-address limit per bus. Going further requires one or both of:

    - **[I²C multiplexers (TCA9548A)](i2c.md#ic-multiplexers-tca9548a--addressing-only-not-throughput)** — splits one bus into up to 8 isolated channels, each with 8 addressable modules. This solves the *addressing* problem but not the *throughput* problem. Testing showed 30 modules addressed successfully, but simultaneous updates slowed to a crawl — the I²C bus simply can't push commands to that many motors fast enough. The practical ceiling for full-speed simultaneous updates on a single bus is around **13 modules**.

    - **[Multiple ESP32s](i2c.md#multi-esp32--the-way-to-scale-further)** — the community's working approach for large displays. Each ESP drives up to 20 modules (two buses + a multiplexer per bus), with a coordinator ESP synchronizing them. More capable, but significantly more complex to set up — see the next question for how this has been prototyped.

    See the [I²C scaling reference](i2c.md#scaling-beyond-16-modules) for a full breakdown of what the community has tried.

??? question "Is I²C the right communication protocol for this project?"
    It works well within the 8–16 module range, but it's not the ideal choice for large displays. [I²C](i2c.md) was designed for short on-board runs between chips, not for driving many devices in a daisy chain. Its limitations show up as:

    - **Address ceiling** — only 8 unique addresses per bus without a multiplexer
    - **Throughput** — serial by nature, each module update is sequential; speed degrades with module count
    - **Cable length** — capacitance on long runs slows signal edges and causes errors

    This project chose I²C because per-module addressing is simpler and the hardware is cheap and available — a reasonable trade-off up to about 16 modules.

??? question "Can I synchronize multiple ESP32s to act as one large display?"
    Yes — this has been prototyped and tested successfully. The implemented approach uses **UART** to coordinate a main node with secondary nodes (coded for up to 4 secondaries, tested with 1). Each ESP32 drives its own slice of the display independently, with the main node distributing display updates that each secondary acts on.

    It works, but there are two real challenges:

    **Synchronization** — getting all slices to start and finish flapping at the same time requires careful timing across nodes, but is solvable with the right firmware logic.

    **Power** — this is the bigger issue for large distributed builds. You can't run long 5V runs across a large display without significant voltage drop. The practical approach is a **12V or 24V PSU** running a main power line, with **buck converters** stepping down to 5V at each ESP32/module section. It's 100% doable but requires planning — it's not as simple as having a single 12V-powered PCB would be.

    If you're planning a large distributed display, expect meaningful firmware and power planning work beyond what's documented here.
