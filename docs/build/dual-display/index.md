# Dual Display — 16 Modules

The dual display configuration drives **up to 16 split-flap modules** from a single ESP32 using two I²C buses. It uses the custom **dowjames v5.1 PCB**, ordered assembled from JLCPCB.

## Overview

- Two separate I²C buses (Bus A + Bus B), each supporting up to 8 modules
- Modules daisy-chain via 4-pin NEXT/PREV headers — no wiring between boards
- Each board has a DIP switch for its I²C address (A0/A1/A2 — 8 unique addresses per bus)
- All SMT components are assembled by JLCPCB — you solder only the I²C pull-up resistors

## Build steps

1. [Bill of materials](bom.md) — everything you need before you start
2. Order the PCBs from JLCPCB — see [`custom-pcb/dowjames-v5.1/pcb-instructions.md`](https://github.com/DrewFerg11/Split-Flap-Display/blob/main/custom-pcb/dowjames-v5.1/pcb-instructions.md)
3. [ESP32 controller board wiring](esp32-wiring.md) — connecting the ESP32, power, and dual I²C buses
4. [Assembly](assembly.md) — mounting into enclosures, hall sensors, motors
