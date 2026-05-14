# Build & Assembly

There are two build options for this project

<div class="grid cards" markdown>

- **Original Display**

    ---

    Single ESP32 controlling up to :material-numeric-8-box: modules over a single I²C bus. 
    
    The original design by Morgan Manly. This project was further improved upon by community members (notable Jordan Hoff, Thom Koopman, and various discord members) to refine the code, increase the flap count to 48, create custom pcbs, and much more.

    [:octicons-arrow-right-24: Original build guide](original/index.md)

- **Dual Display**

    ---

    Single ESP32 controlling up to :material-numeric-1-box::material-numeric-6-box: modules over two I²C buses. 
    
    Enhanced version based on the original design.

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
| Dual-bus firmware[^1] | ✅ | ✅ |
| Original (rounded) enclosure | ✅ | ❌ |
| Square enclosure | ✅ | ✅ |
| Supports 37 flaps | ✅ | ✅ |
| Supports 48 flaps | ✅[^2]  | ❌ |
| Enhanced flap accuracy[^3] | ✅ | ✅ |
| DIY PCB option | ✅ | ✅ |
| Purchased PCB option | ✅ | ✅ |

[^1]: _Code is backwards compatible for non-dual setups_

[^2]: _Square enclosure (currently) only supports 37. Original (rounded) enclosure supports both counts._

[^3]: _Only comes with the square enclosure._
