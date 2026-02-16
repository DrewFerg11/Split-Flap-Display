## Plan: Stepper Motor Accuracy Improvements

**TL;DR** — Your ~10% error rate likely comes from a combination of missed steps (no acceleration), I2C bus timing pressure at high module counts, sensor polling gaps, lack of per-module calibration, and motors losing position after being de-energized. Below are 14 suggestions ranked by expected impact, spanning software and hardware changes.

**Progress Summary:**
- ✅ **Half-stepping implemented** with full web UI control - foundation for accuracy improvements
- ✅ **All operational/accuracy settings** now configurable via web UI without reflashing
- ✅ **Sensor check interval optimized** - step-based checking more reliable than time-based
- ✅ **Smarter position rounding** - reduces quantization errors
- ❌ **Acceleration rejected** - tested and found unnecessary; constant speed is more reliable
- ⏳ **7 software enhancements remaining** - overshoot compensation, per-module calibration, etc.
- 🛠️ **3 hardware upgrades identified** - dual sensors, TMC2208 drivers, decoupling caps

---

### Software Changes


**1. Add Acceleration / Deceleration (Trapezoidal Ramp) — ~~HIGH IMPACT~~ → TESTED: NOT RECOMMENDED**

**Status**: ❌ Tested and rejected. **Conclusion: Skip this approach.**

**Implementation Notes**:
- Added `accelSteps` configurable setting (0=disabled, 30-100=ramp length)
- Implemented trapezoidal profile: ramp from 3 RPM → target RPM → 3 RPM
- Added per-module step tracking for independent acceleration curves
- Added watchdog yield (every 5ms) to prevent FreeRTOS timeout on long movements

**Critical Bugs Found During Testing**:
1. **Step counter overflow**: Modules exceeded expected step count, causing infinite loops
2. **Negative RPM calculation**: When currentStep > totalSteps, deceleration produced 0 RPM stalls
3. **Position tracking drift**: Taking all expected steps didn't guarantee arriving at target position
4. **Timing sensitivity**: 28BYJ-48 motors are too low-torque for smooth acceleration at tested values

**Test Results**:
- `accelSteps=0` (disabled): Baseline ~90% accuracy, works reliably
- `accelSteps=20-30`: Slight smoothing but no measurable accuracy improvement
- `accelSteps=50`: Unreliable homing, modules missed steps frequently
- `accelSteps=75-100`: Modules stalled or entered infinite loops (before fixes)
- Even with all bugs fixed, acceleration didn't improve accuracy over constant speed

**Root Cause Analysis**:
The ~10% error rate is NOT from abrupt starts/stops—it's from:
- Cumulative position drift between magnet corrections
- Mechanical backlash and gear slop in 28BYJ-48 motors
- I2C timing delays causing missed step commands
- Integer rounding errors in position calculations

