# Dual I2C Bus Support — Implementation Plan

Control up to **16 split-flap modules** from a single ESP32-WROOM by using both
hardware I2C peripherals (`Wire` and `Wire1`). Each bus has its own address list
and offset list. Addresses may be reused across buses.

## Design Decisions

| Decision | Choice |
|----------|--------|
| Bus settings model | Two parallel setting pairs: `wire_moduleAddresses` / `wire1_moduleAddresses` and `wire_moduleOffsets` / `wire1_moduleOffsets` |
| `moduleCount` | **Derived** at init — `wire_moduleAddresses.size() + wire1_moduleAddresses.size()`. The explicit `moduleCount` setting is removed. |
| Address reuse | **Allowed** — both buses can use the same 0x20–0x27 range |
| Compile-time flag | `ENABLE_DUAL_I2C` — only defined for the WROOM env |
| Backwards compat | C3 and S3 envs do **not** define the flag; all `wire1` code compiles out. |

---

## Implementation Steps

### Step 1: Core Firmware ✅

- [x] Add `TwoWire *wire` member to `SplitFlapModule`; extend constructor with `TwoWire *wireBus = &Wire` param; replace all bare `Wire.` calls with `wire->` in `writeIO()` and `readHallEffectSensor()`
- [x] Add `SDA2_PIN`, `SCL2_PIN`, and `ENABLE_DUAL_I2C=1` build flags to WROOM env; add `SDA_PIN`/`SCL_PIN` defaults to C3 and S3 envs
- [x] Replace fixed `#define MAX_MODULES 8` with a conditional — 16 when `ENABLE_DUAL_I2C`, 8 otherwise
- [x] Replace `moduleAddresses` / `moduleOffsets` / `moduleCount` settings with wire-prefixed equivalents in `SplitFlapDisplay.ino`; add conditional `wire1_*`, `sda2Pin`, `scl2Pin` settings
- [x] Update private members in `SplitFlapDisplay.h` — wire/wire1 address and offset arrays, counts, and conditional `SDA2Pin` / `SCL2Pin`
- [x] Rewrite `SplitFlapDisplay::init()` — reads wire/wire1 settings, derives `numModules`, initialises both buses, and populates `modules[]`
- [x] Add `logConfiguredModules()` and `scanI2cModules()` diagnostics; call both from `init()` on every boot

### Step 2: Threaded Dual-Bus Movement Engine ✅

The two I2C buses (Wire / Wire1) control physically separate rows of modules — top row (Wire, up to 8) and bottom row (Wire1, up to 8). To achieve reliable timing, each bus runs its movement loop on an independent FreeRTOS task pinned to a separate ESP32 core.

**Step 2a — Extract `moveModules()` static helper**

- [x] Extract the core stepping loop from `moveTo()` into a `static moveModules()` method operating on a subset of modules
- [x] Uses VLA arrays sized by `count` — consistent with the rest of the codebase (`int targetPositions[numModules]` is used in 9+ places). `count` is always bounded by `MAX_MODULES` since it comes from `wireCount`/`wire1Count` which are populated from `MAX_MODULES`-sized arrays, so stack usage is statically bounded in practice. _(Note: original plan called for fixed `[MAX_MODULES]` arrays for FreeRTOS safety, but we reverted to VLAs for consistency since the worst-case allocation is identical.)_
- [x] Handles motor start, stepping, hall-effect sensor checks, and motor stop within a single call

**Step 2b — FreeRTOS threading infrastructure**

- [x] Add `BusMoveParams` struct and `busMovementTask()` static function (file-scope in .cpp)
- [x] Implement `moveToDual()` — creates two FreeRTOS tasks with 4096-byte stacks (bus 1 → core 1, bus 2 → core 0), waits via counting semaphore
- [x] Dynamic task count — skips task creation if a bus has 0 modules

**Step 2c — Auto-dispatch in `moveTo()`**

- [x] When `wire1Count > 0`, `moveTo()` delegates to `moveToDual()` — all existing methods (`writeString`, `homeToString`, `testAll`, etc.) automatically get parallel dual-bus support

