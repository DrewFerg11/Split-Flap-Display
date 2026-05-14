# Build & Assembly

There are two build paths to choose from:

<div class="grid cards" markdown>

- **Original Display** - :material-numeric-8-box:

    ---

    Single ESP32 controlling up to 8 modules over a single I²C bus. The original design by Morgan Manly.

    [:octicons-arrow-right-24: Original build guide](original/index.md)

- **Dual Display** - :material-numeric-1-box::material-numeric-6-box:

    ---

    Single ESP32 controlling up to 16 modules over two I²C buses. Enhanced version based on the original design.

    [:octicons-arrow-right-24: Dual display guide](dual-display/index.md)

</div>

<div style="display: flex; gap: 1.5rem; margin: 2rem 0;" markdown>
<figure style="flex: 1; text-align: center;" markdown>
![8-module split-flap display showing SUN16FEB](../assets/8-modules.png)
<figcaption>Original</figcaption>
</figure>
<figure style="flex: 1; text-align: center;" markdown>
![16-module dual split-flap display showing HELLO WORLD](../assets/16-modules.jpg)
<figcaption>Dual</figcaption>
</figure>
</div>

## Which path is right for you?

| | Original | Dual |
|---|---|---|
| Max modules | 8 | 16 |
| Display rows | 1 | 2 |
| Characters shown | 8 | 16 |
| I²C buses | 1 | 2 |
| Dual-bus firmware | ✅* | ✅ |
| Original (rounded) enclosure | ✅ | ❌ |
| Square enclosure | ✅ | ✅ |
| Enhanced flap accuracy | ✅** | ✅ |
| DIY PCB option | ✅ | ✅ |
| Purchased PCB option | ✅ | ✅ |

*_Firmware is backwards compatible_

**_Only possible with square enclosure_

