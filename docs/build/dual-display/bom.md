# Bill of Materials

Everything you need to build the 16-module dual display. The hardware for each module (motor, hall sensor, nuts, screws) is the same as the original instructions — refer to those for per-module hardware not listed here.

---

## Common

Required for all builds regardless of power or board option.

| Item | Qty | Notes | Link |
|------|-----|-------|------|
| ESP32 DevKit V1 (ESP-WROOM-32) | 1 | | [Amazon](https://a.co/d/09N3pLDK) |
| A3144 Hall Effect Sensor | 16 | | [Amazon](https://a.co/d/0gNrxkEA) |
| N52-D3x1mm magnet | 16 | Stronger than the original design magnets | [Amazon](https://a.co/d/032d6gsi) |
| 40×60mm Perfboard | 1 | Controller board | [Amazon](https://a.co/d/0fF1662l) |
| 3 Pin 0.3" Pitch PCB Mount Screw Terminal | 2 | | [Amazon](https://a.co/d/0hYWviKW) |
| 2 Pin 0.1" Pitch PCB Mount Screw Terminal | 2 | | [Amazon](https://a.co/d/02ZxuIoB) |
| 16 AWG Wire | — | Red and black for 120V wiring | [Amazon](https://a.co/d/0dnlzv3A) |
| 18 AWG Wire | — | Red and black for 5V wiring | [Amazon](https://a.co/d/07qlweqJ) |
| 24 AWG Wire | — | White and yellow for I²C wiring | [Amazon](https://a.co/d/01tevNzU) |
| M3 × 350mm Fully Threaded Rod | 8 | 4 per row | [Amazon](https://a.co/d/03wEigg8) |
| M3 × 30mm Countersunk Machine Screws | 64 | 4 per module | [Amazon](https://a.co/d/0dZ8LeA2) |
| M3 × 6mm Countersink Machine Screws | 8 | End caps | [Amazon](https://a.co/d/0h6fTboP) |
| M3 Nut | 72 | 8 for end caps; 64 for the modules | [Amazon](https://a.co/d/08b878Ls) |
| M3 × 4 × 5 Heat Set Insert | 8 | End caps | [Amazon](https://a.co/d/01hBrO8S) |
| 4mm × 18mm Dowel Pins | 16 | I recommend [Jordan Hoff's metal dowel mod](https://makerworld.com/en/models/1269780-split-flap-display-metal-dowel-mod#profileId-1296205) | [Amazon](https://a.co/d/0g9X6VVT) |
| Black PETG Filament | ~2 kg | Everything except flaps | [Amazon](https://a.co/d/044ucNUs) |
| Black PLA Filament | ~1 kg | Flaps | [Amazon](https://a.co/d/0aR8hNht) |
| White PLA Filament | ~1 kg | Flaps | [Amazon](https://a.co/d/0fP4AraA) |

---

## Module Board

Choose one option. See the [Module Boards](../../module-boards/index.md) section for a full comparison.

=== "Custom PCB"

    *Parts listed assume the dowjames v5.1 design.*

    | Item | Qty | Notes | Link |
    |------|-----|-------|------|
    | dowjames v5.1 Custom PCB | 16 | Ordered assembled from JLCPCB | [Ordering guide](../../module-boards/custom-pcb/dowjames-v5.1/ordering.md) |
    | 5V 28BYJ-48 Stepper Motor | 16 | You don't need the separate driver board | [Amazon](https://a.co/d/02uVBov3) |
    | 3 Pin JST PH Female Connector | 16 | | [Amazon](https://a.co/d/0fr4haGp) |
    | 4.7kΩ Pull-Up Resistor | 2 | One per I²C bus — first board in each chain | [Amazon](https://a.co/d/0fgxiKNf) |
    | 2.54mm Pitch 4 Pin Stackable Header | 2 | [See assembly note](build-guide/assembly.md#stackable-header-note) | [Amazon](https://a.co/d/00aMJGHA) |

=== "DIY Board"

    Refer to the [original build instructions](https://www.instructables.com/Split-Flap-Display-3D-Printed-Modular-Compact-Encl/) for the per-module component list.

---

## Power Supply

Choose one option. See [Power](power.md) for details on each.

=== "Option A — Integrated PSU (recommended)"

    | Item | Qty | Optional | Notes | Link |
    |------|-----|----------|-------|------|
    | MEAN WELL LRS-75-5 | 1 | | | [Amazon](https://a.co/d/08QuIBPq) |
    | Main Power Switch | 1 | | | [Amazon](https://a.co/d/03vKCmQP) |
    | 16 AWG 6ft Computer Power Cord | 1 | | | [Amazon](https://a.co/d/0bsVQSLV) |
    | 16 AWG Wire Ferrule Terminals | 6 | | Recommended for 120V connections | [Amazon](https://a.co/d/08AVfBJG) |
    | 4.8mm 16 AWG Female Spade Connector | 3 | | | [Amazon](https://a.co/d/0gb0nYzu) |
    | M5 16 AWG Ring Connector | 5 | | | [Amazon](https://a.co/d/0dwV33gi) |
    | M3 × 35mm Countersunk Machine Screws | 2 | | | [Amazon](https://a.co/d/07yLTH74) |
    | M3 × 30mm Countersunk Machine Screws | 2 | | | [Amazon](https://a.co/d/0dZ8LeA2) |
    | M3 × 12mm Countersink Machine Screws | 2 | | | [Amazon](https://a.co/d/0gWgyT27) |
    | M3 × 6 × 5 Heat Set Insert | 5 | | | [Amazon](https://a.co/d/01hBrO8S) |
    | Shelly 1PM Gen4 | 1 | ✅ | Smart power monitoring | [Amazon](https://a.co/d/0e8rYym1) |
    | 16 AWG Wire Ferrule Terminals (Shelly) | 4 | ✅ | Not needed without the Shelly | [Amazon](https://a.co/d/08AVfBJG) |

=== "Option B — External Adapter"

    | Item | Qty | Notes | Link |
    |------|-----|-------|------|
    | 5V 10A power supply with 5.5×2.1mm DC barrel plug | 1 | | — |
