# Build & Assembly

There are two build options for this project:

<div style="display: flex; gap: 1.5rem; margin: 2rem 0 0.25rem;" markdown>
<figure style="flex: 1; text-align: center; margin: 0;" markdown>
![8-module split-flap display showing SUN16FEB](../assets/display/8-modules.png)
</figure>
<figure style="flex: 1; text-align: center; margin: 0;" markdown>
![16-module dual split-flap display showing HELLO WORLD](../assets/display/16-modules.jpg)
</figure>
</div>

<div class="grid cards" markdown>

- **Original Display**

    ---

    Single ESP32 controlling up to :material-numeric-8-box: modules over a single I²C bus. 
    
    The original design by Morgan Manly. This project was further improved upon by the community (notably Jordan Hoff, Thom Koopman, and various Discord members) with refining the code, increasing the flap count to 48, creating custom pcbs, and much more.

    [:octicons-arrow-right-24: Original build guide](original/index.md)

- **Dual Display**

    ---

    Single ESP32 controlling up to :material-numeric-1-box::material-numeric-1-box: modules over two I²C buses. 
    
    Enhanced version by Drew Ferguson, based on the original design.

    [:octicons-arrow-right-24: Dual display guide](dual-display/index.md)

</div>

## Which build is right for you?

| | Original | Dual |
|---|---|---|
| Max modules | 8 | 16 |
| Display rows | 1 | 2 |
| Characters shown | 8 | 16 |
| I²C buses | 1 | 2 |
| Dual-bus firmware[^1] | ✅ | ✅ |
| Original (rounded) enclosure | ✅ | ❌ |
| Square enclosure | ✅[^2] | ✅ |
| Supports 37 flaps | ✅ | ✅ |
| Supports 48 flaps | ✅ | ❌ |
| Enhanced flap accuracy[^3] | ✅ | ✅ |
| DIY PCB option | ✅ | ✅ |
| Purchased PCB option | ✅ | ✅ |

[^1]: _Code is backwards compatible for non-dual setups_

[^2]: _Square enclosure (currently) only supports 37. Original (rounded) enclosure supports both counts._

[^3]: _Only comes with the square enclosure._
