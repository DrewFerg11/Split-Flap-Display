# Plan: Distributed Multi-ESP Split-Flap Display

## Problem

A single ESP32 with dual I2C buses can't reliably drive 20 displays (100 modules). Testing with 25 modules shows ~3-4 RPM at 30-43% of target speed. Each additional module adds I2C overhead (step commands, mux switching, sensor reads), degrading performance for all modules. The practical limit is **10-15 modules per bus** for acceptable speed.

## Goal

Coordinate **4 ESP32s** to drive **20 displays** (100 modules) that behave as a **single logical display** to the user, MQTT, and Home Assistant.

## Hardware Layout

The number of displays per ESP is **fully configurable** — not hardcoded. Each ESP's I2C bus/mux/channel mapping is defined in its own config file, exactly like the current `wire0ChModAddrs` pattern in the `.ino`. You can add or remove modules from any ESP by editing its config.

**Default layout** (adjustable per config):

```
Main ESP (1)              Worker ESP (2)          Worker ESP (3)          Worker ESP (4)
├─ Wire0: 2-3 displays    ├─ Wire0: 2-3 displays  ├─ Wire0: 2-3 displays  ├─ Wire0: 2-3 displays
├─ Wire1: 2-3 displays    ├─ Wire1: 2-3 displays  ├─ Wire1: 2-3 displays  ├─ Wire1: 2-3 displays
├─ WiFi (web/MQTT/HA)     ├─ WiFi (OTA only)      ├─ WiFi (OTA only)      ├─ WiFi (OTA only)
├─ UART TX (broadcast)    ├─ UART RX ← main      ├─ UART RX ← main      ├─ UART RX ← main
└─ UART RX (responses)    └─ UART TX → main       └─ UART TX → main       └─ UART TX → main
```

**Example distribution** (5 displays / 25 modules per ESP):
- **Total**: 4 ESPs × ~25 modules = ~100 modules across 20 displays
- Each bus carries 10-13 modules — well within the performant range
- If Main ESP becomes a bottleneck (web + MQTT + coordination + local motors), reduce its display count and shift modules to workers

## Architecture: Main/Worker with UART Bus

### Why UART Over WiFi-Based Options

| Option | Latency | Reliability | Complexity |
|--------|---------|-------------|------------|
| **UART serial bus** | **~0.1ms** | **Wired, no drops** | **Low** |
| ESP-NOW (WiFi direct) | ~1-5ms | Good, rare drops | Medium |
| UDP broadcast | ~2-10ms | Unreliable, drops | Medium |
| MQTT relay | ~10-50ms | Depends on broker | Low but slow |
| HTTP REST | ~20-100ms | Reliable but slow | Low but slow |

**UART wins** since the ESPs are physically close. Benefits:
- Deterministic sub-millisecond latency — critical for synchronized movement
- No WiFi dependency for inter-ESP communication (works even if WiFi is down)
- Simple protocol: JSON lines over serial
- Hardware flow control available if needed
- ESP32 has 3 UART ports (UART0=USB/debug, UART1 and UART2 available)

### GPIO Availability (ESP32-WROOM)

Currently used:
- GPIO 1/3 — UART0 (USB serial debug)
- GPIO 21/22 — Wire0 (I2C bus 0)
- GPIO 32/33 — Wire1 (I2C bus 1)

Free GPIO suitable for UART: **4, 5, 13, 14, 16, 17, 18, 19, 23, 25, 26, 27** (14+ pins).

UART needs only **2 pins** (TX + RX) regardless of how many workers are connected. Even a star topology (dedicated UART per worker) would only need 6-8 pins for 3-4 workers. GPIO is **not a constraint**.

**Recommended UART pins**: GPIO 16 (RX) / GPIO 17 (TX) — these are the default UART2 pins on ESP32-WROOM, no remapping needed.

### Communication Topology

**Broadcast bus** (recommended — GO signal arrives at all workers simultaneously):

```
Main ESP
├── GPIO 17 (UART2 TX) ────┬──────────────┬──────────────→  (broadcast bus)
│                   Worker 2 RX     Worker 3 RX     Worker 4 RX
│
└── GPIO 16 (UART2 RX) ←───┴──────────────┴──────────────  (response bus)
                    Worker 2 TX     Worker 3 TX     Worker 4 TX
```

