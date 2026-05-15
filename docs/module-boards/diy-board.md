# DIY Board

The DIY board is the original approach from Morgan Manly's design — discrete breakout modules wired together to drive each split-flap module's stepper motor.

## Components (per module)

| Component | Purpose |
|---|---|
| **PCF8575** I²C I/O expander | Receives commands from ESP32 over I²C, drives motor coil outputs |
| **ULN2003** motor driver | Translates logic-level signals from the PCF8575 to the higher current needed by the stepper coils |
| **28BYJ-48** stepper motor | Drives the flap drum |
| **Hall sensor** | Detects the home magnet on the drum for zero-position calibration |
| Connecting wires | Link the breakout boards together |

## Address configuration

The PCF8575 has three address pins — A0, A1, A2 — set via the onboard DIP switch or solder pads. Each module in a chain must have a unique address. See the [I²C reference](../i2c.md#address-assignment-via-dip-switch) for the full address table.

!!! info "Pull-up resistors"
    You must add **4.7kΩ pull-up resistors** on SDA and SCL for the I²C bus to function. One set per bus is sufficient — add them on the first module in the chain. See the [I²C reference](../i2c.md#pull-up-resistors) for details.

## Pros and cons

**Pros:**

- Low per-unit cost — parts are inexpensive and widely available
- Good for experimenting with a single module before committing to a full build
- No minimum order

**Cons:**

- More soldering per module — each board requires wiring multiple components together
- Bulkier than the custom PCB
- More points of failure (hand-soldered connections)
- Harder to replicate consistently across 8–16 modules
