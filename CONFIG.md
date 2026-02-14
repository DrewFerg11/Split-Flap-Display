# Split Flap Display - Configuration Reference

## Operational Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `debugLogging` | bool | `true` | Enable general debug output (init, config, commands) |
| `perfLogging` | bool | `true` | Enable I2C bus performance metrics logging per movement |
| `i2cTransactionTime` | int | `65` | Estimated microseconds per I2C transaction (used for utilization % calculation) |
| `quickHome` | bool | `true` | Skip label/blank phases during home sequence (faster startup) |

---

## Accuracy Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `accuracyLogging` | bool | `true` | Enable detailed accuracy/calibration debug output |
| `stepSettleUs` | int | `100` | Microseconds to wait after each step for motor coil settling (0=disabled) |
| `sensorDebounceCount` | int | `1` | Consecutive sensor reads required before triggering magnet detection (1=no debounce, increase for noisy sensors) |
| `sensorDebugMs` | int | `0` | Log hall sensor HIGH/LOW transitions for N ms after first move (0=disabled, useful for debugging sensor behavior) |
| `sensorCheckSteps` | int | `10` | Steps between sensor reads (0=use time-based 20ms polling, 10-15 recommended for reliable magnet detection) |
| `retryFailedSteps` | int | `3` | I2C retry attempts on step failure (0=disabled, helps overcome temporary I2C glitches) |
| `missedMagnetRecovery` | bool | `true` | Auto-home modules that miss magnet crossings during movement (automatic error recovery) |
| `errorStatsTracking` | bool | `true` | Track position error statistics per module (max error, avg error, correction count) |

---

## Logging Output Reference

### Debug Logging (`debugLogging`)
- `[INIT] Wire initialized: SDA=... SCL=... @ 400kHz`
- `[INIT] Wire1 initialized: SDA=... SCL=... @ 400kHz`
- `[CMD] writeChar: '...' to all ... modules (speed=...)`
- `[CMD] writeString: '...' (speed=..., centering=...)`
- `[CMD] writeDisplays: ... displays (speed=..., centering=...)`
- `Display ...: '...'` (per display in writeDisplays)
- `[CONFIG] Dual bus mode: ENABLED/DISABLED`
- `[CONFIG] Mux...: 0x.. on Wire (bus 0)`
- `[CONFIG] Mux...: 0x.. on Wire1 (bus 1)`
- `[CONFIG] Using default: Mux0 at 0x70 on Wire`
- `[CONFIG] Loading ... for Mux...`
- `[INIT] Bus0Task created on Core 0`
- `[INIT] Bus1Task created on Core 1`
- `[INIT] Parallel execution initialized`
- `=== I2C Configuration ===` and `=== I2C Scanner ===` summaries

### Perf Logging (`perfLogging`)
- `[PERF Bus%d] mods=%d/%d steps=%d(~%d/mod) i2c=%d dur=%dms rpm=%.1f(%.0f%%) ops=%d/s util=%.1f%%`
  - **mods=X/Y**: X active modules out of Y total on this bus
  - **steps=N(~M/mod)**: Total steps and average per module
  - **i2c=N**: Total I2C operations (steps + mux selects + sensor reads)
  - **dur=Nms**: Wall time for movement
  - **rpm=X(Y%)**: Achieved RPM and percentage of target speed
  - **ops=N/s**: I2C operations per second
  - **util=X%**: I2C bus utilization

### Accuracy Logging (`accuracyLogging`)
- `[ACC Bus%d] Mod%d magnet correction: OLD_POS -> NEW_POS` (when magnet triggers position snap)
- `[ACC Bus%d] Mod%d arrived at pos=... (target=...)` (when module reaches target position)
- `[STATS Bus%d] Mod%d: corrections=..., maxError=..., avgError=...` (summary of error tracking per module, only if `errorStatsTracking=true` and corrections occurred)
- `[ACC Bus%d] Summary: ... magnet corrections applied` (at end of movement)

### Other Always-On Logs
- `[SENSOR Bus%d] Mod%d: HIGH/LOW at pos=...` (only if `sensorDebugMs > 0`, logs hall sensor transitions)
- `[RECOVERY Bus%d] Mod%d missed magnet: expected ... crossings, got ... - auto-homing` (only if `missedMagnetRecovery=true`)
- `[RECOVERY Bus%d] Mod%d homed successfully at step ...`
- `[RECOVERY Bus%d] Mod%d repositioned to target ...`
- `[ERROR Bus%d] Mod%d failed to home after ... steps`
- `[ERROR] Invalid mux index ...`
- `[ERROR] MUX 0x... channel select failed: error ...`
- `Error writing data to module 0x... error code: ...` (basic I2C write error)
- `[ACC] Module 0x...: I2C write failed after ... attempts` (retry logic error)

---

## Quick Tuning Tips

**For Maximum Accuracy (100% target):**
- `stepSettleUs = 200-300` (increase from 100 to ensure coils stabilize)
- `retryFailedSteps = 3` (enables automatic I2C retries)
- `missedMagnetRecovery = true` (auto-homes if magnet is missed)
- `errorStatsTracking = true` (monitor error trends)
- `sensorCheckSteps = 10` (check magnet more frequently)

**For Performance (faster movements, acceptable ~95% accuracy):**
- `stepSettleUs = 0` (disable coil settling delay)
- `sensorCheckSteps = 25-50` (reduce sensor check frequency)
- `accuracyLogging = false` (disable accuracy logs)
- `perfLogging = false` (disable performance logs)

**For Debugging Issues:**
- `accuracyLogging = true` (see all magnet corrections)
- `sensorDebugMs = 5000` (log hall sensor transitions for 5 seconds after movement starts)
- `perfLogging = true` (see I2C utilization and achieved RPM)
- `errorStatsTracking = true` (identify problematic modules)
