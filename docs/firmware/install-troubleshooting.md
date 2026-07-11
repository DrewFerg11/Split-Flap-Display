---
description: Fixes for common problems flashing Split Flap Display firmware from the browser.
---

# Web Flasher Troubleshooting

Having trouble with the [web installer](install-web-flasher.md)? Start here.

## Device not detected

- **No device shows up when you click Install** — make sure you're using a **data** USB cable, not a charge-only one. Try a different USB port or cable.
- **Board isn't detected / install fails immediately** — some boards (notably several ESP32-S3 minis) need to be put into upload mode manually: hold **BOOT**, press and release **RESET**, then release **BOOT**, then try again.
- **Driver issues on Windows** — most boards use a CP2102 or CH340 USB-serial chip. If the port never appears in the browser's device picker, install the [CP210x](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) or [CH340](https://www.wch.cn/downloads/CH341SER_EXE.html) driver for your OS.

## After flashing

- **Flashed, but nothing happens** — reconnect the cable and check that `Split Flap Display` appears in your Wi-Fi list within about 30 seconds of power-up.
- **Wi-Fi setup form never appeared** — see the manual AP-mode fallback on the [install page](install-web-flasher.md#connect-to-wi-fi).
- **Want to watch what the board is doing** — use the **Logs & Console** option described on the [install page](install-web-flasher.md#connect-to-wi-fi) to open a live serial monitor in the browser.

## Still stuck?

Browse the [Releases page](https://github.com/DrewFerg11/Split-Flap-Display/releases) for other versions, or [build from source](install-manual.md) instead.
