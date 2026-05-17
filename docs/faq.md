# FAQ

??? question "Something missing or wrong?"
    If your question isn't here or an answer is outdated, [open an issue on GitHub](https://github.com/DrewFerg11/Split-Flap-Display/issues) and it'll get addressed.

---

## Getting Started

??? question "What's the difference between the Original and Dual Display?"
    The Original display supports up to **8 modules** on a single I²C bus. The Dual Display uses both of the ESP32's hardware I²C peripherals to support up to **16 modules** across two buses. The firmware is backwards compatible — a dual-bus firmware build works fine on an 8-module setup.

    See the [Build & Assembly overview](build/index.md) for a full comparison.

??? question "Do I need the custom PCB, or can I use the DIY boards?"
    Both work. The [custom PCB](module-boards/custom-pcb/index.md) (dowjames v5.1) arrives mostly assembled from JLCPCB — you just add pull-up resistors and set the DIP switch. The [DIY board](module-boards/diy-board.md) uses discrete breakout modules you solder together yourself. For a full 16-module build, the custom PCB saves a significant amount of assembly time.

??? question "How many modules can I run?"
    With a single ESP32 and two I²C buses: **16 modules maximum**. The PCF8575 expander supports 8 unique addresses per bus, and the ESP32 has two hardware I²C peripherals. Going beyond 16 requires multiple ESP32s. See the [I²C reference](i2c.md#scaling-beyond-16-modules) for details on scaling.

??? question "Can I run fewer than 16 modules?"
    Yes — the firmware works with any number from 1 to 16. Just configure the module count in the settings page after flashing.

---

## Hardware & Wiring

??? question "Do I need pull-up resistors on every board?"
    No — only **one set per I²C bus**. One 4.7kΩ resistor from SDA to +5V, and one from SCL to +5V, on the **first board in each chain**. Adding pull-ups to every board over-loads the bus. See the [I²C reference](i2c.md#pull-up-resistors) for details.

??? question "My display works with one module but breaks when I add a second."
    Almost always missing pull-up resistors. The PCF8575 has a weak internal pull-up that can barely hold the bus with one device — adding a second loads it past the threshold. Add 4.7kΩ pull-ups on SDA and SCL on the first board in the chain. See the [I²C reference](i2c.md#it-worked-with-one-module-breaks-when-i-add-a-second) for the full explanation.

??? question "My modules stop responding when I add the 3rd, 7th, or 8th module."
    Usually a **power** issue, not I²C. As the daisy chain grows, voltage sag at the far end can drop below what the motors need. Check your power supply capacity, wire gauge, and consider feeding power from the middle of the chain. See the [I²C reference](i2c.md#adding-the-3rd--7th--8th-module-makes-the-display-freeze) for more.

??? question "Two modules share the same address and neither works — how do I fix it?"
    Each module must have a unique DIP switch setting. The PCF8575 defaults to address `0x20` — if two modules share that setting, both fail. Set a different DIP combination (A0/A1/A2) on every module. See the [I²C address table](i2c.md#address-assignment-via-dip-switch).

??? question "Which power supply should I use?"
    For a full 16-module dual display, the **MEAN WELL LRS-75-5** (5V, 14A) is recommended. It's hardwired into the printed PSU enclosure and provides plenty of current headroom. A 5V 10A barrel jack adapter works for some setups but has known cold-start reliability issues at full scale. See the [Power page](build/dual-display/power.md) for details.

---

## Firmware

??? question "How do I build and flash the firmware?"
    See the [Firmware setup guide](firmware/setup.md) for step-by-step instructions. You'll need PlatformIO and Node.js installed.

??? question "The web interface isn't loading after flashing."
    The firmware uploads in two parts — the firmware binary and the LittleFS filesystem (which contains the web interface). If the web interface is blank or missing, you likely only uploaded the firmware. Run the filesystem upload step as well. See the [firmware setup guide](firmware/setup.md).

??? question "How do I update the firmware over Wi-Fi (OTA)?"
    Set an OTA password in the settings page, then add that password as the `auth` flag in `platformio.ini` and use an `*_ota` environment (e.g. `esp32_wroom_ota`) for subsequent uploads.

---

## Troubleshooting

??? question "My motors are getting hot immediately after powering on."
    This is a known issue with the PCF8575 — it pulls all output pins HIGH by default on power-up, energizing both motor coils continuously. **Flash the firmware before connecting motors.** With firmware running, the ESP32 de-energizes idle coils. See the [custom PCB common gotchas](module-boards/custom-pcb/index.md#common-gotchas).

??? question "My ESP32 won't boot reliably on a cold start."
    Likely an inrush current issue with the power supply. This is a known limitation of lower-quality or lower-rated 5V adapters at full 16-module scale. Switching to the integrated Mean Well PSU (Option A) resolves this. See the [Power page](build/dual-display/power.md#why-are-there-multiple-options) for the full explanation.

??? question "My I²C errors appear under load but go away when motors are idle."
    A noisy or undersized power supply can corrupt I²C signal edges when motors are drawing current. Try a higher-quality dedicated 5V supply for the module chain. See the [I²C reference](i2c.md#ic-error-codes-appear-under-load-but-vanish-on-a-different-power-supply).
