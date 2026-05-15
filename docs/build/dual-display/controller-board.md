# Controller Board Assembly

The dual display configuration requires a custom ESP32 controller board that manages two independent I²C buses (Bus A and Bus B) and coordinates power distribution to support up to 16 split-flap modules. This page covers assembling the perfboard-based design.

![ESP32 controller board](../../assets/esp32-controller-board-1.jpg){ width="500" .center }

*The assembled ESP32 controller board*

## Overview

The controller board is built on perfboard using:

- **ESP32 microcontroller** — runs the firmware and manages both I²C buses
- **Perfboard with screw terminals** — provides clean connections for power, ground, and I²C signals
- **Pull-up resistors** — one set per I²C bus (typically 4.7kΩ)
- **Wiring and connectors** — interface with the 5V power supply and split-flap PCB daisy chains

## Components

Before assembling, gather:

- 1x **ESP32 development board** (commonly NodeMCU-style with 30 pins)
- 1x **Perfboard** (breadboard-style through-hole board, ~5cm × 7cm minimum)
- 2x pairs of **4.7kΩ resistors** for I²C pull-ups (one pair per bus)
- **Screw terminal blocks** (5mm pitch) for power input and bus outputs
- **22 AWG hookup wire** in different colors (red for 5V, black for GND, blue/green for SDA/SCL)
- **Solder and soldering iron**
- **Heat shrink tubing** (optional but recommended for protection)

![Controller board with labeled components](../../assets/esp32-controller-board-label.png){ width="500" .center }

*Labeled components on the controller board — ESP32, pull-up resistors, and screw terminals for power and dual I²C bus outputs*

## Pin Connections

The ESP32 has two dedicated I²C peripherals:

| Signal | Bus A | Bus B |
|---|---|---|
| **SDA** (data) | GPIO 21 | GPIO 23 |
| **SCL** (clock) | GPIO 22 | GPIO 19 |
| **GND** | GND | GND |

Each I²C bus also requires **pull-up resistors** on SDA and SCL to 5V (4.7kΩ each). Only one set of pull-ups per bus is needed, soldered on the perfboard.

## Assembly Steps

### 1. Prepare the Perfboard

- Mount the ESP32 on the perfboard using header sockets or by soldering the pins directly
- Plan the layout so power (5V/GND) runs along one edge and I²C signals are easily accessible
- Leave space for the pull-up resistors and output connectors

### 2. Add Screw Terminals

Solder screw terminal blocks to the perfboard for:

- **5V power input** (from the external power supply)
- **GND** (ground, connected to both ESP32 and power supply)
- **Bus A output** (SDA, SCL, GND to first split-flap daisy chain)
- **Bus B output** (SDA, SCL, GND to second split-flap daisy chain)

### 3. Solder Pull-up Resistors

For **Bus A**:

- Solder a 4.7kΩ resistor from the SDA line (GPIO 21) to the 5V rail
- Solder another 4.7kΩ resistor from the SCL line (GPIO 22) to the 5V rail

For **Bus B**:

- Repeat with GPIO 23 (SDA) and GPIO 19 (SCL)

### 4. Wire the Internal Connections

Run jumper wires from the ESP32 GPIO pins through the pull-up resistors to the screw terminals, and tie all grounds together. Keep power and signal runs neat so the back side stays clean.

![Internal wiring — front view](../../assets/esp32-controller-board-internal-wiring.png){ width="500" .center }

*Front-side internal wiring — ESP32 GPIO pins routed through pull-up resistors to the bus output terminals*

![Internal wiring — back view](../../assets/esp32-controller-board-internal-wiring-back.jpg){ width="500" .center }

*Back-side solder joints — keep traces short and well-soldered to avoid intermittent I²C errors*

### 5. Test Before Installation

- Power on the controller board and check that the ESP32 boots
- Use the firmware's I²C scanner (if available) to verify each bus can detect modules
- Confirm no shorts between 5V and GND

![Completed controller board](../../assets/esp32-controller-board-2.jpg){ width="500" .center }

*Completed controller board ready for installation*

## Connection to Dual Displays

The perfboard provides two independent daisy-chain outputs:

- **Bus A (GPIO 21/22)** — connects to the first chain of up to 8 split-flap modules
- **Bus B (GPIO 23/19)** — connects to the second chain of up to 8 split-flap modules

Each split-flap PCB has NEXT/PREV headers that daisy-chain along the row. The controller board's I²C outputs feed the first module in each chain; the modules then daisy-chain through their onboard headers.

![External wiring — controller to PSU and displays](../../assets/esp32-controller-board-external-wiring.png){ width="500" .center }

*External wiring — 5V power input from the PSU on one side, Bus A and Bus B outputs to the split-flap daisy chains on the other*

![Controller board installed](../../assets/esp32-controller-board-3.jpg){ width="500" .center }

*Controller board wired into the full dual-display setup*

![Controller board with bus wires connected](../../assets/esp32-controller-board-4.jpg){ width="500" .center }

*Power and I²C bus wires connected to the screw terminals — ready to drive both daisy chains*

![Controller board installed in the enclosure](../../assets/esp32-controller-board-5.jpg){ width="500" .center }

*Final installation — controller board mounted and powering the dual display*

## Power Distribution

All power runs through the main 5V supply connected to the controller board's screw terminals. The controller board is responsible for:

- Receiving 5V from the external PSU
- Supplying 5V and GND to the split-flap daisy chains via the output terminals
- The ESP32 itself draws minimal current (~80–100mA); most power goes directly to the motors

## Next Steps

Once the controller board is assembled and tested, proceed to [ESP32 Wiring](esp32-wiring.md) to understand how it connects to the full dual display system.