**Step 2d — Explicit dual homing**

- [x] Implement `homeDual()` — two-phase parallel homing (spin to find magnet → move to space char) using `moveToDual()` directly
- [x] `home()` dispatches to `homeDual()` when `wire1Count > 0`

**Step 2e — `writeStringDual(String row1, String row2)`**

- [x] Writes different text to each row independently with per-row centering
- [x] Available for Step 4 UI integration; existing `writeString()` unchanged (treats all modules as one flat string)

**Step 2f — Bug fixes**

- [x] Fix `homeToChar()` argument order: `moveTo(targets, true, speed)` → `moveTo(targets, speed)` (was passing `true` as speed ≈ 1 RPM)
- [x] Move `wire1Count` declaration outside `#ifdef ENABLE_DUAL_I2C` (fixes compilation for C3/S3 envs)

**Step 2g — Helper signature refactor (not in original plan)**

- [x] `startMotors()` / `stopMotors()` / `checkAllFalse()` made `static` and refactored to take explicit `(SplitFlapModule *mods, int count)` parameters. Required because the static `moveModules()` operates on a subset and cannot reference instance members `this->modules` / `this->numModules`. Existing call sites in `home()` / `homeToString()` / `homeToChar()` updated to pass `(modules, numModules)`.
- [x] Added `#include <freertos/semphr.h>` for semaphore primitives used in `moveToDual()`.

### Step 3: Frontend Settings Page ✅

**3a — `index.js` data model:**
- [x] Add `isDualI2C` computed property — true when `settings.wire1Addresses` is present
- [x] Replace `addressArray` / `offsetArray` / `setAddress` / `setOffset` with bus-prefixed versions: `wireAddressArray`, `wireOffsetArray`, `wire1AddressArray`, `wire1OffsetArray` plus matching setters
- [x] Add `addWireModule` / `removeWireModule` / `addWire1Module` / `removeWire1Module` helpers
- [x] Add `wireCount` / `wire1Count` getters derived from array length

**3b — `settings.html` markup:**
- [x] Remove `moduleCount` input — count is now derived from array length
- [x] Replace single address/offset grids with labeled "Bus 1" section; +/− controls to add/remove modules
- [x] Add "Bus 2" section with address + offset grids, wrapped in `x-if="isDualI2C"`
- [x] Add `sda2Pin` / `scl2Pin` inputs in Advanced section, hidden via `x-show="isDualI2C"`
- [x] Update Hardware Settings help-modal copy
- [x] Update validation error key references to match new key names
- [x] Remove hardcoded `disabled` attribute from Save Settings button

**Bug fix: NVS key length (found during Step 3 testing)**
- [x] ESP32 NVS has a 15-character key limit. `wire_moduleAddresses` (20), `wire_moduleOffsets` (18), `wire1_moduleAddresses` (21), `wire1_moduleOffsets` (19) all exceeded the limit — `putString` silently failed, returning defaults on every reload. Renamed to `wireAddresses` (13), `wireOffsets` (11), `wire1Addresses` (14), `wire1Offsets` (12) across `.ino`, `.cpp`, `index.js`, and `settings.html`.

### Step 4: Backend Validation

The existing validation API (`JsonSetting::validate()`) is **per-field, string-based** — it only checks one value at a time. Cross-field invariants (e.g. address-list length must equal offset-list length) need a `validateCrossFields()` hook in `JsonSettings::fromJson()` that runs before any writes to flash.

- [x] Implement `validateCrossFields(const JsonDocument &doc)` in `JsonSettings.cpp` — called at the top of `fromJson()` before `preferences.begin()`
- [x] Validate `wireAddresses.size() == wireOffsets.size()`; same for the `wire1*` pair when present
- [x] Validate `wireCount + wire1Count <= MAX_MODULES` (16 on WROOM, 8 elsewhere)
- [x] Validate each I²C address is a valid MCP23017 address (32–39); generic I²C range was too broad
- [x] Validate no duplicate addresses within the same bus (duplicates across buses are intentional and allowed)
- [x] Validate `sdaPin != sclPin`; `sda2Pin != scl2Pin`; no pin shared across Bus 1 and Bus 2
- [x] Toast now shows the specific validation error; added error divs under `sda2Pin` and `scl2Pin` inputs

