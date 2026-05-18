# Assembly

Note: most of the build steps from the [original guide](../../original/index.md) still apply, so these steps won't go into detail for items already documented there. If something is missing, please [open an issue](https://github.com/DrewFerg11/Split-Flap-Display/issues) and it'll get addressed.

I used a custom pcb module board for my setup, so these instructions will primarily focus on details specific to those boards, but this information is easily adaptable for the original diy boards.

## 1. Build Modules

Building each individual module is essentially the same as the original design. This includes:

- Installing the motor and hall sensor in the enclosure
- Assembling the drum with flaps and magnet
- Connecting the [DIY or custom PCB module board](../../../module-boards/index.md)

![Building modules](../../../assets/building-modules.jpg){ width="500" .center }

## 2. Solder Module Wires

Regardless of the [module board](../../../module-boards/index.md) you're using, you'll want to solder wires of the correct length to the **first module board in each row** before connecting all modules together. I used **18 AWG** wire for 5V/GND and **24 AWG** wire for both I²C lines. Approximate wire lengths:

- Row 1 (top): 90mm
- Row 2 (bottom): 140mm

!!! tip "How I wired the first module"
    I used the [dowjames v5.1 custom PCB](../../../module-boards/custom-pcb/dowjames-v5.1/index.md) and soldered the wires directly to the PREV header pins, then added heat shrink tubing for protection. You'll also notice the [pull-up resistors](../../../i2c.md#pull-up-resistors) soldered on.

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Module board wiring step 1](../../../assets/module-board-wire-1.jpg)
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Module board wiring step 2](../../../assets/module-board-wire-2.jpg)
</figure>
</div>

## 3. Prepare Threaded Rods

Cut all 8 threaded rods down to about **230mm** (minimum 225mm). The cut doesn't have to be exact — there should be some extra room at both ends, between the rods and the end caps.

## 4. Install Heat Set Inserts

Use a soldering iron to install the heat set inserts into the [printed end mounts](printed-parts.md):

- **Left end mount** — M3 × 4 × 5 heat set inserts × 4
- **Right end mount** — M3 × 4 × 5 heat set inserts × 4
- **SFD enclosure mounts** (Option A only) — M3 × 6 × 5 heat set inserts × 4
- **PSU enclosure mounts** (Option A only) — M3 × 4 × 5 heat set inserts × 2

!!! warning "Watch for melted plastic"
    As you sink the inserts, make sure no melted plastic is pushed up around them. Any plastic protruding above the surface will prevent the end caps from sitting flush and leave gaps in the enclosure. Use a sharp knife to trim any excess.

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Heat set inserts in end plates](../../../assets/heat-set-inserts-end-plates-1.jpg)
<figcaption>End plates</figcaption>
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Heat set inserts in SFD enclosure mounts](../../../assets/heat-set-inserts-sfd-1.jpg)
<figcaption>SFD enclosure mounts</figcaption>
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Heat set inserts in PSU enclosure mounts](../../../assets/heat-set-inserts-psu-1.jpg)
<figcaption>PSU enclosure mounts</figcaption>
</figure>
</div>

## 5. Install Controller Board

Place the [controller board](controller-board.md) in the **left end mount** with the terminals facing down and the USB port facing forward. The fit should be snug — you may need to slightly flex the end mount to allow the board to slide in.

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Controller board installation step 1](../../../assets/install-controller-board-1.jpg)
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Controller board installation step 2](../../../assets/install-controller-board-2.jpg)
</figure>
</div>

## 6. Connect Modules

Connect all modules together in two rows of 8. **Set the DIP switches on each module before connecting them** — see the [I²C address table](../../../i2c.md#address-assignment-via-dip-switch) for the correct address per position.

!!! tip
    Painter's tape helps hold modules together while you assemble the row.

<a id="stackable-header-note"></a>
!!! warning "Custom PCB — stackable header required"
    If you are using the [dowjames v5.1 custom PCB](../../../module-boards/custom-pcb/dowjames-v5.1/index.md), you'll likely need a **2.54mm 4-pin stackable header** to connect the PCBs. By design, the PCBs are slightly narrower than the enclosure, which makes 8 consecutive boards hard to connect. Place this header between modules 4 & 5 or 5 & 6 on each row.

The diagram below shows how the 16 modules are arranged, **viewed from the back**. Numbering runs 8→1 left to right (mirrored from the front).

For **Option A (Integrated PSU)**, modules 4 and 7 in **Row 2** use the **D.1 mount enclosure** instead of the standard A.1 — these are the positions where the PSU enclosure attaches.

<table style="border-collapse: collapse; text-align: center; width: 100%; table-layout: fixed;">
  <thead>
    <tr>
      <th style="padding: 0.4rem 0.3rem; text-align: left; width: 7rem;">← Back view</th>
      <th style="padding: 0.4rem 0.3rem;">8</th>
      <th style="padding: 0.4rem 0.3rem;">7</th>
      <th style="padding: 0.4rem 0.3rem;">6</th>
      <th style="padding: 0.4rem 0.3rem;">5</th>
      <th style="padding: 0.4rem 0.3rem;">4</th>
      <th style="padding: 0.4rem 0.3rem;">3</th>
      <th style="padding: 0.4rem 0.3rem;">2</th>
      <th style="padding: 0.4rem 0.3rem;">1</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="padding: 0.4rem 0.3rem; text-align: left; font-weight: bold;">Row 1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
    </tr>
    <tr>
      <td style="padding: 0.4rem 0.3rem; text-align: left; font-weight: bold;">Row 2</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444; font-weight: bold; background: #b45309; color: #fff;">D.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444; font-weight: bold; background: #b45309; color: #fff;">D.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.4rem 0.3rem; border: 1px solid #444;">A.1</td>
    </tr>
  </tbody>
</table>

**A.1** — Standard SFD Enclosure &nbsp;|&nbsp; **D.1** — Mount Enclosure (Option A — Integrated PSU only)

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Connecting modules step 1](../../../assets/connect-modules-1.jpg)
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Connecting modules step 2](../../../assets/connect-modules-2.jpg)
</figure>
</div>

