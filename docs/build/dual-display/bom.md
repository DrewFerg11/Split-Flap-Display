# PCB Ordering — v5.1 from JLCPCB

!!! note "Full guide"
    The complete step-by-step ordering guide (with screenshots) lives at [`custom-pcb/dowjames-v5.1/pcb-instructions.md`](https://github.com/DrewFerg11/Split-Flap-Display/blob/main/custom-pcb/dowjames-v5.1/pcb-instructions.md). It will be migrated into this site in a follow-up update.

## Quick summary

Each PCB is a per-module I²C-controlled stepper driver board. You need **one PCB per split-flap module**. Most people order 10 at a time (JLCPCB's minimum for assembly).

The boards daisy-chain via 4-pin NEXT/PREV headers carrying +5V, GND, SCL, and SDA — no wiring between boards.

## Files needed

Download [`splitflapv5.1-pcb.zip`](https://github.com/DrewFerg11/Split-Flap-Display/blob/main/custom-pcb/dowjames-v5.1/splitflapv5.1-pcb.zip) and unzip. Inside the `splitflapv5.1-pcb/` folder:

| File | Purpose |
|------|---------|
| `gerber.zip` | Gerber + drill files (PCB manufacturing) |
| `bom.csv` | Bill of materials with LCSC part numbers |
| `positions.csv` | Pick-and-place / CPL file |

## Key gotchas

!!! warning "Log in before uploading"
    Upload the gerber while logged out and JLCPCB may not auto-populate the PCB dimensions — you'll have blank X/Y fields.

!!! warning "Don't power motors without firmware"
    The PCF8575 (U1 on the board) pulls all output pins HIGH by default. With a motor connected and no firmware running, both coils are energized continuously — the motor and ULN2003 driver will overheat and can warp flaps.

!!! warning "Select C1 and C2_BYPASS1"
    These bypass capacitors may default to unchecked in the BOM review. They are required — select them. (They're capacitors, not resistors, despite the name.)

!!! warning "I²C pull-up resistors required"
    Add 4.7kΩ pull-up resistors from SDA → +5V and SCL → +5V on **one board** in the chain. The display will not work without them.