### Step 5: Custom Text Mode — Dual Display Enhancements

#### Design decisions (resolved)
- **Combined vs Separate Rows:** Dual mode defaults to **Separate Rows**. A "Combined / Separate" toggle appears in the UI only when `isDualI2C`. Combined = one input spanning all modules (existing behavior). Separate = two inputs, one per row.
- **Multi-word in Separate mode:** Single shared word list, pairs approach — word[0] on Row 1 + word[1] on Row 2, advancing together each cycle.
- **Overflow handling:** Input border and counter turn red when over limit; backend truncates silently. Multi-word chips turn red if the word exceeds the row's module count. No per-character highlighting (requires contenteditable, too complex).
- **Toggle persistence:** Does not persist between page loads; always resets to Separate Rows default in dual mode.

#### 5a — Characters remaining counter (all modes)
- [x] Add a live `X / Y` counter below each text input, updating on every keystroke; turns red when `input.length >= limit` and input border turns red
- [x] Single-bus: one counter, limit = `wireCount`
- [x] Dual Combined mode: one counter, limit = `wireCount + wire1Count`
- [x] Dual Separate mode: one counter per row — Row 1 limit = `wireCount`, Row 2 limit = `wire1Count`
- [x] Multi-word: show a static per-row limit hint next to the Add button; chips in the word list turn red if the word exceeds the limit

#### 5b — Combined / Separate toggle (dual mode only)
- [x] Add a "Combined / Separate" toggle in the Custom Text UI, visible only when `isDualI2C`; defaults to Separate
- [x] `separateRows` state variable in `index.js`; initialized to `isDualI2C` on load (defaults separate in dual, irrelevant in single)
- [x] In Combined mode: existing single text input shown, behavior identical to single-bus
- [x] In Separate mode: two labeled inputs — "Row 1" and "Row 2" — replace the single input

#### 5c — Single-word mode, Separate Rows
- [x] Row 1 and Row 2 inputs each enforce their bus's module count via the red border/counter
- [x] Center Text toggle centers each row independently within its own slot count
- [x] `updateDisplay()` sends `{ mode: "dual-single", row1: "...", row2: "...", center: bool }` to `/text`
- [x] `/text` handler in `SplitFlapWebServer.cpp` detects `mode: "dual-single"`, stores row1/row2, sets firmware mode 7
- [x] New `dualSingleInputMode()` in `.ino` (case 7) reads row1/row2 from webserver, calls `display.writeStringDual(row1, row2)`

#### 5d — Multi-word mode, Separate Rows
- [x] Single word list; in Separate mode, words advance in pairs — word[i] on Row 1, word[i+1] on Row 2, index advances by 2 each cycle
- [x] Odd-length list: last cycle uses word[last] on Row 1 and blank on Row 2
- [x] Chips in the word list that exceed `wireCount` (or `wire1Count` for the paired slot) turn red as a visual warning; backend still truncates
- [x] `updateDisplay()` sends `{ mode: "dual-multiple", words: [...], delay: N, center: bool }` to `/text`
- [x] `/text` handler stores the list, sets firmware mode 8
- [x] New `dualMultiInputMode()` in `.ino` (case 8) advances index by 2, calls `display.writeStringDual(word[i], word[i+1])`

#### 5e — Backwards compatibility
- [x] Single-bus builds: all existing behavior unchanged — modes 0 and 1 (single/multi text) work exactly as before
- [x] Date and Time modes unchanged; they write flat strings via `writeString()` which auto-dispatches to `moveToDual()` already

