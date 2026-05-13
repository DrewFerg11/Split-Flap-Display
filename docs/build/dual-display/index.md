# Dual Display — 16 Modules

The dual display configuration drives **up to 16 split-flap modules** from a single ESP32 using two I²C buses. It uses the custom **dowjames v5.1 PCB**, ordered assembled from JLCPCB.

## Overview

- Two separate I²C buses (Bus A + Bus B), each supporting up to 8 modules
- Modules daisy-chain via 4-pin NEXT/PREV headers — no wiring between boards
- Each board has a DIP switch for its I²C address (A0/A1/A2 — 8 unique addresses per bus)
- All SMT components are assembled by JLCPCB — you solder only the I²C pull-up resistors

## Build steps

1. [Order the PCBs](pcb-ordering.md) from JLCPCB
2. [Wiring guide](wiring.md) — connecting modules, power, and ESP32
3. [Assembly](assembly.md) — mounting into enclosures, hall sensors, motors
4. [Firmware configuration](firmware-config.md) — dual-bus settings, addressing, modes

## What you'll need

!!! warning "Coming soon"
    Full bill of materials for the dual display setup will be added here.
