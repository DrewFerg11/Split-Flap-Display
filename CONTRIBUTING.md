# Contributing

Thanks for your interest! This project is a community fork of
[jhoff/Split-Flap-Display](https://github.com/jhoff/Split-Flap-Display), which
is itself based on the original design by
[Morgan Manly](https://github.com/ManlyMorgan/Split-Flap-Display).

## Reporting issues

- **Bugs:** Open a [GitHub issue](../../issues/new?template=bug_report.yml) with the board you're using,
  what you expected, what actually happened, and (if relevant) the serial log.
- **Feature ideas:** Issues are fine, but if it's open-ended, the
  [Discord](https://discord.gg/RCvks4XXXH) is usually a better place to discuss
  first.
- **Questions / help:** Try the Discord — issues are best reserved for things
  that are actionable in code.

## Pull requests

1. Fork the repo and create a branch off `main` with a descriptive name.
2. Make your change. Keep it focused — one logical change per PR.
3. Run the formatter before committing:
   ```
   npm run format
   ```
   The `format-check` workflow will fail your PR if this hasn't been run.
4. Push to your fork and open a PR against `main` here.
5. Be patient — this is a side project; reviews may take a few days.

For firmware build setup, see the [manual install guide](docs/firmware/install-manual.md).

## C++ unit tests

The [coverage map](tests/COVERAGE.md) describes the coordinated backfill issues,
the real code each suite exercises, regression evidence, and simulation limits.
It also documents `npm run test:web`, the Python release-tool tests, and the
optional Linux/macOS sanitizer build.

Host tests use GoogleTest, CMake, and CTest. They run on your computer without
an ESP32, MQTT broker, or network services. The first CMake configuration needs
internet access to download the pinned GoogleTest dependency. PlatformIO still
builds the firmware; the host test build is separate.

### Prerequisites and commands

Install CMake 3.24 or newer and a C++17 compiler with its build tools:

- Ubuntu/Debian: `sudo apt install build-essential cmake`.
- macOS: install the Xcode Command Line Tools (`xcode-select --install`) and
  CMake (for example, `brew install cmake`).
- Windows: install Visual Studio Build Tools with the **Desktop development
  with C++** workload and CMake. Run from a Developer PowerShell. CMake can use
  the Visual Studio generator; MinGW with a suitable CMake generator also works.

From the repository root, on all three platforms:

```sh
cmake -S tests -B build/unit-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build/unit-tests --config Debug --parallel
ctest --test-dir build/unit-tests -C Debug --output-on-failure --no-tests=error
```

`--config Debug` / `-C Debug` also support Visual Studio's multi-configuration
generator. `CMAKE_BUILD_TYPE` applies to single-configuration generators.

List discovered tests or run just the entry-point test:

```sh
ctest --test-dir build/unit-tests -C Debug -N
ctest --test-dir build/unit-tests -C Debug -R AppEntryPointTest --output-on-failure --no-tests=error
```

### Initial coverage and adding tests

There is one initial test, `IdleLoopStillServicesMqttAndWebMaintenance`, in
`tests/unit/AppEntryPointTest.cpp`. It invokes the actual firmware `loop()`
from `src/SplitFlapDisplay.ino`, with the display in idle mode, and checks that
MQTT, OTA, and reboot maintenance are serviced once, no display write occurs,
and the loop yields. The sketch stays intact, with no production test hooks.

CMake uses `configure_file(COPYONLY)` to mirror the complete, unmodified `.ino`
into the ignored test build directory and compiles it as C++. The copy is
refreshed automatically when the original changes. This changes the location
used to resolve quoted includes, allowing `tests/support/` to supply hardware
and application dependency substitutes instead of adjacent production headers.
`SketchDeclarations.h` supplies forward declarations that Arduino normally
generates. No source parsing, extraction, or rewriting is used.

The substitutes implement just enough of the dependencies to compile the
sketch and run the idle-loop scenario. They are not general Arduino emulation
or replacements for testing the real application classes. Sketch functions
such as `backgroundTick()` and `checkConnection()` are compiled from the `.ino`
too; the test provides no serial input, a fixed clock, and no reconnect request.
PlatformIO uses the real headers and never includes `tests/support/`. This test
does not establish coverage of `setup()`, other display modes, actual MQTT
reconnection, or physical hardware. The native sketch target uses the single-bus
configuration; the three firmware builds still check board-specific compilation.

The [MQTT regression suite](tests/MQTT.md) separately compiles the real
`SplitFlapMqtt` class and PubSubClient3, with an in-memory broker peer and a
simulated clock. It covers connection-loss recovery and retry timing without
replacing the MQTT implementation under test. See that page for the original
bug characterization, regression checks, and limits of the simulation.

To grow coverage:

1. Add a descriptive `*Test.cpp` under `tests/unit/` with GoogleTest `TEST` or
   `TEST_F` cases named for observable behavior.
2. Register the source and relevant production files in `tests/CMakeLists.txt`.
   For an independent component, add a separate executable, link
   `GTest::gtest_main`, and register it with `gtest_discover_tests`. Keep the
   sketch substitutes private to their targets; other tests should use
   real production headers whenever possible.
3. Keep tests deterministic: use controlled inputs or fakes instead of real
   hardware, network connections, elapsed-time sleeps, or copied production
   implementations. Add only the test seams the behavior under test needs.
   Google Mock can be enabled later when a specific test benefits from it.
4. Run the tests and format changed C++ files with the repository's
   `.clang-format`. `npm run format:cpp` covers `src/` and `tests/` in a POSIX
   shell with clang-format available. In PowerShell, run
   `clang-format -i path/to/ChangedTest.cpp` for each changed C++ file.

GitHub Actions runs CTest on pull requests and pushes to `main`; failed
assertions or zero discovered tests fail the unit-test job. The existing board
builds and C++ formatting checks remain in place. Making **C++ unit tests** a
required merge check is configured separately in GitHub's branch rules.

## Native emulator check

**CI / Native emulator** builds each PR's firmware source on Linux against
[Split-Flap-Emulator](https://github.com/DrewFerg11/Split-Flap-Emulator)'s
host shims, then runs five scenarios: 8- and 16-module release smoke, MQTT/Home
Assistant, time mode across a DST change, and date mode across New Year. The
job summary lists each result. Reports, serial logs and replays are uploaded
as the `native-emulator-evidence` artifact.

The job is **not** a merge requirement. Released v1.4.1 fails it on
[#54](https://github.com/DrewFerg11/Split-Flap-Display/issues/54): lost
settings in the smoke scenarios, and `NOT_FOUND` errors for unset keys in the
rest (see also [#84](https://github.com/DrewFerg11/Split-Flap-Display/issues/84)).
A fix for those should turn the scenarios green. `EMULATOR_REF` in the
workflow pins the emulator version; bump it on purpose.

## A note on licensing

Neither this fork nor its upstream currently has an explicit software license,
which means the firmware code is — strictly — under default copyright by its
original authors. By opening a PR you agree that your contribution may later be
relicensed under whatever permissive open-source license the project eventually
adopts (most likely MIT). If that's a dealbreaker for you, please raise it in
an issue first.

The 3D model files are covered separately by
[LICENSE-HARDWARE.md](LICENSE-HARDWARE.md) (CC BY-NC-SA 4.0).