#### Bug fixes found during testing
- [x] Centering toggle change not triggering re-draw — fixed by resetting `writtenString = ""` after each `/text` update
- [x] Flash of old content when switching modes — fixed by clearing `inputString = ""` before entering multi/dual modes and moving `writtenString = ""` to after `setMode()` per branch to eliminate async race with the loop
- [x] Flat `"row1|row2"` string appearing on display — race between async handler setting `inputString` and loop running old `singleInputMode()`; removed the `inputString` sync and relied on the empty-guard in `singleInputMode()` instead

### Step 6: Accuracy Test Mode

#### Design decisions (resolved)
- **Name:** "Accuracy Test" in the dropdown
- **All modules same char:** Yes, for both single-bus and dual-bus. `display.writeChar(c)` calls `moveTo()` which auto-dispatches to `moveToDual()` when dual I2C is active — no separate handling needed for either mode.
- **Counter reset:** Resets to 0 when the user clicks Start Test
- **Config placement:** Inline on the control page when this mode is selected
- **State split:** Firmware owns mode 9 and runs the display loop. Browser tracks displayed char and loop count locally by doing the math — same delay and stepSize on both sides keeps them in sync without polling. No `/test-state` endpoint needed.

#### 6a — Firmware state and mode (`SplitFlapWebServer.h/.cpp` + `SplitFlapDisplay.ino`) ✅
- [x] Add state to `SplitFlapWebServer`: `accuracyCharIndex`, `accuracyDelay`, `accuracyStepSize`, `lastAccuracyStepTime`; add public getters and setters; initialise in constructor
- [x] Extend `/text` handler with `mode: "accuracy"` branch: store delay (×1000 for ms) and stepSize, reset charIndex and lastStepTime, clear `inputString` (prevents singleInputMode flash during mode transition), set mode 9, reset `writtenString`
- [x] `SplitFlapModule::StandardChars` and `ExtendedChars` moved to `public` — single source of truth; `accuracyTestMode()` references them directly instead of duplicating the arrays
- [x] Add `accuracyTestMode()` in `.ino`: checks `millis() - lastAccuracyStepTime >= accuracyDelay`, writes `display.writeChar(charSet[charIndex])`, advances charIndex by stepSize, wraps at charSetSize, updates lastAccuracyStepTime
- [x] Add `case 9: accuracyTestMode(); break;` to the loop switch; add forward declaration

#### 6b — Control page UI (`index.html`) ✅
- [x] Add `<option value="9">Accuracy Test</option>` to the mode dropdown with `@change="stopAccuracyTest()"` on the select
- [x] Inline config: delay input (seconds, min 1, default 5) and step size input (flaps, min 1, default 1)
- [x] Status card (shown after Start clicked): current character in large white text, cycle counter
- [x] Button text dynamically changes to "Start Test" / "Restart Test" when mode 9 is selected

#### 6c — JS state and timer (`index.js`) ✅
- [x] State: `accuracyDelay: 5`, `accuracyStepSize: 1`, `accuracyStep: 0`, `accuracyInterval: null`
- [x] Computed getters: `accuracyCharSet` (37 or 48 chars based on `settings.charset`), `accuracyCharIndex`, `accuracyLoopCount`, `accuracyCurrentChar`
- [x] `startAccuracyTest()`: validates delay, clears old interval, resets step, POSTs to `/text`, starts `setInterval` at `accuracyDelay * 1000` ms on success
- [x] `stopAccuracyTest()`: clears interval; called on dropdown `@change`

### Step 7: Reboot Notice ✅

- [x] In `SplitFlapWebServer.cpp` POST `/settings` handler, detect changes to `wireAddresses`, `wireOffsets`, `wire1Addresses`, `wire1Offsets`, `sdaPin`, `sclPin`, `sda2Pin`, `scl2Pin` and set `rebootRequired = true` — same pattern as OTA password change
- [x] Response message: "Hardware settings changed. Rebooting to apply..."

### Step 8: MQTT Updates