**Two-wire bus**: One line for broadcast (main TX → all workers' RX in parallel), one for responses (workers' TX → main RX, addressed token protocol — only respond when polled). Total wiring: **3 wires** (TX, RX, GND) from main to each worker.

Workers share the response line. Only one worker transmits at a time, controlled by the main polling each worker in sequence after a PREPARE. The GO signal is broadcast-only (no response needed), so all workers receive it simultaneously.

**Daisy-chain alternative**: Main→1→2→3→4, where each ESP forwards messages to the next. Saves wiring (each ESP only connects to its neighbor) but adds ~1ms latency per hop. The two-phase prepare/execute pattern mitigates this — PREPARE can tolerate a few ms of propagation since it's just setup. The GO signal adds ~2-3ms total propagation for 3 hops, which is acceptable. If wiring is easier as daisy-chain, it works fine.

### UART Electrical Considerations

All ESPs share the same 25V PSU with separate 5V buck converter branches. **UART requires a common ground reference** between all devices — this is satisfied since all 5V branches share the PSU's common ground.

At **6 feet max** wire length (main to furthest worker):
- UART at 460800 baud: No signal integrity concerns at this distance
- No RS-485 transceivers needed — direct GPIO-to-GPIO UART is fine
- If noise ever becomes an issue (unlikely at 6ft), add 100Ω series resistors on TX lines as a first step, RS-485 transceivers as a last resort

### Baud Rate

**460800 baud** (conservative for 6ft wires). At 460800 baud:
- A 200-byte command transmits in ~4ms
- Round-trip with response: ~8ms
- GO signal (20 bytes): ~0.4ms — all workers start within <1ms of each other
- Well within synchronization requirements
- Can increase to 921600 if needed, but 460800 gives margin for noise at wire length

## Role Definitions

### Main ESP Responsibilities
- **Web server** (settings UI, control page)
- **MQTT client** (Home Assistant integration)
- **mDNS** and **OTA** for the cluster
- **Command parser** — receives user text, splits across ESPs
- **Display map** — knows which displays are on which ESP
- **Sync coordinator** — two-phase command execution
- **Local motor control** — drives its own displays (count configurable, can be reduced if overloaded)

### Worker ESP Responsibilities
- **UART listener** — receives commands from main
- **Local motor control** — drives its own displays (count configurable per worker)
- **Status reporter** — sends heartbeat/errors back to main
- **OTA receiver** — main can proxy firmware updates to workers
- **Minimal WiFi** — connected for OTA only; minimal debug web page (status/errors), no full UI

### Main ESP Workload Concern

The main ESP juggles: WiFi, web server, MQTT, UART coordination, **and** local motor control. If the main becomes a bottleneck (sluggish web UI, MQTT timeouts, delayed UART responses), the fix is simple: **reduce the main's local display count** and shift those modules to a worker. In the extreme case, the main could drive zero local displays and act purely as a coordinator — though in practice 3-4 displays (15-20 modules) should be fine since the web/MQTT/UART work is lightweight and bursty while motor stepping is the sustained load.

## Synchronization Protocol

### Two-Phase Execution

To ensure all 20 displays update simultaneously:

```
Phase 1: PREPARE
  Main → Workers: {"cmd":"prepare","id":42,"displays":{"5":"HELLO","6":"WORLD",...}}
  Workers calculate target positions, pre-load everything
  Workers → Main: {"cmd":"ready","id":42,"esp":"2"}

Phase 2: EXECUTE  
  Main waits for all "ready" responses (timeout: 50ms)
  Main → Workers: {"cmd":"go","id":42}
  All ESPs (including main) start motor movement simultaneously
```

**Why this works**: The PREPARE phase handles all the slow work (string parsing, position calculation). The GO signal is a tiny packet (~20 bytes) that arrives at all workers within microseconds over UART. All ESPs begin stepping at effectively the same instant.

**Timeout handling**: If a worker doesn't respond "ready" within 50ms, main proceeds without it and logs an error. The missing worker will catch up on the next command.