## 7. Connect End Plates

Connect the end plates to each side and insert the threaded rods into all 8 holes. **Be careful not to push too hard on the lower front hole** that slots through the flaps — you may hit a couple of flaps, but gently twisting the rod should let it pass. Finally, install 8 M3 nuts on each side. Hand-tighten enough to secure all modules, but not so much that the end modules get squeezed and the flap drums rub against the inside of the enclosure.

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Connecting end plate step 1](../../../assets/connect-end-plate-1.jpg)
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Connecting end plate step 2](../../../assets/connect-end-plate-2.jpg)
</figure>
</div>

## 8. Wire Controller Board to Modules

The [controller board](controller-board.md) provides two independent daisy-chain outputs:

- **Bus 1** (GPIO 21/22 — left terminal) — connects to the first chain (top row) of up to 8 split-flap modules
- **Bus 2** (GPIO 33/32 — right terminal) — connects to the second chain (bottom row) of up to 8 split-flap modules

Each split-flap PCB has NEXT/PREV headers that daisy-chain along the row. The controller board's I²C outputs feed the first module in each chain; the modules then daisy-chain through their onboard headers.

Connect the I²C wires you soldered in [step 2](#2-solder-module-wires) to the matching I²C output terminals on the controller board.

![External wiring — controller to PSU and displays](../../../assets/esp32-controller-board-external-wiring.png){ width="500" .center }

*External wiring — 5V power input from the PSU on one side, Bus 1 and Bus 2 outputs to the split-flap daisy chains on the other*

<div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Wiring controller board step 1](../../../assets/wire-control-board-1.jpg)
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Wiring controller board step 2](../../../assets/wire-control-board-2.jpg)
</figure>
</div>

!!! tip "Test before closing the enclosure"
    This is a good time to flash the firmware (if you haven't already) and verify all 16 modules respond and home correctly. It's much easier to troubleshoot a misbehaving module now than after the enclosure is sealed.

---

The next steps depend on which power option you chose. Skip to the section that matches your build.

=== "Option A — Integrated PSU"

    ### 9. Install PSU Hardware

    !!! warning "Mains voltage"
        These steps involve 120V AC wiring. If you're not comfortable working with mains voltage, get help from someone who is.

    - **Install the Main Power Switch** into its 3D-printed mount with the switch facing the bottom.

        !!! warning "Switch is hard to remove once installed"
            Once pressed in, the switch is nearly impossible to remove without breaking the printed mount. Verify the mount has no imperfections and that you have the correct orientation before installing.

    - **Mount the LRS-75-5** into the PSU enclosure.
    - **(Optional)** Install the [Shelly 1PM Gen4](../bom.md#power-supply) for smart power monitoring.

    ### 10. Wire the PSU

    - Cut the 120V wires to the correct lengths and crimp on ferrules/spade/ring connectors as needed.
    - Connect Line, Neutral, and Ground wires to the LRS-75-5's input terminals.
    - If installing the Shelly, wire it inline with the AC input per its documentation.

    ### 11. Connect PSU to Controller Board

    - Connect 5V and GND wires from the LRS-75-5's output terminals to the controller board's power input terminals.
    - Thread the wires through the hole on the back of the **left end cap**.
    - Place the end cap into position over the controller board — **don't screw it down yet**.
    - Route the 5V/GND wires through the corresponding hole in the PSU enclosure.
    - Screw down the wires at the PSU's output terminals.
    - **Double-check all wiring one last time** before closing the enclosure.

    ### 12. Mount the PSU Enclosure

    Screw the PSU enclosure to the back of the display using the **M3 × 12mm screws** through the middle holes of the D.1 mounts.

    ### 13. Close the Enclosure

    - Place the PSU cable cover into the PSU enclosure.
    - Open the end cap just enough for the wire cover to slot into the end cap hole — the wire cover should be secure once the end cap is in place.
    - Tighten down the end cap screws.
    - Screw on the PSU enclosure top.

    ### 14. Power On & Test

    Plug in the power cord, flip the main switch, and verify the display boots and all modules respond.

=== "Option B — External Adapter"

    ### 9. Install Barrel Jack

    - Cut the 5V/GND wires to the correct length.
    - Connect one end to the controller board's 5V/GND input terminals.
    - Connect the other end to the barrel jack.
    - Place the barrel jack into the cutout in the **C.1 or C.2 left end cap** (matched to your jack diameter).

    ### 10. Close the Enclosure

    Screw on the end caps. Make sure the barrel jack is seated properly in the cutout.

    ### 11. Power On & Test

    Plug in the external 5V power supply and verify the display boots and all modules respond.
