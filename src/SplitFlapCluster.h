#pragma once

#include "JsonSettings.h"

#include <Arduino.h>
#include <ArduinoJson.h>


class SplitFlapDisplay;

class SplitFlapCluster {
  public:
    SplitFlapCluster(JsonSettings &settings);

    // Call after webServer.init() (settings loaded from NVS by then)
    void begin();

    // Call every loop() iteration
    void loop();

    // Provide display reference so workers can report module count in pong
    // (call after display.init())
    void setDisplay(SplitFlapDisplay *d) { display = d; }

    // Role helpers
    bool isMain()       const { return role == "main"; }
    bool isWorker()     const { return role == "worker"; }
    bool isStandalone() const { return role == "standalone"; }

    // --- Main-side status ---
    // Has worker workerId (1-4) sent a pong within the timeout window?
    bool isWorkerAlive(uint8_t workerId) const;
    int  aliveWorkerCount()              const;

    // --- Worker-side status ---
    // Has the main sent a ping within the timeout window?
    bool isMainAlive() const;

    // ---- Cluster command distribution ----------------------------------
    // Called from .ino to write text across the whole cluster.
    // texts[i] = text for logical display index i (0-based global).
    // numTexts  = number of elements in texts[] to send (typically 8).
    // Returns true when the call handled execution (always for standalone/main).
    // Workers must NOT call this — they execute via PREPARE/GO from the main.
    bool distributeWrite(String texts[], int numTexts,
                         float speed = 0.0f, bool centering = true);

    // Broadcast HOME to all workers and execute homeAllChannels locally.
    // Call from .ino instead of display.homeAllChannels() in cluster setups.
    // speed = 0.0 resolves to settings "maxVel" at call time.
    void distributeHome(float speed = 0.0f, bool quickHome = true);

    // Total logical displays across the cluster (main's clusterDisplayCount
    // plus each alive worker's reported displayCount). Standalone returns
    // the local physical display count.
    int getTotalDisplayCount() const;

  private:
    JsonSettings     &settings;
    SplitFlapDisplay *display = nullptr;

    String role;       // "standalone" | "main" | "worker"
    String clusterId;  // "1" | "2" | ...

    // Incoming bytes accumulate here until '\n'
    String lineBuffer;

    unsigned long lastPingSentMs;
    unsigned long *workerLastSeenMs = nullptr;  // allocated in begin(); index 1..maxWorkers
    unsigned long mainLastSeenMs;
    int maxWorkers = 0;  // read from JsonSettings "clusterMaxWorkers" in begin()

    // ---- Main-side command sequencing ----------------------------------
    int  nextCmdId = 0;                   // monotonically increasing command counter
    bool workerReadyReceived[5] = {};     // [1..maxWorkers] set by handleMainRx on "ready"

    // Per-worker routing populated from pong ("offset" / "displays" fields)
    int workerOffset[5]       = {};       // logical display start index for each worker
    int workerDisplayCount[5] = {};       // number of logical displays each worker owns

    // ---- Worker-side pending-command state ------------------------------
    String pendingTexts[8];               // local display texts received in last PREPARE
    int    pendingNumTexts = 0;
    float  pendingSpeed    = 0.0f;        // 0 = unset; overwritten by PREPARE before executeGo runs
    bool   pendingCentering = true;
    int    pendingCmdId    = -1;

    // ---- Protocol -------------------------------------------------------
    // Transmit: serialises doc, appends "|<CRC8-hex>\n"
    void sendLine(const JsonDocument &doc);

    // Returns true when a complete line (up to '\n') is ready in `out`
    bool pollLine(String &out);

    // Verifies the trailing "|XX" CRC suffix, strips it, leaves bare JSON.
    // Returns false (and logs) on mismatch or missing CRC.
    bool verifyAndStrip(String &line);

    // CRC-8/SMBUS (poly 0x07, init 0x00, no reflection)
    uint8_t crc8(const uint8_t *data, size_t len);

    // ---- Main role ------------------------------------------------------
    void runMain();
    void broadcastPing();
    void handleMainRx(const String &json);

    // Main-side helpers
    void sendPrepare(String texts[], int numTexts, float speed, bool centering, int cmdId);
    void sendGo(int cmdId);
    void sendHomeCmd(float speed, bool quickHome);
    // Poll UART until all alive workers send READY or timeout expires.
    // Returns true if all alive workers responded in time.
    bool waitForReady(int cmdId, unsigned long timeoutMs);

    // ---- Worker role ----------------------------------------------------
    void runWorker();
    void handleWorkerRx(const String &json);
    void sendPong();

    // Worker-side helpers
    void executeGo();              // call display->writeDisplays with pending state
    void sendReady(int cmdId, unsigned long prepMs);
    void sendDone(int cmdId);
};