### Command Format (JSON Lines over UART)

```jsonc
// Main → Workers (broadcast)
{"cmd":"prepare","id":1,"displays":{"5":"HELLO","6":"WORLD"}}
{"cmd":"go","id":1}
{"cmd":"home","id":2}
{"cmd":"config","key":"halfStepping","value":1}
{"cmd":"ping"}
{"cmd":"status"}  // Request status from specific worker

// Workers → Main (responses)
{"cmd":"ready","id":1,"esp":"2","ms":12}
{"cmd":"done","id":1,"esp":"2","ms":850}
{"cmd":"pong","esp":"2","modules":25,"uptime":3600}
{"cmd":"error","esp":"3","msg":"Module 0x22 I2C timeout"}
```

## Build-Time Configuration

### Config Folder Structure

All ESPs run **identical firmware**. The role and I2C wiring are determined by a build-time config selected via a PlatformIO build flag.

```
src/config/
├── esp_1.h      // Main ESP — cluster coordinator
├── esp_2.h      // Worker 2
├── esp_3.h      // Worker 3
└── esp_4.h      // Worker 4
```

Each config file defines `CLUSTER_ROLE`, `CLUSTER_ID`, `CLUSTER_OFFSET`, `CLUSTER_DISPLAY_COUNT`, and per-bus I2C wiring defaults:

```cpp
// src/config/esp_1.h — Main ESP
#define CLUSTER_ROLE          "main"
#define CLUSTER_ID            "1"
#define CLUSTER_OFFSET        0    // First logical display index owned by this ESP
#define CLUSTER_DISPLAY_COUNT 5    // Adjust freely

#define WIRE0_MUX_ADDRS        "112"
#define WIRE0_CH_MOD_ADDRS_112 "32,33,34,35,36;32,33,34,35,36;32,33,34,35,36;;;;;"

#define WIRE1_MUX_ADDRS        "112"
#define WIRE1_CH_MOD_ADDRS_112 "32,33,34,35,36;32,33,34,35,36;;;;;;"
```

```cpp
// src/config/esp_2.h — Worker 2
#define CLUSTER_ROLE          "worker"
#define CLUSTER_ID            "2"
#define CLUSTER_OFFSET        5    // Logical displays 5-9
#define CLUSTER_DISPLAY_COUNT 5

// WIRE0/WIRE1 defines same format, adjusted for this ESP's wiring
```

The `.ino` has `#ifndef` fallback guards for every define, so existing envs (`esp32_wroom`, etc.) continue to build unchanged.

### PlatformIO Build Environments

```ini
; platformio.ini
[env:esp_1]
extends=env:esp32_wroom
build_flags =
    ${env:esp32_wroom.build_flags}
    '-I ${PROJECT_DIR}/config'
    '-include esp_1.h'

[env:esp_2]
extends=env:esp32_wroom
build_flags =
    ${env:esp32_wroom.build_flags}
    '-I ${PROJECT_DIR}/config'
    '-include esp_2.h'

; ... esp_3, esp_4, plus _ota variants for each
```

To build and flash a specific ESP: `pio run -e esp_2 -t upload`

The `.ino` reads these defines and populates JsonSettings defaults. The existing NVS override system works unchanged — build-time values are just the defaults, NVS always wins at runtime.

## Display Mapping

The main ESP maintains a **display map** that assigns logical display indices to physical ESPs. Each ESP knows its own `DISPLAY_OFFSET` and `DISPLAY_COUNT`, so when it receives a PREPARE command, it knows which displays are its responsibility.

The main doesn't need to know the internal I2C layout of each worker — it just sends "display 7 = HELLO" and the worker maps that to its local bus/mux/channel via its own config.

```
             Logical Display Index
  0    1    2    3    4    5    6    7    8    9   ...  19
  ├── ESP 1 (main) ──────┤├── ESP 2 (worker) ─────┤     │
                                                   ├─ ESP 4 ─┤
```

When the user sends "HELLO WORLD SPLIT FLAP" via the web UI or MQTT, the main ESP:
1. Splits the text across logical display indices 0-19
2. Groups by target ESP using the display offset/count
3. Sends PREPARE to each worker with only its relevant displays
4. Waits for READY from all workers
5. Sends GO to all (and starts its own motors locally)

