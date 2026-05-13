# Original Display — 37 / 48 Characters

The original design supports up to **8 modules** on a single I²C bus. Full build instructions were written by [Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display) and are hosted on Instructables.

## Build instructions

:fontawesome-solid-arrow-up-right-from-square: **[Full build guide on Instructables](https://www.instructables.com/Split-Flap-Display-3D-Printed-Modular-Compact-Encl/)**

The Instructables guide covers the complete build: 3D printing, hardware assembly, wiring, and firmware upload. It is the authoritative reference for the original design.

## Print files

Two versions are available depending on your character set preference:

| Version | Characters | Print files |
|---|---|---|
| Original | 37 characters | [MakerWorld](https://makerworld.com/en/models/1116618#profileId-1114192) |
| Extended | 48 characters | [MakerWorld](https://makerworld.com/en/models/1296793-split-flap-display-extended-charset-48-flaps#profileId-1328346) |

The **48-character extended version** is recommended — it includes lowercase letters and additional symbols.

## Firmware

Both versions use the same firmware in this repository. Follow the [firmware setup guide](../../firmware/setup.md) to build and flash.

## Notes from the community

!!! tip "Magnet polarity"
    The hall effect sensor used in the original design is polarity-sensitive — the sensor markings **must face the magnet**. Installing it backwards is a common mistake. The v5.1 custom PCB build uses a DRV5033 sensor which is polarity-agnostic.

!!! tip "Power supply"
    Use a 5V supply rated for at least 5A for a full 8-module setup. A Raspberry Pi 4 PSU (3A) is often insufficient. The Raspberry Pi 5 PSU (5.1V 5A USB-C) is recommended.
