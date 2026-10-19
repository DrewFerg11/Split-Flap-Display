# Test coverage backfill

Related issues share the `[Test coverage]` prefix. Implementation branches use
`test-coverage/`; this initial backfill is on `test-coverage/backfill`.

This backfill changes tests, test support, CI, and documentation only. Production
files under `src/` match the main baseline. Known defects are tracked in
[#82](https://github.com/DrewFerg11/Split-Flap-Display/issues/82), which remains
open independently of the coverage stories.

| Issue | Area | Real code exercised | Controlled boundary |
| --- | --- | --- | --- |
| [#73](https://github.com/DrewFerg11/Split-Flap-Display/issues/73) | Settings | `JsonSetting`, `JsonSettings`, ArduinoJson 7.4.3 | In-memory Preferences |
| [#74](https://github.com/DrewFerg11/Split-Flap-Display/issues/74) | Modules | `SplitFlapModule`, both production character arrays | Recorded I2C bytes and sensor inputs |
| [#75](https://github.com/DrewFerg11/Split-Flap-Display/issues/75) | Display | `SplitFlapDisplay`, movement engine, real modules | I2C, logical microseconds, synchronous task completion |
| [#76](https://github.com/DrewFerg11/Split-Flap-Display/issues/76) | Modes | Complete sketch, real formatters and mode handlers | Fixed wall clock and recorded display commands |
| [#77](https://github.com/DrewFerg11/Split-Flap-Display/issues/77) | HTTP commands | `SplitFlapWebServer` callbacks, real settings/JSON | Requests/responses, filesystem, WiFi, OTA |
| [#78](https://github.com/DrewFerg11/Split-Flap-Display/issues/78) | Lifecycle | Sketch `setup()`, `backgroundTick()`, provisioning callbacks | Serial bytes, provisioning API, restart recording |
| [#79](https://github.com/DrewFerg11/Split-Flap-Display/issues/79) | Web UI | Unmodified `src/web/index.js` | Alpine registration, fetch, browser timers |
| [#80](https://github.com/DrewFerg11/Split-Flap-Display/issues/80) | Release tools | Manifest generator, version injection, binary-size check | Temporary files, subprocess/environment and SCons registration |

## Run locally

Use the normal CMake/CTest commands in CONTRIBUTING.md for all C++ tests. The
component and sketch suites run in single- and dual-bus configurations. Their
names start with `single.` and `dual.`; MQTT keeps its existing independent
target and real PubSubClient3 dependency.

```sh
cmake -S tests -B build/unit-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/unit-tests --config Debug --parallel
ctest --test-dir build/unit-tests -C Debug --output-on-failure --no-tests=error --timeout 30
npm run test:web
python -m unittest discover -s tests/python -v
```

JavaScript uses Node's built-in test runner and strict assertions. A VM module
loads the original ES module, substituting only its Alpine import and browser
services; no production source rewriting or new test dependency is required.
The command uses `--experimental-vm-modules`, which Node currently requires for
that API. Each case has a fresh VM context, and asynchronous request assertions
wait for promise completion. The test does not start a real browser.

Python uses standard-library `unittest`, `unittest.mock`, and temporary directories.
The PlatformIO scripts receive a small fake build environment, so their real
functions run without git, publishing, flashing, or a PlatformIO build.

Linux/macOS GCC or Clang can additionally run address/undefined-behavior checks:

```sh
cmake -S tests -B build/unit-tests-asan -DCMAKE_BUILD_TYPE=Debug -DSPLITFLAP_SANITIZERS=ON
cmake --build build/unit-tests-asan --parallel 2
ctest --test-dir build/unit-tests-asan --output-on-failure --no-tests=error --timeout 30
```

GitHub Actions runs the native, sanitized, JavaScript, and Python suites, along
with the existing three firmware builds. There is no coverage-percentage gate.

## Coverage and known gaps

The suite checks supported behavior in the existing implementation. Settings
coverage includes typed persistence, JSON export/update, reset in the application's
`config` namespace, complete-payload address/offset validation, address bounds,
and complete pin-conflict rejection. It does not promise that every rejected
update is transactional or that partial updates are validated against saved fields.

Module/display coverage includes real character maps, ordinary phase sequencing,
logical position wrap, stop/re-energize, in-range calibration, sensor read errors,
layout, movement completion, callbacks, coil release, and dual-bus routing.
The phase after resume, out-of-range calibration, and arrival during sensor
correction have separate known defects and are excluded from this passing suite.

Sketch tests cover explicit clock tokens and configured formats, repeated-output
suppression, non-empty message cycling, odd dual-row lists, startup, provisioning,
and recovery. Default/fallback clock formatting and empty-list execution remain
known gaps. HTTP tests cover existing successful commands, common uppercase
escapes, existing invalid-mode/shape/delay rejection, method checks, network/OTA
behavior, missing-timezone-file fallback, and existing reconnect/reboot triggers.
They do not establish unchanged state for all rejected commands, safe arbitrary
payloads, or immediate application of cached motion settings.

The web suite exercises model behavior and emitted payloads with the existing
permissive settings API. It does not require stricter types or numeric form
bindings. Python tests exercise manifest offsets/chip sets, version selection,
and binary-size boundaries without publishing or flashing.

When a test exposes a defect, record a minimal reproduction and its evidence in
[#82](https://github.com/DrewFerg11/Split-Flap-Display/issues/82). Do not change
production behavior, weaken an expected result to endorse a known defect, or
leave a crashing/known-failing case in the normal suite. Create a focused bug
issue/PR from the inventory when a fix is prioritized, and add the regression
case with that fix. Earlier proposed desired-behavior cases are linked in the
inventory; their removal here does not mean the defects were resolved.

Deliberate regression checks are a local validation exercise, not a CI step that
rewrites production code. Restore the source byte-for-byte and rerun the normal
suite afterward. No hardware verification is implied by host test results.

The revised suite detected all eight deliberate changes below against otherwise
unchanged production code. All changes were restored before the final passing run.

| Story | Deliberate change | Detecting test |
| --- | --- | --- |
| #73 | Bypass complete-payload cross-field validation | `CompleteInvalidConfigurationIsRejectedBeforeWriting` |
| #74 | Change the first coil pattern | `StepsWrapPositionAndFollowCoilSequence` |
| #75 | Omit the published display state | `CentersTruncatesAndClearsWithPublishedDisplayState` |
| #76 | Convert the explicit minutes token as month | `TokenConversionDistinguishesMonthAndMinutes` |
| #77 | Decode a space escape as an underscore | `CommonUppercaseEscapesAreDecoded` |
| #78 | Restart inside the provisioning callback before its response | `SuccessfulProvisioningPersistsCredentialsAndRespondsBeforeRestart` |
| #79 | Send single mode for a dual-row command | `dual row single request preserves rows` |
| #80 | Shift manifest offsets by one | `test_factory_and_app_offsets_and_names` |

## Scope limits and future work

- Fake storage checks persistence and rejection before writing for the complete
  cross-field cases exercised. It does not establish general validation-before-write,
  thread safety, rollback after flash failure, or atomicity across power loss.
  The partial-write and concurrency problems in #54 remain separate.
- Host strings implement only the Arduino surface needed by these tests. New
  tests should verify boundary semantics before relying on additional APIs.
- The sketch double in `tests/support/SplitFlapDisplay.h` mirrors the character
  arrays in `src/SplitFlapModule.cpp`; keep these copies in sync manually.
  The module suite uses the production arrays directly. There is no automated
  comparison between the mirrors and production arrays.
- I2C tests verify bytes and logical positions, not physical motor alignment,
  power delivery, electrical timing, sensor noise, or radio reliability.
- Dual-bus task substitutes execute tasks synchronously and check routing and
  completion bookkeeping. Actual FreeRTOS concurrency still needs device tests.
- Provisioning tests cover the sketch's callbacks and sequencing, not Improv's
  packet parser or USB transport. Startup remains in the `.ino`; no application
  interface hierarchy is introduced for tests.
- Request tests exercise registered callbacks, not an HTTP socket stack. UI tests
  verify model behavior and payloads, not DOM rendering or Alpine internals.
- Timing tests use logical clocks. Simulated days/weeks are not physical soak
  tests. Keep the existing MQTT recovery scope notes in MQTT.md.

Add a small case when changing one of these behaviors. Prefer expected results
at the boundary over assertions about private helper calls or copied algorithms.
