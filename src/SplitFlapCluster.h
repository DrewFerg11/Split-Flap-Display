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

    // ---- Worker role ----------------------------------------------------
    void runWorker();
    void handleWorkerRx(const String &json);
    void sendPong();
};
