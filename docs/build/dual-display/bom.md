# Bill of Materials

Everything you need to build the 16-module dual display. Items marked **required** are needed for every build. Power supply is **choose one**.

---

## Modules & PCBs

| Item | Qty | Notes |
|------|-----|-------|
| dowjames v5.1 PCB (JLCPCB assembled) | 16 | Order 16 minimum — see [PCB ordering guide](../../../custom-pcb/dowjames-v5.1/pcb-instructions.md) |
| Stepper motor | 16 | One per module |
| Hall effect sensor — TI DRV5033 | 16 | Polarity-agnostic; one per module |
| 4.7kΩ through-hole resistors | 2 | I²C pull-ups — only needed on one PCB per bus |

---

## Enclosure

| Item | Qty | Notes |
|------|-----|-------|
| Square enclosure — printed parts | 16 sets | [MakerWorld — square enclosure](https://makerworld.com/en/models/2489058-split-flap-display-square-enclosure#profileId-2734898) |
| PSU enclosure — printed parts | 1 set | 3D printed housing that mounts to back of display (integrated PSU option only) |

---

## ESP32 Controller Board

| Item | Qty | Notes |
|------|-----|-------|
| ESP32-WROOM DevKit V1 | 1 | Required for dual-bus support |
| Perfboard | 1 | For controller board assembly |
| Screw terminals | — | GND, +5V, I²C Bus A, I²C Bus B |

See [ESP32 controller board wiring](esp32-wiring.md) for the full build.

---

## Power Supply — choose one

=== "Option A — Integrated PSU (recommended)"

    **MEAN WELL LRS-75-5** — 5V 14A, hardwired via terminal blocks, mounts inside a 3D-printed enclosure on the back of the display.

    - Higher current headroom (14A) — comfortable margin for 16 motors
    - Cleaner install — no external brick
    - Requires terminal block wiring and the printed PSU enclosure
    - 110V/240V input — works worldwide

    | Spec | Value |
    |------|-------|
    | Output | 5V DC |
    | Current | 14A |
    | Wattage | 70W |
    | Connection | Terminal block |

    [MEAN WELL LRS-75-5 on Amazon](https://www.amazon.com/dp/B09DKGZ9Y5){ .md-button }

=== "Option B — External Adapter"

    **5V 10A barrel jack adapter** — wall adapter brick that sits separately from the display. Simpler setup, no hardwiring required.

    - Easier to set up — plug straight in
    - Lower current (10A) — sufficient for 16 modules but less headroom
    - Brick sits externally — less tidy for a finished display
    - Check availability before ordering (stock varies)

    | Spec | Value |
    |------|-------|
    | Output | 5V DC |
    | Current | 10A |
    | Wattage | 50W |
    | Connection | 5.5×2.1mm barrel jack |
