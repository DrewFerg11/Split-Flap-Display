# dowjames v5.1

This is split-flap module PCB designed by dowjames. It integrates the PCF8575 I/O expander, ULN2003 motor driver, hall sensor connector, and I²C daisy-chain headers onto a single compact board — ordered fully assembled from JLCPCB.

<div style="display: flex; gap: 1.5rem; margin: 2rem 0;" markdown>
<figure style="flex: 1; text-align: center;" markdown>
![PCB v5.1 front](../../../assets/pcb/pcb-v5.1-front.png)
<figcaption>Front</figcaption>
</figure>
<figure style="flex: 1; text-align: center;" markdown>
![PCB v5.1 back](../../../assets/pcb/pcb-v5.1-back.png)
<figcaption>Back</figcaption>
</figure>
</div>

[Order from JLCPCB](ordering.md){ .md-button .md-button--primary }

---

## Specs

| | |
|---|---|
| **I²C expander** | PCF8575 |
| **Motor driver** | ULN2003 |
| **Address config** | 3-pin DIP switch (A0/A1/A2) — 8 unique addresses |
| **Daisy-chain** | 4-pin NEXT/PREV right-angle headers (+5V, GND, SCL, SDA) |
| **Assembly** | Top-side SMT, assembled by JLCPCB |
| **Minimum order** | 10 boards |

## What arrives assembled

All SMT components are pre-soldered by JLCPCB:

- PCF8575 I²C I/O expander
- ULN2003 motor driver
- Bypass capacitors
- DIP switch
- Hall sensor connector
- NEXT/PREV daisy-chain headers

## What you still need to do

!!! info "Common gotchas for all custom PCBs"
    Before proceeding, read the [common gotchas](../index.md#common-gotchas) on the Custom PCBs page — firmware flashing order, pull-up resistors, and power wiring apply to this board.

### 1. Add I²C Pull-Up Resistors

Add **4.7kΩ pull-up resistors** on the **first board in each chain only** — one resistor from SDA to +5V, one from SCL to +5V.

![Pull-up resistors soldered to the back of a PCB](../../../assets/pcb/pullup-resistors-back-1.jpg)

### 2. Set DIP Switch Addresses

Each board needs a unique I²C address set via the onboard 3-pin DIP switch. See the [I²C reference](../../../i2c.md#address-assignment-via-dip-switch) for the address table. Every module in a chain must have a different setting.