- [x] Fix init order in `SplitFlapDisplay.ino`: `splitflapMqtt.setDisplay(&display)` moved before `splitflapMqtt.setup()` so `display` is non-null when the discovery payload is published
- [x] Add `"max": <numModules>` to the HA text discovery payload using `display->getNumModules()`; HA enforces the input length cap in its UI
- [x] `|`-separated payload on `topic_command`: detects `|` in the received message, splits and calls `writeStringDual(row1, row2)` on WROOM; falls back to `writeString()` on single-bus builds (`#ifdef ENABLE_DUAL_I2C` guard)
- [x] `publishState()` works for 16-module strings without changes; `writeStringDual()` already publishes `row1|row2`
- [x] Accuracy test publishes via `writeChar()` on each step — intentional, HA can track the current test character

### Step 9: Mode Cleanup



### Step 10: Random

- [ ] Update home logic to center 'ok' in dual mode
- [ ] Add config for 'fast home' which modifies to homing logic to only home, instead of home, home all and display display ok, then home the ok modules.

### Step Final: Testing

#### Misc fixes (found during testing, applied before formal test pass)
- [x] Date and Time modes now use `writeStringDual(result, "")` in dual mode — content on row 1 only, row 2 blank; single-bus behavior unchanged
- [x] New "Date + Time" mode (mode 10, WROOM only): row 1 = time format, row 2 = date format, both using existing `timeFormat`/`dateFormat` settings; shown in dropdown only when `isDualI2C`
- [x] Custom Text Update Display race condition fixed — removed redundant `/settings` POST for modes 6 and 9; `/text` handler already calls `setMode()` internally, eliminating the race where `/settings` could overwrite the firmware mode back to 6 (default: break)
- [x] Separate/Combined toggle defaults to left-position and white when in Separate (default) state
- [x] MQTT max field accounts for `|` separator in dual mode (`numModules + 1`); pipe-separated command payload routes to `writeStringDual()`

#### Still to verify
- [ ] Compile with `ENABLE_DUAL_I2C` (WROOM) and without (C3, S3) — confirm no `wire1` symbols emitted in C3/S3 binaries
- [ ] Single bus, 8 modules — verify no behavioral change vs. main
- [ ] Dual bus, 16 modules — all home, write string, report correct positions; both rows move in parallel
- [ ] Asymmetric bus counts (e.g. 3 + 5, 8 + 0, 0 + 8) — confirm `moveToDual()` handles empty-bus skip
- [ ] Settings save/load round-trip for all wire1 settings
- [ ] Settings page: Bus 2 visible on WROOM, hidden on C3/S3, validation errors show under correct field
- [ ] Address reuse: same I²C address on both buses responds independently
- [ ] OTA update on dual-I2C build — verify post-OTA boot still inits both buses


---

## File Change Summary

| File | Changes |
|------|---------|
| `platformio.ini` | Add `SDA2_PIN`, `SCL2_PIN`, `ENABLE_DUAL_I2C` to WROOM; add `SDA_PIN`/`SCL_PIN` to C3 and S3 |
| `src/SplitFlapModule.h/.cpp` | Add `TwoWire *wire` member; replace `Wire.` → `wire->` throughout |
| `src/SplitFlapDisplay.h` | Conditional `MAX_MODULES`; wire/wire1 address/offset arrays; `wire1Count` always declared; `moveModules()`, `moveToDual()`, `homeDual()`, `writeStringDual()` |
| `src/SplitFlapDisplay.cpp` | Dual-bus `init()`; `moveModules()` extracted stepping loop; `moveToDual()` FreeRTOS parallel tasks; `homeDual()`; `writeStringDual()`; auto-dispatch in `moveTo()`/`home()`; `homeToChar()` bug fix |
| `src/SplitFlapDisplay.ino` | Wire-prefixed settings; conditional wire1 settings; `sdaPin`/`sclPin` use board macros |
| `src/JsonSettings.cpp` | _(pending)_ Validate vector length parity; validate total ≤ MAX_MODULES |
| `src/web/index.js` | _(pending)_ Wire/wire1 array helpers; `isDualI2C` computed property |
| `src/web/settings.html` | _(pending)_ Two bus grids; remove `moduleCount` input; add SDA2/SCL2 inputs |
