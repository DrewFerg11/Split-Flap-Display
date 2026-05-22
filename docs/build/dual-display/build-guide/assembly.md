# Assembly

!!! warning "Disclaimer"
    These instructions are provided by a hobbyist for educational purposes only. The author is not a licensed engineer or electrician. **Follow these steps at your own risk.** No liability is accepted for any damage, injury, or loss — including from AC mains wiring in Option A. If you are not comfortable working with mains voltage, please consult a qualified electrician.

Most of the build steps from the [original guide](../../original/index.md) still apply, so these steps won't go into detail for items already documented there. If something is missing, please [open an issue](https://github.com/DrewFerg11/Split-Flap-Display/issues) and it'll get addressed.

I used a [custom PCB module](../../../module-boards/custom-pcb/index.md) for my setup, so these instructions focus on details specific to those boards. The steps are easily adaptable to the original [DIY module board](../../../module-boards/diy-board.md) — let me know if anything is unclear for that path.

## 1. Build Modules

Building each module is essentially the same as the original design. This includes:

- Installing the motor and hall sensor in the enclosure
- Assembling the drum with flaps and magnet
- Connecting the [DIY or custom PCB module board](../../../module-boards/index.md)

![Building modules](../../../assets/building-modules.jpg){ width="500" .center }

## 2. Solder Module Wires

Regardless of the [module board](../../../module-boards/index.md) you're using, solder wires to the **first module of each row** (the one that will sit closest to the controller board) before connecting all modules together. Use **18 AWG** wire for 5V/GND and **24 AWG** wire for both I²C lines. Approximate wire lengths:

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

Use a soldering iron to install heat set inserts into the [printed parts](printed-parts.md):

- **Left end plate** — 4× M3 × 4 × 5 heat set inserts
- **Right end plate** — 4× M3 × 4 × 5 heat set inserts
- **D.1 SFD enclosure mounts** (Option A only) — 4× M3 × 6 × 5 heat set inserts
- **PSU enclosure** (Option A only) — 2× M3 × 4 × 5 heat set inserts

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

Place the [controller board](controller-board.md) into the **left end plate** with the terminals facing down and the USB port facing forward. The fit should be snug — you may need to slightly flex the plate to slide the board in.

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
    If you're using the [dowjames v5.1 custom PCB](../../../module-boards/custom-pcb/dowjames-v5.1/index.md), you'll likely need a **2.54mm 4-pin stackable header** to connect the PCBs. The PCBs are slightly narrower than the module enclosure by design, which makes connecting 8 consecutive boards directly difficult. Add this header between modules 4 & 5 or 5 & 6 on each row to take up the slack.

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
<figcaption>Back side</figcaption>
</figure>
<figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
![Connecting modules step 2](../../../assets/connect-modules-2.jpg)
<figcaption>End plates</figcaption>
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

- **Bus 1** (GPIO 21/22 — left terminal) — drives the top row of up to 8 modules
- **Bus 2** (GPIO 33/32 — right terminal) — drives the bottom row of up to 8 modules

The controller board's I²C outputs feed the first module of each row; from there, the NEXT/PREV headers on each PCB daisy-chain power and I²C through the rest of the row.

