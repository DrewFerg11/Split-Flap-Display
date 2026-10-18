# Test coverage backfill

Related issues share the `[Test coverage]` prefix. Implementation branches use
`test-coverage/`; this initial backfill is on `test-coverage/backfill`.

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

## Regression evidence

Backfilling tests exposed rejected updates saving values, unknown settings
throwing, incomplete cross-field validation for partial updates, permissive CSV
parsing, reset using a hardcoded namespace, invalid motion settings, repeated
motor phases after restart, an extra rotation after sensor correction at the
target, clock defaults rendering months instead of minutes, empty message-list
division, and invalid HTTP commands changing state. Focused fixes accompany
the tests. Cached motion settings now request a restart to apply the new values.
Percent decoding now handles each escape once and accepts either
hex case, and timezone JSON parsing passes the file length explicitly.

During implementation, deliberate regressions were tested in each issue area:
skip configuration validation, restore the old motor restart behavior, remove
sensor-arrival handling, change the default clock token, bypass decoding,
restart before provisioning responds, alter the UI dual-row payload, and
change manifest offsets. The relevant runner must fail for each mutation; all
source is restored before the final passing runs. Mutation is a validation
exercise, not a CI step that rewrites source on each run.

## Scope limits and future work

- Fake storage checks validation-before-write behavior. It does not establish
  thread safety, rollback after flash failure, or atomicity across power loss.
  The concurrency portion of #54 remains separate.
- Host strings implement only the Arduino surface needed by these tests. New
  tests should verify boundary semantics before relying on additional APIs.
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