## Implementation Phases

### Phase 0: Build-Time Config Scaffolding
- [x] Create `src/config/` folder with `esp_1.h` through `esp_4.h`
- [x] Add `CLUSTER_ROLE`, `CLUSTER_ID`, `CLUSTER_OFFSET`, `CLUSTER_DISPLAY_COUNT` defines
- [x] Move I2C wiring defines (`WIRE0_MUX_ADDRS`, `WIRE0_CH_MOD_ADDRS_112`, etc.) into per-ESP config headers
- [x] Add PlatformIO build environments (`esp_1`–`esp_4` + OTA variants) using `-D ESP_CONFIG_FILE`
- [x] Update `.ino` to `#include ESP_CONFIG_FILE` when defined, with `#ifndef` fallback guards
- [x] Verified: `esp_1`, `esp_2`, and `esp32_wroom` all build successfully

### Phase 1: UART Communication Layer
- [x] Init UART2 on GPIO 16 (RX) / GPIO 17 (TX) at 460800 baud
- [x] Implement JSON line protocol: `sendLine(json)`, `pollLine()` with newline delimiter
- [x] Add CRC8 checksum to each JSON line for integrity at 6ft wire length
- [x] Main: broadcast on TX, poll responses on RX
- [x] Worker: listen on RX, respond on TX only when addressed
- [x] Add `ping`/`pong` heartbeat (main pings every 5s, workers respond with status + module count)
- [x] Startup handshake: main sends immediate ping on boot; tracks each worker's `lastSeenMs`
- [x] Tolerate late-joining workers (isWorkerAlive() timeout; workers respond on any subsequent ping)

### Phase 2: Command Distribution
- [x] Main: Parse incoming text and split across display map using `CLUSTER_OFFSET`/`CLUSTER_DISPLAY_COUNT`
- [x] Main: Send PREPARE with per-ESP display assignments
- [x] Worker: Receive PREPARE command, map to local bus/mux/channel, calculate target positions
- [x] Worker: Send READY response with prep time
- [x] Main: Send GO signal, execute locally simultaneously
- [x] Worker: Execute on GO signal
- [x] Main: Send HOME command (broadcast, all ESPs home their modules)

### Phase 2.5: Web UI Cluster Control
- [ ] Web server: Add `/api/displays` endpoint that returns total display count from `cluster.getTotalDisplayCount()` and per-display metadata (offset, modules)
- [ ] Web UI: Mode 7 (per-display) dynamically renders input boxes for all logical displays across the cluster — not just the main's local displays
- [ ] Web UI: Per-display submit routes through `cluster.distributeWrite()` (already wired in `.ino`, just needs the front-end to send the right count)
- [ ] Worker web UI: Minimal read-only status page showing last received text, last command timestamp, and main alive/dead status

### Phase 3: Synchronized Execution
- [ ] Implement two-phase commit with timeout (50ms for READY, proceed without stragglers)
- [ ] Measure and log sync accuracy (time between GO received and first step on each ESP)
- [ ] Add `cmd:done` reporting for movement completion tracking
- [ ] Handle partial failures (worker timeout → log error, continue, worker catches up next cycle)
- [ ] Main: Track which workers are alive and skip dead workers in PREPARE distribution

### Phase 4: Settings & Configuration Sync
- [ ] Main: Broadcast runtime config changes to workers (halfStepping, maxVel, stepSettleUs, etc.)
- [ ] Worker: Apply received settings to local NVS
- [ ] Web UI: Show cluster status panel (connected workers, module counts per ESP, last heartbeat)
- [ ] Web UI: Show per-worker error log
- [ ] Main: Forward OTA firmware updates to workers via WiFi (workers have minimal WiFi for this)

### Phase 5: Home Assistant / MQTT Integration
- [ ] Main: Aggregate total module count from all workers for HA entity
- [ ] Main: Report cluster health via MQTT (worker count, per-worker status)
- [ ] Main: Handle per-display text from HA across cluster
- [ ] Error propagation: Worker errors surface in main's MQTT state and HA sensors