**Recommendation**: Skip acceleration entirely. Focus on half-stepping (#2) for double resolution and better positioning granularity. The complexity and timing sensitivity of acceleration doesn't justify the negligible accuracy gain.

**2. Half-Stepping (8-Phase Sequence) — HIGH IMPACT**

**Status**: ✅ **COMPLETE** - Fully implemented, tested, and deployed with web UI control

**Implementation Details**:
- Added `halfStepping` boolean setting (default: `false` for backward compatibility)
- Implemented 8-phase sequence in `SplitFlapModule.cpp step()` method
- Alternates dual-coil (cases 0,2,4,6) and single-coil (cases 1,3,5,7) excitation
- Automatically doubles `stepsPerRot` from 2048 to 4096 when enabled
- Dynamic `maxStepNumber` calculation (4 vs 8 phases)
- **Critical fix**: `magnetPosition` and `displayOffset` are now automatically doubled when half-stepping is enabled

**Bug Fixed**: Initial implementation only doubled `stepsPerRot` but not `magnetPosition` or `displayOffset`, causing homing failures. The magnet is detected at a physical position that corresponds to different step counts depending on stepping mode (730 for full-step = 1460 for half-step).

**Performance Fix**: Half-stepping requires 2x more steps per rotation, which doubles all per-step overhead (I2C transactions, settle delays, sensor checks). The code now automatically halves `stepSettleUs` when half-stepping is enabled to maintain similar physical rotation speeds. For example, `stepSettleUs=100` becomes 50µs per step in half-step mode, giving equivalent 100µs per full-step unit.

**Performance Reality**: Testing shows that with half-stepping at 4096 steps/rotation controlled via I2C through PCF8575 expanders, the **practical maximum speed is ~10 RPM**, not 15 RPM. The 15 RPM datasheet specification for 28BYJ-48 motors was likely for full-stepping with direct drive or dedicated stepper drivers. Overhead sources:
- I2C transaction time: ~65µs per step command
- Multiplexer switching: Additional I2C operations per module
- Sensor checking: I2C reads every 10 steps
- Code execution: Position tracking, timing calculations, FreeRTOS overhead

With `stepSettleUs=0`, the system achieves 5-10 RPM depending on module count and movement complexity (60-66% of 15 RPM target). This is acceptable performance for a split-flap display where visual smoothness matters more than raw speed.

**How It Works**:
The 8-phase half-step sequence doubles resolution to **4096 positions/revolution**, giving:
- 4096 / 37 ≈ 110.7 steps per flap (vs. 55.35 now) — finer granularity, smaller rounding errors
- Smoother motion with less vibration
- Better low-speed torque (critical for reliable starting)
- Trade-off: slightly slower max speed at the same step rate, but the 28BYJ-48 is already slow

**Debug Logging Added**:
- Init sequence shows `stepsPerRot`, `magnetPosition`, `displayOffset` values for both modes
- Movement logging shows speed calculations and timing parameters
- Module initialization logs step sizes and character position samples

**Testing Results**:
- ✅ Verified init logs show correct parameter doubling: `stepsPerRot=4096, magnetPosition=1460`
- ✅ Homing works reliably across all 25 modules
- ✅ Web UI controls added for `halfStepping` toggle
- ✅ Performance confirmed: ~10 RPM practical max with I2C overhead (60-66% of 15 RPM target)
- ✅ Resolution doubled: 110.7 steps/char vs 55.35 steps/char (full-step)

**Achieved Benefit**: Improved positioning granularity, smoother motion, better low-speed torque. Essential foundation for accuracy improvements.

**3. Overshoot Compensation (Step Past, Then Settle) — MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

After reaching the target position, the drum has momentum and may coast 1 flap. Add a small "overshoot + settle back" routine:
- Step N steps past the target (e.g., 10-15 steps)
- Pause briefly (50-100ms)
- This ensures the flap is firmly seated against the stop position
- Since you only move forward, this doesn't require reverse stepping

**4. Per-Module Offset Calibration — MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

Currently all modules share a single `displayOffset`. Mechanical variations (magnet placement, gear tolerances, flap thickness) mean each module needs its own offset. The temp files show this was planned (`moduleOffsets[]`).
- Add a per-module offset array stored in settings (comma-separated list)
- Apply per-module offset in `SplitFlapModule` constructor alongside `magnetPos + stepOffset`
- Add a web UI calibration page where you can adjust each module individually

**5. Adaptive Speed Based on Movement Distance — MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

Short movements (1-3 flaps, ~55-165 steps) are more prone to overshoot than long movements. Use a slower speed for short movements:
- < 100 steps: cap at 5 RPM
- 100-500 steps: cap at 10 RPM
- > 500 steps: allow full speed
- This is per-module, calculated in the stepping loop

**6. Post-Movement Sensor Verification — MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

After all modules reach their targets, do a verification pass: check if the magnet is where it should be (if the target position is within the magnet's detection zone). If not, the module has drifted and should re-home. This catches errors that accumulate between magnet crossings.

**7. Reduce Sensor Check Interval — MEDIUM IMPACT**

**Status**: ✅ **IMPLEMENTED**
COMPLETE** - Implemented with web UI control

The `sensorCheckSteps` setting controls how often the hall sensor is checked during stepping. At line 1084 in SplitFlapDisplay.cpp, the code checks `if (stepsSinceSensorCheck[i] >= sensorCheckSteps)` to trigger sensor reads.

The current default `sensorCheckSteps = 10` provides reliable magnet detection with minimal overhead. At 4096 steps/rot (half-stepping), this gives ~410 sensor checks per rotation - guaranteed to catch the single magnet multiple times during its detection window.

**Configuration**: Fully adjustable via web UI (Settings → Advanced → Accuracy Settings). Step-based checking is more reliable and performant than time-based polling for single-magnet drums
**8. Holding Torque After Movement — LOW-MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

Currently `stop()` de-energizes all coils. If there's any vibration, airflow, or gravity effect, the drum can drift. Options:
- Keep one coil pair energized after stopping (holding torque mode) — uses power but locks position
- Add a configurable `holdTimeMs` setting: keep coils energized for N seconds after movement, then release
- This is especially important if modules are mounted vertically

**9. Periodic Re-Homing During Idle — LOW-MEDIUM IMPACT**

**Status**: ⏳ Not implemented yet.

This is "Priority 4" from the temp accuracy improvements doc. If the display is idle for more than N minutes, automatically re-home all modules, then restore the displayed text. This prevents long-term drift accumulation.

**10. Smarter `charPositions` Rounding — LOW IMPACT**

**Status**: ✅ **COMPLETE**

Changed `SplitFlapModule.cpp init()` from truncation `(int) currentPosition` to proper rounding:
```cpp
charPositions[i] = (int) round(currentPosition);
```
This ensures each flap position is as close to ideal as possible. Rounding distributes quantization error more evenly across all character positions instead of always biasing downward.

**Achieved Benefit**: Minor improvement in positioning accuracy at character boundaries.

**11. I2C Clock Speed Reduction for Reliability — LOW IMPACT**

**Status**: ⏳ Not implemented yet.

The code sets I2C to 400 kHz, but WIRING.md recommends 100 kHz for long runs. If your I2C wiring is more than ~30cm, 400 kHz can cause bit errors (NACK), which means missed steps. Consider:
- Defaulting to 100 kHz for reliability
- Making `i2cClockSpeed` a configurable setting
- Adding I2C error counters to the performance logging

---

### Hardware Changes


**12. Add a Second Hall Effect Sensor (or Optical Sensor) — HIGH IMPACT**
**Status**: 🛠️ Hardware change, not implemented.

A single magnet/sensor only corrects position once per full revolution. Adding a second sensor 180° opposite would correct twice per revolution, halving the maximum drift window. Alternatively, an optical encoder disc with slots at each flap position would give continuous absolute position feedback — this is how commercial split-flap displays achieve 100% accuracy.

**13. Use TMC2208 Stepper Drivers (Replace PCF8575 Direct Drive) — HIGH IMPACT**

**Status**: 🛠️ Hardware change, not implemented.

The PCF8575 I/O expander drives the 28BYJ-48 coils directly. A dedicated stepper driver like the TMC2208 supports:
- Microstepping (up to 256 microsteps) for much finer positioning
- StallGuard current sensing (detects missed steps in real-time)
- Silent operation (StealthChop)
- This is a significant hardware redesign but would transform accuracy

**14. Add Decoupling Capacitors Near Each PCF8575 — LOW IMPACT**

**Status**: 🛠️ Hardware change, not implemented.

If not already present, add 100nF ceramic capacitors as close as possible to each PCF8575's VCC/GND pins. I2C glitches from power supply noise can cause corrupted writes, resulting in missed or incorrect steps. The BOM mentions bulk capacitors but not per-IC decoupling.
