# Power

The dual display requires a stable **5V DC** supply capable of handling up to 16 stepper motors running simultaneously. There are two options to power your dual display. Choose your option no as it'll dictate the [hardware you buy](bom.md) and [parts you print](build-guide/printed-parts.md). 

## Why are there multiple options?

I originally designed the dual display to work with an external 5V power supply, connected via barrel jack. This option was clean, simple, and worked during my testing. It was only after I soldered the ESP32 controller board and attached 16 modules when I ran into power issues. The main issue was the ESP32 wouldn't boot from a cold start. I found a workaround to get it to boot, but then I started having some reliability issues when running all 16 modules.

I'm no expert, but I believe the power supply couldn't handle the inrush current, resulting in the ESP32 not getting sufficient power. This issue wasn't originally obvious because I did most of my testing with everything connected through a breadboard. For whatever reason, 16 modules works fine with the external power supply in that setup.

I tried fixing this issue by adding a thermistor. I got it working, but wasn't confident in the safety of that setup (the thermistor was extremely hot). Maybe someone can come up with a better solution, but I abandoned that route and switched to an integrated power supply.

For the record, my external power supply was a cheap Amazon purchase. You might be able to find a higher quality external power supply that doesn't have this issue.

## Power requirements

TODO

---

## Option A — Integrated PSU (recommended)

**MEAN WELL LRS-75-5** — 5V 14A enclosed switching supply, hardwired via terminal blocks, mounts inside a 3D-printed enclosure on the back of the display.

[MEAN WELL LRS-75-5 on Amazon](https://www.amazon.com/dp/B09DKGZ9Y5){ .md-button }

| Spec | Value |
|---|---|
| Output voltage | 5V DC |
| Output current | 14A |
| Max wattage | 70W |
| Input voltage | 85–264V AC (worldwide) |
| Connection | Screw terminal block |
| Certifications | CE, RoHS, UL |

**Pros:**

- Highest current headroom — 14A gives comfortable margin for all 16 motors
- Clean install — no external brick, PSU lives inside the printed enclosure on the back
- Worldwide input voltage — no adapter needed for 110V or 240V regions
- Industrial-grade reliability

**Cons:**

- Requires hardwiring (terminal block connections)
- Requires the 3D-printed PSU enclosure and extra hardware/assembly
- AC mains inside the enclosure — take care with wiring

!!! warning "Mains voltage"
    The LRS-75-5 connects directly to AC mains (110/240V). Ensure all mains-side wiring is properly insulated and the enclosure is fully closed before powering on.

---

## Option B — External Adapter

**5V 10A barrel jack adapter** — wall adapter brick that sits separately from the display. Simpler setup, no hardwiring required.

!!! warning "Known reliability issues at 16 modules"
    This is documented for completeness but has known cold-start reliability issues at full 16-module scale, the ESP32 may not boot reliably. See [Why are there multiple options?](#why-are-there-multiple-options) above. If you proceed with Option B, use a quality supply (not a [cheap Amazon generic](https://a.co/d/036pBEuH)) and test thoroughly before treating the build as complete.

To find a compatible supply: search for any **5V 10A** power supply with a **5.5×2.1mm DC barrel plug**. Quality varies significantly — avoid no-name generic units.

| Spec | Value |
|---|---|
| Output voltage | 5V DC |
| Output current | 10A min |
| Input voltage | 100–240V AC |
| Connection | 5.5×2.1mm barrel jack |

**Pros:**

- No hardwiring — plug straight in
- Simpler build — no PSU enclosure needed
- Easy to swap or replace

**Cons:**

- Known cold-start reliability issues at 16 modules — the ESP32 may fail to boot
- Lower headroom than Option A
- Brick sits externally — less tidy for a finished display

---

## Wiring

Both options deliver power to the first modules (+5V and GND), which daisy-chains through all subsequent boards. See the [Controller Board Assembly](build-guide/controller-board.md) for the full connection diagram.

!!! danger "Don't power through the ESP32 USB port"
    Avoid powering the display chain through the ESP32's USB port, especially on smaller variants (ESP32-C3, ESP32-S3). This works fine for testing a few modules modules at a time, but not the entire display. The USB traces aren't rated for motor current and will overheat. Power the boards directly from the 5V supply.
