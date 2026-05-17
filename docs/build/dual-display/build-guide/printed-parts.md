# 1. Printed Parts

All print files are available on MakerWorld — each plate is pre-configured with the correct print settings.

[Download Print Files on MakerWorld](#){ .md-button }

---

## Which plates do you need?

Which plates you print depends on the [power option](../power.md) you chose. **All builds require plates A, B, and E.** The C and D plates are mutually exclusive based on your power choice.

| Plate | Part | Option A (Integrated PSU) | Option B (External Adapter) |
|---|---|:---:|:---:|
| **A.1** | SFD Enclosure | ✅ ×14 | ✅ ×16 |
| **B.1** | End Cap — Right | ✅ | ✅ |
| **C.1** | End Cap — Left (Barrel 34.5mm) | ❌ | ✅ one of C.1 or C.2 |
| **C.2** | End Cap — Left (Barrel 37.5mm) | ❌ | ✅ one of C.1 or C.2 |
| **D.1** | SFD Enclosure Mount | ✅ ×2 | ❌ |
| **D.2** | End Cap — Left (PSU) | ✅ | ❌ |
| **D.3** | PSU Enclosure — Top | ✅ | ❌ |
| **D.4** | PSU Enclosure — Bottom | ✅ | ❌ |
| **E.1** | ESP Spacer | ✅ | ✅ |

---

## Plate details

### A.1 — SFD Enclosure
The main module enclosure body.

- **Option A:** print **14** — the remaining 2 slots are covered by the D.1 enclosure mounts
- **Option B:** print **16**

### B.1 — End Cap Right
The right-side end cap contains the plate and cap. One per dual display. I recommend a smooth plate for the end cap.

### C — End Cap Left (Barrel Jack)
*Option B (External Adapter) only.* The left-side end cap contains the plate and cap with a cutout for the barrel jack power connector.

- **C.1** — for a **34.5mm** barrel jack
- **C.2** — for a **37.5mm** barrel jack

Measure your barrel jack before printing. One per dual display. I recommend a smooth plate for the end cap.

### D — Integrated PSU Enclosure
*Option A (Integrated PSU) only.* All four D plates are required.

- **D.1 — SFD Enclosure Mount** — replaces two A.1 enclosures to allow the PSU enclosure to be mounted. Print **2**.
- **D.2 — End Cap Left (PSU)** — left-side end cap with cutout for AC mains wiring. I recommend a smooth plate for the end cap.
- **D.3 — PSU Enclosure Top** — top panel of the PSU housing (vented). I recommend a smooth plate for the top.
- **D.4 — PSU Enclosure Bottom** — bottom panel of the PSU housing with mounting points

### E.1 — ESP Spacer
A small spacer that sets the correct standoff height between the ESP32 and the perfboard when soldering. It also aligns the ESP32 reboot button with the side cutout in the enclosure so it's accessible without disassembly.
