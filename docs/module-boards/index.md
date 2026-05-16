# Module Boards

Every split-flap module needs a board to drive its stepper motor. The board receives I²C commands from the ESP32, translates them into motor coil signals via a ULN2003 driver, and handles the hall sensor input used for homing.

There are two options:

<div class="grid cards" markdown>

- :material-soldering-iron: **DIY Board**

    ---

    Discrete components soldered together on a breakout board. The original design — lower upfront cost, more soldering work.

    [:octicons-arrow-right-24: DIY board guide](diy-board.md)

- :material-integrated-circuit-chip: **Custom PCB**

    ---

    A purpose-built PCB ordered assembled from JLCPCB. All SMT components pre-soldered — you only add pull-up resistors.

    [:octicons-arrow-right-24: Custom PCB guide](custom-pcb/index.md)

</div>

---

## Which should I choose?

| | DIY Board | Custom PCB |
|---|---|---|
| Upfront cost per-unit | Lower  | Higher (varies) |
| Soldering required | Significant | Minimal |
| Assembly time | Higher | Low |
| Reliability | Good | Excellent |
| Form factor | Bulkier | Compact, purpose-built |
| Availability | Source parts yourself | Ordered as a set from JLCPCB |
