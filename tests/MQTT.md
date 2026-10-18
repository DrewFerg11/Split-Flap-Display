# MQTT recovery tests

`MqttRecoveryTest.cpp` compiles the real `SplitFlapMqtt.cpp` and its real header
from unchanged CMake build-directory copies, alongside PubSubClient3 v3.3.0
(commit `a69cc5c44fffda7e07963863dcffdeb4384f0579`). Firmware also pins this
version in `platformio.ini`, so the protocol library under test matches the
one deployed. The target does not use the
sketch test's fake `SplitFlapMqtt.h`.

Only the Arduino runtime, settings, display, and TCP transport are substituted.
The in-memory transport decodes outgoing MQTT packets and supplies broker
responses. The real library handles CONNECT, subscriptions, incoming commands,
keepalive detection, and callbacks. The simulated clock advances without sleeps.

## Evidence for issue #70

Commit `3a46657` deliberately asserts the original broken behavior. Its four
parameterized cases pass against the unfixed firmware and demonstrate:

1. A connection is established and a fresh clock command reaches the display.
2. Either the socket closes or a half-open session stops answering keepalives.
3. After a simulated outage and restored connectivity, two minutes of loop calls
   leave the display frozen, with no second connection attempt.
4. Availability stays retained `online`, because CONNECT registers no last will.
5. Calling `setup()` again restores delivery through the same peer and callback,
   proving the simulated transport can recover when the application asks it to.

The scenario runs both with and without MQTT credentials, for each failure mode.
Fix commit `559b3ce` makes all four characterization cases fail as expected.
The final test commit replaces the freeze assertions with recovery expectations
using the same initial connection, clock command, outage, and loop sequence.

The final four recovery cases were also run against the original MQTT source
and header: all four fail. Removing automatic retry, command resubscription,
or last-will registration individually also makes the recovery tests fail.
Restoring the fixed source makes the complete suite pass again. These mutation
checks were performed during implementation; they do not modify source in CI.

## Current regression coverage

The suite contains 17 MQTT cases, plus the existing sketch entry-point test:

- Closed sockets and half-open keepalive timeouts, with and without credentials.
- Fresh clock commands delivered after automatic recovery, including a second
  outage, without calling `setup()`, changing settings, or resetting the device.
- Command resubscription and credentials restored on a new session.
- Retained `offline` last will during unexpected loss, then `online` on recovery.
- Broker unavailable at boot, followed by successful recovery.
- Five-second retry spacing, measured from the end of a slow connection attempt.
- Retry timing across the 32-bit `millis()` wraparound boundary.
- No connection attempts while Wi-Fi is down, before MQTT setup, or with an empty
  broker configuration; normal attempts resume when configured and connected.
- Settings changes ending the old session and using the new topics/credentials.
- A simulated day of 1,440 minute updates on a healthy session without needless
  reconnections. This advances logical time; it is not a real-time soak test.
- A broker refusing CONNECT (e.g. not authorized) or accepting TCP but never
  answering: retries stay paced and recovery follows once the broker answers.
- A half-open session replaced within two keepalive periods even while the
  display keeps publishing state, so outbound traffic cannot mask the loss.
- Wi-Fi loss that leaves a stale socket: no dialing until Wi-Fi returns.
- Publishing state while disconnected is harmless.
- A simulated week of minute updates with about 200 seeded outages of every
  modeled kind; the clock never stays frozen, though commands sent during an
  outage are lost because they are QoS 0.

These tests demonstrate a reproducible firmware recovery defect. They do not
prove what originally disconnected the reporter's hardware, nor emulate radio,
router, real broker, motor, or Home Assistant behavior. The in-memory peer
supports only the MQTT packet paths needed here; it is not a general broker
implementation or MQTT conformance test. HA discovery payload sizing is outside
this issue's scope. The suite uses single-bus commands; board-specific and dual-I2C
compilation are still checked by the firmware builds.

Run the MQTT scenarios using the normal CMake build followed by:

```sh
ctest --test-dir build/unit-tests -C Debug -R "Mqtt(Recovery|Session)Test" --output-on-failure --no-tests=error
```
