# MQTT recovery tests

`MqttRecoveryTest.cpp` compiles the real `SplitFlapMqtt.cpp` and its real header
from unchanged CMake build-directory copies, alongside PubSubClient3 v3.3.0
(commit `a69cc5c44fffda7e07963863dcffdeb4384f0579`). The target does not use the
sketch test's fake `SplitFlapMqtt.h`.

Only the Arduino runtime, settings, display, and TCP transport are substituted.
The in-memory transport decodes outgoing MQTT packets and supplies broker
responses. The real library handles CONNECT, subscriptions, incoming commands,
keepalive detection, and callbacks. The simulated clock advances without sleeps.

## Characterization of issue #70

The initial test deliberately asserts the existing broken behavior. It passes
against the unfixed firmware and demonstrates:

1. A connection is established and a fresh clock command reaches the display.
2. Either the socket closes or a half-open session stops answering keepalives.
3. After a simulated outage and restored connectivity, two minutes of loop calls
   leave the display frozen, with no second connection attempt.
4. Availability stays retained `online`, because CONNECT registers no last will.
5. Calling `setup()` again restores delivery through the same peer and callback,
   proving the simulated transport can recover when the application asks it to.

The scenario runs both with and without MQTT credentials, for each failure mode.
The fix will intentionally break these characterization assertions; the final
test commit will replace them with recovery expectations using the same scenario.

These tests demonstrate a reproducible firmware recovery defect. They do not
prove what originally disconnected the reporter's hardware, nor emulate radio,
router, real broker, motor, or Home Assistant behavior.

Run the MQTT scenarios using the normal CMake build followed by:

```sh
ctest --test-dir build/unit-tests -C Debug -R MqttRecoveryTest --output-on-failure --no-tests=error
```
