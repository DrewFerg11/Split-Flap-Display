# Assembly

Note: most of the build steps from the original guide still apply. So we won't re-cover them all. If there is something missing, please make an issue in GH and we'll get it addressed.

### 1. Build Modules

Building each individual module is essentially the same as the original design. This includes:

- Installing the motor and hall sensor in the enclosure
- Assembling the drum with flaps and magnet
- Connecting the module board
- Connecting all modules together in two rows of 8

#### Module layout

The diagram below shows how the 16 modules are arranged, **viewed from the back**. Numbering runs 8→1 left to right (mirrored from the front).

For **Option A (Integrated PSU)**, modules 4 and 7 in Row 1 use the **D.1 mount enclosure** instead of the standard A.1 — these are the positions where the PSU enclosure attaches.

<div style="overflow-x: auto; margin: 1.5rem 0;">
<table style="border-collapse: collapse; text-align: center; min-width: 500px;">
  <thead>
    <tr>
      <th style="padding: 0.5rem 1rem; text-align: left;">← Back view</th>
      <th style="padding: 0.5rem 1rem;">8</th>
      <th style="padding: 0.5rem 1rem;">7</th>
      <th style="padding: 0.5rem 1rem;">6</th>
      <th style="padding: 0.5rem 1rem;">5</th>
      <th style="padding: 0.5rem 1rem;">4</th>
      <th style="padding: 0.5rem 1rem;">3</th>
      <th style="padding: 0.5rem 1rem;">2</th>
      <th style="padding: 0.5rem 1rem;">1</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="padding: 0.5rem 1rem; text-align: left; font-weight: bold;">Row 1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
    </tr>
    <tr>
      <td style="padding: 0.5rem 1rem; text-align: left; font-weight: bold;">Row 2</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444; font-weight: bold; background: #b45309; color: #fff;">D.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444; font-weight: bold; background: #b45309; color: #fff;">D.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
      <td style="padding: 0.5rem 1rem; border: 1px solid #444;">A.1</td>
    </tr>
  </tbody>
</table>
</div>

**A.1** — Standard SFD Enclosure &nbsp;|&nbsp; **D.1** — Mount Enclosure (Option A — Integrated PSU only)



### 2. Install Heat Set Inserts

Use a soldering iron to install the heat set inserts into:

- Left end mount
- Right end mount
- Enclosure mount x2 (if used the external PSU)