Connect the I²C wires you soldered in [step 2](#2-solder-module-wires) to the matching terminals on the controller board.

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

    !!! warning "Mains voltage"
        These steps involve 120V AC wiring. If you're not comfortable working with mains voltage, get help from someone who is.

    ### A.9. Install PSU Hardware

    Dry-fit each piece of hardware in the PSU enclosure first to confirm everything seats correctly. You'll remove them again before wiring.

    - **Mount the [LRS-75-5](../bom.md#power-supply)** into the PSU enclosure.
    - **(Optional)** Install the [Shelly 1PM Gen4](../bom.md#power-supply) for smart power monitoring.
    - **Install the main power switch** into its 3D-printed mount with the switch facing the bottom.

    !!! warning "Switch is hard to remove once installed"
        Once pressed in, the switch is nearly impossible to remove without breaking the printed mount. Verify the mount has no imperfections and that you have the correct orientation before installing.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![PSU hardware install step 1](../../../assets/psu-hardware-install-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![PSU hardware install step 2](../../../assets/psu-hardware-install-2.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![PSU hardware install step 3](../../../assets/psu-hardware-install-3.jpg)
    </figure>
    </div>

    ### A.10. Prepare PSU Wires

    Use **16 AWG** for 120V lines. Cut and terminate wires based on whether you installed the Shelly.

    === "With Shelly"

        | Wire | From | To | Length | From Connector | To Connector |
        |---|---|---|---|---|---|
        | GND | Outlet (G) | PSU (G) | ~70mm | Female spade | M4 ring |
        | LINE | Outlet (L) | Shelly (L) | ~80mm | Female spade | Wire ferrule |
        | NEUTRAL | Outlet (N) | Shelly (N) | ~90mm | Female spade | Wire ferrule |
        | LINE | Shelly (O) | PSU (L) | ~90mm | Wire ferrule | M4 ring |
        | NEUTRAL | Shelly (N) | PSU (N) | ~80mm | Wire ferrule | M4 ring |

        <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
        <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
        ![PSU outlet wires](../../../assets/psu-outlet-wires-1.jpg)
        <figcaption>GND, LINE, and NEUTRAL from the outlet</figcaption>
        </figure>
        <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
        ![PSU wires](../../../assets/psu-wires-1.jpg)
        <figcaption>LINE and NEUTRAL from the PSU</figcaption>
        </figure>
        </div>

    === "Without Shelly"

        | Wire | From | To | Length | From Connector | To Connector |
        |---|---|---|---|---|---|
        | GND | Outlet (G) | PSU (G) | ~70mm | Female spade | M4 ring |
        | LINE | Outlet (L) | PSU (L) | ~70mm | Female spade | M4 ring |
        | NEUTRAL | Outlet (N) | PSU (N) | ~70mm | Female spade | M4 ring |

    Use **18 AWG** for the 5V DC lines from the PSU to the controller board. Keep the two wires paired — don't separate them.

    | Wire | From | To | Length | From Connector | To Connector |
    |---|---|---|---|---|---|
    | 5V | PSU (V+) | Controller board (+5V terminal) | ~280mm | M4 ring | Wire ferrule |
    | GND | PSU (V−) | Controller board (GND terminal) | ~280mm | M4 ring | Wire ferrule |

    ![5V wires](../../../assets/5v-wires-1.jpg){ width="500" .center }

    ### A.11. Wire the PSU

    You can now connect all the power hardware together.

    === "With Shelly"

        Screw down the 3 AC wires at the PSU terminals first, then connect the LINE and NEUTRAL wires to the outlet. Use the same outlet pins shown in the photo.

        ![PSU wired step 1](../../../assets/psu-wired-1.jpg){ width="500" .center }

        Next, place the LRS-75-5 into the enclosure and connect all the wires.

        <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
        <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
        ![PSU wired and installed step 1](../../../assets/psu-wired-installed-1.jpg)
        </figure>
        <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
        ![PSU wired and installed step 2](../../../assets/psu-wired-installed-2.jpg)
        </figure>
        <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
        ![PSU wired and installed step 3](../../../assets/psu-wired-installed-3.jpg)
        <figcaption>Note the twist in the GND wire.</figcaption>
        </figure>
        </div>

    === "Without Shelly"

        Screw down the 3 AC wires at the PSU terminals first, then connect all 3 to the outlet.

        !!! tip
            See the With Shelly tab for reference photos — the AC wiring at the PSU and outlet is similar.

    ### A.12. Mount the PSU Enclosure

    Place the PSU enclosure on the back of the display and secure it with **M3 × 12mm screws** through the two middle holes into the [D.1 mounts](printed-parts.md). You'll need to push the Shelly aside to access the second screw.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Mount PSU enclosure step 1](../../../assets/mount-psu-enclosure-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Mount PSU enclosure step 2](../../../assets/mount-psu-enclosure-2.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Mount PSU enclosure step 3](../../../assets/mount-psu-enclosure-3.jpg)
    </figure>
    </div>

    ### A.13. Connect PSU to Controller Board

    Thread the ferrule end of the 5V wires under the Shelly wires and out through the output hole on the PSU enclosure, just next to the outlet mount. Screw the ring end down at the PSU's +V and −V output terminals.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Connect PSU to controller step 1](../../../assets/connect-psu-controller-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Connect PSU to controller step 2](../../../assets/connect-psu-controller-2.jpg)
    </figure>
    </div>

    Thread the ferrule ends through the hole on the [D.2 end cap](printed-parts.md). **Don't attach the end cap to the end plate yet.** Screw the two ferrule ends into the **+5V** and **GND** terminals on the controller board.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Thread 5V wires step 1](../../../assets/thread-5v-wires-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Thread 5V wires step 2](../../../assets/thread-5v-wires-2.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Connect 5V to controller](../../../assets/connect-5v-controller-1.jpg)
    </figure>
    </div>

    !!! warning
        Double-check all wiring one last time before closing the end cap.

    Position the end cap over the end plate, leaving a small gap. Place one end of the wire cover into the PSU enclosure (routing the 5V wires through it) and align the other end over the hole on the end cap. Push the end cap fully closed while simultaneously pressing the wire cover into the hole — done correctly, the wire cover will lock into place.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Install wire cover step 1](../../../assets/install-wire-cover-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Install wire cover step 2](../../../assets/install-wire-cover-2.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Install wire cover step 3](../../../assets/install-wire-cover-3.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Install wire cover step 4](../../../assets/install-wire-cover-4.jpg)
    </figure>
    </div>

    ### A.14. Close the Enclosure

    Place the PSU enclosure top onto the mounted bottom half. Make sure the Shelly is seated in its slot, then press the top fully closed. Secure with 4 screws — **M3 × 35mm** for the bottom two holes and **M3 × 30mm** for the top two.

    !!! warning
        Double-check all wiring one last time before closing the enclosure top.

    ![Close PSU top](../../../assets/close-psu-top-1.jpg){ width="500" .center }

    Confirm the 8 threaded rod nuts on the right end plate are hand-tight. Place the right end cap over the plate, then secure both end caps with 8 **M3 × 6mm** screws (4 on each side).

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Close end cap step 1](../../../assets/close-end-cap-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Close end cap step 2](../../../assets/close-end-cap-2.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![Close end cap step 3](../../../assets/close-end-cap-3.jpg)
    </figure>
    </div>

    ### A.15. Power On & Test

    Plug in the power cord, flip the main switch, and verify the display boots and all modules respond.

    ![Powered PSU](../../../assets/psu-power-1.jpg){ width="500" .center }

=== "Option B — External Adapter"

    ### B.9. Install Barrel Jack

    Cut the 5V and GND wires to approximately 100mm each. Connect one end of each wire to the controller board's 5V/GND input terminals, and the other end to the barrel jack. Seat the barrel jack into the cutout in the [**C.1 or C.2 left end cap**](printed-parts.md) (matched to your jack diameter).

    ![Installing barrel jack](../../../assets/install-barrel-jack-1.jpg){ width="500" .center }

    ### B.10. Close the Enclosure

    Confirm the 8 threaded rod nuts on the right end plate are hand-tight. Place both end caps over their plates and secure with 8 **M3 × 6mm** screws (4 on each side). Make sure the barrel jack remains seated in its cutout as you close the left end cap.

    ### B.11. Power On & Test

    Plug in the external 5V power supply and verify the display boots and all modules respond.

    <div style="display: flex; gap: 1rem; margin: 1.5rem 0; flex-wrap: wrap;" markdown>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![External power setup 1](../../../assets/ext-power-1.jpg)
    </figure>
    <figure style="flex: 1 1 200px; text-align: center; margin: 0;" markdown>
    ![External power setup 2](../../../assets/ext-power2.jpg)
    </figure>
    </div>