## Decisions (Resolved)

1. **UART wiring → Broadcast bus.** Main TX wired in parallel to all workers' RX. Workers' TX wired to main's RX with token protocol. Only 3 wires per worker (TX, RX, GND). GPIO 16/17 on all ESPs. The ESP32-WROOM has 14+ free GPIO pins — even a star topology would fit, but broadcast is simpler and ensures the GO signal arrives at all workers simultaneously. Daisy-chain remains a fallback if wiring is easier physically.

2. **Firmware → Same firmware, build-time config.** `src/config/` holds one header per ESP. A `-D ESP_CONFIG_FILE` build flag selects it at build time; a `#include ESP_CONFIG_FILE` in the `.ino` pulls it in. Role (`main`/`worker`/`standalone`), cluster ID, display count/offset, and I2C wiring are all in the config header. Fallback `#ifndef` guards keep the existing `esp32_wroom` env working unchanged. To flash ESP 2: `pio run -e esp_2 -t upload`.

3. **Worker web UI → Minimal debug page.** Workers serve a single status page (uptime, module count, errors, last command). Full settings UI only on main.

4. **Display assignment → Fixed in config.** Each ESP's `DISPLAY_OFFSET` and `DISPLAY_COUNT` are set in its config header. Simple, predictable, no discovery protocol needed.

5. **Power sequencing → Tolerate late joins.** All ESPs share the same 25V PSU (separate 5V branches, common ground). They'll power up roughly together but the system tolerates different boot times. Main retries pings until all expected workers respond. Workers that join late sync on the next command cycle.

6. **Failure mode → Hold last display.** If main goes down, workers keep showing their last received text indefinitely. No blank screens.

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| UART noise on 6ft wires | Corrupted commands | CRC8 checksum on every JSON line; 100Ω series resistors if needed; RS-485 as last resort |
| Worker boot slower than main | Missed initial commands | Main retries pings until all workers respond; workers catch up on next command |
| WiFi reconnect storm | Multiple ESPs hammering router | Only main uses WiFi actively; workers connect minimally for OTA |
| OTA complexity | Bricking workers | Workers have fallback AP mode; main proxies OTA updates |
| Clock drift between ESPs | Desynchronized movement | GO signal is trigger-based, not time-based; no drift possible |
| Main ESP overloaded | Slow web UI, missed MQTT | Reduce main's local display count (can go down to 0 displays, pure coordinator) |
| UART response collisions | Garbled worker responses | Token protocol — main polls each worker by ID, only one responds at a time |
| Shared GND across 5V branches | Ground loops | All branches return to same PSU GND bus; keep GND wires short and direct |

## Performance Expectations

With 10-13 modules per bus (configurable per ESP):
- **Target RPM**: 8-10 RPM (vs current 3-4 RPM with 25 modules/bus)
- **Sync accuracy**: <1ms between ESPs (broadcast UART, GO signal is 20 bytes at 460800 = 0.4ms)
- **Sync accuracy (daisy-chain)**: <3ms (if daisy-chain topology chosen instead)
- **Command latency**: <15ms from user action to all ESPs starting movement (PREPARE + READY + GO)
- **Throughput**: ~250 I2C ops/sec per bus (vs current ~170 ops/sec when overloaded)
- **Wire length**: 6ft max, well within UART spec at 460800 baud

## Summary

```
User / MQTT / HA
        │
   Main ESP (1)  ─── src/config/esp_1.h
   ├── Local: N displays (configurable)
   ├── Web UI + MQTT + mDNS + OTA coordinator
   │
   UART2 Bus @ 460800 baud (GPIO 16/17, ≤6ft, common GND)
   ├── Worker 2: N displays (configurable) ─── src/config/esp_2.h
   ├── Worker 3: N displays (configurable) ─── src/config/esp_3.h
   └── Worker 4: N displays (configurable) ─── src/config/esp_4.h
   
   All ESPs: same firmware, different build-time configs
   Sync: 2-phase PREPARE/GO protocol, <1ms sync accuracy
   Total: ~20 displays, ~100 modules (all counts adjustable per config)
```
