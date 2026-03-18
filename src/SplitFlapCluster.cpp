#include "SplitFlapCluster.h"
#include "SplitFlapDisplay.h"

SplitFlapCluster::SplitFlapCluster(JsonSettings &settings)
    : settings(settings), lastPingSentMs(0), mainLastSeenMs(0) {
}

void SplitFlapCluster::begin() {
    role      = settings.getString("clusterRole");
    clusterId = settings.getString("clusterId");
    maxWorkers = settings.getInt("clusterMaxWorkers");
    workerLastSeenMs = new unsigned long[maxWorkers + 1]();  // zero-initialised; index 1..maxWorkers

    if (role == "standalone") {
        CLUSTER_PRINTLN("[CLUSTER] Standalone mode — UART disabled");
        return;
    }

    // UART2 — pins and baud come from settings so they can be changed without recompiling.
    // Note: both ends must use the same values or communication will fail.
    int baud  = settings.getInt("clusterUartBaud");
    int rxPin = settings.getInt("clusterUartRxPin");
    int txPin = settings.getInt("clusterUartTxPin");
    Serial2.begin(baud, SERIAL_8N1, rxPin, txPin);

    CLUSTER_PRINTF("[CLUSTER] UART2 @ %d baud  RX=GPIO%d  TX=GPIO%d  role=%s  id=%s\n",
                   baud, rxPin, txPin, role.c_str(), clusterId.c_str());

    if (isMain()) {
        CLUSTER_PRINTF("[CLUSTER] Main — pinging workers every %dms\n",
                       settings.getInt("clusterPingIntervalMs"));
        broadcastPing();  // immediate first ping so workers know main is up
    } else {
        CLUSTER_PRINTLN("[CLUSTER] Worker — listening for main");
    }
}

void SplitFlapCluster::loop() {
    if (role == "standalone") return;
    if (isMain())   runMain();
    else            runWorker();
}

// ---------------------------------------------------------------------------
// Main-side logic
// ---------------------------------------------------------------------------

void SplitFlapCluster::runMain() {
    // Periodic ping
    if (millis() - lastPingSentMs >= (unsigned long)settings.getInt("clusterPingIntervalMs")) {
        broadcastPing();
    }

    // Drain incoming lines (pong responses from workers)
    String line;
    while (pollLine(line)) {
        if (!verifyAndStrip(line)) continue;
        handleMainRx(line);
    }
}

void SplitFlapCluster::broadcastPing() {
    JsonDocument doc;
    doc["cmd"] = "ping";
    doc["esp"] = clusterId;
    sendLine(doc);
    lastPingSentMs = millis();
    CLUSTER_PRINTF("[CLUSTER] Ping broadcast — %d/%d workers alive\n",
                   aliveWorkerCount(), maxWorkers);
}

void SplitFlapCluster::handleMainRx(const String &json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        CLUSTER_PRINTF("[CLUSTER] RX JSON parse error: %s  raw='%s'\n",
                       err.c_str(), json.c_str());
        return;
    }

    const char *cmd   = doc["cmd"];
    const char *espId = doc["esp"];
    if (!cmd || !espId) return;

    if (strcmp(cmd, "pong") == 0) {
        int id = atoi(espId);
        if (id >= 1 && id <= maxWorkers) {
            bool wasAlive = isWorkerAlive(id);
            workerLastSeenMs[id] = millis();

            long modules  = doc["modules"]  | -1L;
            long uptime   = doc["uptime"]   | -1L;
            int  offset   = doc["offset"]   | 0;
            int  displays = doc["displays"] | 0;

            // Store routing info for PREPARE distribution
            workerOffset[id]       = offset;
            workerDisplayCount[id] = displays;

            if (!wasAlive) {
                CLUSTER_PRINTF("[CLUSTER] Worker %d connected  modules=%ld  uptime=%lds  offset=%d  displays=%d\n",
                               id, modules, uptime, offset, displays);
            } else {
                CLUSTER_PRINTF("[CLUSTER] Pong from worker %d  modules=%ld  uptime=%lds  offset=%d  displays=%d\n",
                               id, modules, uptime, offset, displays);
            }
        }
    } else if (strcmp(cmd, "ready") == 0) {
        int id    = atoi(espId);
        int cmdId = doc["id"] | -1;
        if (id >= 1 && id <= maxWorkers && cmdId >= 0) {
            workerReadyReceived[id] = true;
            long prepMs = doc["ms"] | 0L;
            CLUSTER_PRINTF("[CLUSTER] READY from worker %d  cmd=%d  prep=%ldms\n",
                           id, cmdId, prepMs);
        }
    } else if (strcmp(cmd, "done") == 0) {
        int id    = atoi(espId);
        int cmdId = doc["id"] | -1;
        CLUSTER_PRINTF("[CLUSTER] DONE from worker %d  cmd=%d\n", id, cmdId);
    } else {
        CLUSTER_PRINTF("[CLUSTER] Unknown cmd from ESP %s: '%s'\n", espId, cmd);
    }
}

bool SplitFlapCluster::isWorkerAlive(uint8_t id) const {
    if (workerLastSeenMs == nullptr) return false;
    if (id < 1 || id > maxWorkers)   return false;
    if (workerLastSeenMs[id] == 0)   return false;
    return (millis() - workerLastSeenMs[id]) < (unsigned long)settings.getInt("clusterWorkerTimeoutMs");
}

int SplitFlapCluster::aliveWorkerCount() const {
    int n = 0;
    for (int i = 1; i <= maxWorkers; i++) {
        if (isWorkerAlive(i)) n++;
    }
    return n;
}

// ---------------------------------------------------------------------------
// Worker-side logic
// ---------------------------------------------------------------------------

void SplitFlapCluster::runWorker() {
    String line;
    while (pollLine(line)) {
        if (!verifyAndStrip(line)) continue;
        handleWorkerRx(line);
    }
}

void SplitFlapCluster::handleWorkerRx(const String &json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        CLUSTER_PRINTF("[CLUSTER] RX JSON parse error: %s  raw='%s'\n",
                       err.c_str(), json.c_str());
        return;
    }

    const char *cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "ping") == 0) {
        mainLastSeenMs = millis();
        CLUSTER_PRINTLN("[CLUSTER] Ping from main → sending pong");
        sendPong();

    } else if (strcmp(cmd, "prepare") == 0) {
        unsigned long t0 = millis();
        int cmdId       = doc["id"]     | -1;
        float speed     = doc["speed"]  | settings.getFloat("maxVel");
        bool centering  = doc["center"] | true;

        int myOffset = settings.getInt("clusterOffset");
        int myCount  = settings.getInt("clusterDisplayCount");

        // Extract the slice of texts that belongs to this worker
        pendingNumTexts = 0;
        for (int i = 0; i < myCount && i < 8; i++) {
            String key = String(myOffset + i);
            const char *text = doc["displays"][key];
            pendingTexts[i] = text ? String(text) : String("");
            pendingNumTexts++;
        }
        pendingSpeed    = speed;
        pendingCentering = centering;
        pendingCmdId    = cmdId;

        unsigned long prepMs = millis() - t0;
        CLUSTER_PRINTF("[CLUSTER] PREPARE cmd=%d  offset=%d  count=%d  prep=%lums\n",
                       cmdId, myOffset, pendingNumTexts, prepMs);
        sendReady(cmdId, prepMs);

    } else if (strcmp(cmd, "go") == 0) {
        int cmdId = doc["id"] | -1;
        CLUSTER_PRINTF("[CLUSTER] GO received  cmd=%d  pending=%d\n", cmdId, pendingCmdId);
        if (cmdId == pendingCmdId && pendingCmdId >= 0) {
            executeGo();
        } else {
            CLUSTER_PRINTF("[CLUSTER] GO id mismatch — ignoring (got %d, have %d)\n",
                           cmdId, pendingCmdId);
        }

    } else if (strcmp(cmd, "home") == 0) {
        float speed  = doc["speed"] | settings.getFloat("maxVel");
        bool quick   = doc["quick"] | (settings.getInt("quickHome") != 0);
        CLUSTER_PRINTF("[CLUSTER] HOME received  speed=%.1f  quick=%d\n", speed, quick);
        if (display != nullptr) {
            display->homeAllChannels(speed, quick);
        }
        sendDone(-1);

    } else {
        // Future: "config", "status" commands can be added here
        CLUSTER_PRINTF("[CLUSTER] Unhandled cmd from main: '%s'\n", cmd);
    }
}

void SplitFlapCluster::sendPong() {
    JsonDocument doc;
    doc["cmd"]      = "pong";
    doc["esp"]      = clusterId;
    doc["uptime"]   = (long)(millis() / 1000);
    doc["offset"]   = settings.getInt("clusterOffset");
    doc["displays"] = settings.getInt("clusterDisplayCount");

    if (display != nullptr) {
        doc["modules"] = display->getNumModules();
    }

    sendLine(doc);
}

bool SplitFlapCluster::isMainAlive() const {
    if (mainLastSeenMs == 0) return false;
    return (millis() - mainLastSeenMs) < (unsigned long)settings.getInt("clusterWorkerTimeoutMs");
}

// ---------------------------------------------------------------------------
// Public API (main + standalone)
// ---------------------------------------------------------------------------

bool SplitFlapCluster::distributeWrite(String texts[], int numTexts,
                                       float speed, bool centering) {
    // Workers must not call this — they execute via PREPARE/GO
    if (isWorker()) return false;

    // 0.0 means "use configured maxVel" — resolved here so callers don't need
    // to know the setting name
    if (speed <= 0.0f) speed = settings.getFloat("maxVel");

    if (isMain()) {
        int cmdId = ++nextCmdId;

        // Reset READY flags for all workers before sending PREPARE
        for (int i = 1; i <= maxWorkers; i++) {
            workerReadyReceived[i] = false;
        }

        sendPrepare(texts, numTexts, speed, centering, cmdId);
        waitForReady(cmdId, 200);   // 200 ms — generous for <6 ft UART at 460800
        sendGo(cmdId);
        // ↑ GO is now in transit; execute locally with minimal delay below
    }

    // Execute the local slice (both standalone and main)
    if (display != nullptr) {
        int offset   = settings.getInt("clusterOffset");  // 0 for standalone
        int numLocal = display->getNumDisplays();

        // writeDisplays() iterates numDisplays entries, so always pass 8 slots
        String localTexts[8] = {};
        for (int i = 0; i < 8; i++) {
            int globalIdx = offset + i;
            localTexts[i] = (globalIdx < numTexts) ? texts[globalIdx] : String("");
        }
        display->writeDisplays(localTexts, speed, centering);
    }

    return true;
}

void SplitFlapCluster::distributeHome(float speed, bool quickHome) {
    if (speed <= 0.0f) speed = settings.getFloat("maxVel");

    if (isMain()) {
        sendHomeCmd(speed, quickHome);
        // Workers receive and execute HOME; main executes locally below
    }

    // Always execute locally (standalone, main, and workers that boot independently)
    if (display != nullptr) {
        display->homeAllChannels(speed, quickHome);
    }
}

int SplitFlapCluster::getTotalDisplayCount() const {
    if (!isMain()) {
        // Standalone: return local physical display count
        return (display != nullptr) ? display->getNumDisplays() : 0;
    }

    // Main: own count + each alive worker's reported count
    int total = settings.getInt("clusterDisplayCount");
    for (int i = 1; i <= maxWorkers; i++) {
        if (isWorkerAlive(i)) {
            total += workerDisplayCount[i];
        }
    }
    return total;
}

// ---------------------------------------------------------------------------
// Main-side protocol helpers
// ---------------------------------------------------------------------------

void SplitFlapCluster::sendPrepare(String texts[], int numTexts,
                                   float speed, bool centering, int cmdId) {
    JsonDocument doc;
    doc["cmd"]    = "prepare";
    doc["id"]     = cmdId;
    doc["speed"]  = speed;
    doc["center"] = centering;

    JsonObject displays = doc["displays"].to<JsonObject>();
    for (int i = 0; i < numTexts; i++) {
        displays[String(i)] = texts[i];
    }

    sendLine(doc);
    CLUSTER_PRINTF("[CLUSTER] PREPARE sent  cmd=%d  displays=%d  speed=%.1f\n",
                   cmdId, numTexts, speed);
}

void SplitFlapCluster::sendGo(int cmdId) {
    JsonDocument doc;
    doc["cmd"] = "go";
    doc["id"]  = cmdId;
    sendLine(doc);
    CLUSTER_PRINTF("[CLUSTER] GO sent  cmd=%d\n", cmdId);
}

void SplitFlapCluster::sendHomeCmd(float speed, bool quickHome) {
    JsonDocument doc;
    doc["cmd"]   = "home";
    doc["speed"] = speed;
    doc["quick"] = quickHome;
    sendLine(doc);
    CLUSTER_PRINTF("[CLUSTER] HOME broadcast  speed=%.1f  quick=%d\n", speed, quickHome);
}

bool SplitFlapCluster::waitForReady(int cmdId, unsigned long timeoutMs) {
    unsigned long deadline = millis() + timeoutMs;

    while (millis() < deadline) {
        // Drain any incoming bytes looking for READY responses
        String line;
        while (pollLine(line)) {
            if (!verifyAndStrip(line)) continue;
            handleMainRx(line);
        }

        // Check if every alive worker has responded
        bool allReady = true;
        for (int i = 1; i <= maxWorkers; i++) {
            if (isWorkerAlive(i) && !workerReadyReceived[i]) {
                allReady = false;
                break;
            }
        }
        if (allReady) {
            CLUSTER_PRINTF("[CLUSTER] All alive workers READY for cmd %d\n", cmdId);
            return true;
        }

        delay(1);
    }

    // Timeout — log which workers did not respond
    for (int i = 1; i <= maxWorkers; i++) {
        if (isWorkerAlive(i) && !workerReadyReceived[i]) {
            CLUSTER_PRINTF("[CLUSTER] Timeout waiting for READY from worker %d (cmd %d)\n",
                           i, cmdId);
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Worker-side protocol helpers
// ---------------------------------------------------------------------------

void SplitFlapCluster::executeGo() {
    if (display == nullptr) {
        sendDone(pendingCmdId);
        return;
    }

    unsigned long t0 = millis();

    // writeDisplays() expects at least numDisplays entries; pad to 8
    String textsForDisplay[8] = {};
    for (int i = 0; i < 8; i++) {
        textsForDisplay[i] = (i < pendingNumTexts) ? pendingTexts[i] : String("");
    }

    CLUSTER_PRINTF("[CLUSTER] Executing GO  cmd=%d  numTexts=%d  speed=%.1f\n",
                   pendingCmdId, pendingNumTexts, pendingSpeed);

    display->writeDisplays(textsForDisplay, pendingSpeed, pendingCentering);

    unsigned long elapsed = millis() - t0;
    CLUSTER_PRINTF("[CLUSTER] GO complete  cmd=%d  elapsed=%lums\n", pendingCmdId, elapsed);

    // Record last executed command for worker status page
    lastExecutedMs    = millis();
    lastExecutedCount = min(pendingNumTexts, 8);
    for (int i = 0; i < lastExecutedCount; i++) {
        lastExecutedTexts[i] = pendingTexts[i];
    }

    sendDone(pendingCmdId);
    pendingCmdId = -1;  // clear so a stale GO cannot re-trigger
}

void SplitFlapCluster::sendReady(int cmdId, unsigned long prepMs) {
    JsonDocument doc;
    doc["cmd"] = "ready";
    doc["id"]  = cmdId;
    doc["esp"] = clusterId;
    doc["ms"]  = (long)prepMs;
    sendLine(doc);
    CLUSTER_PRINTF("[CLUSTER] READY sent  cmd=%d  prep=%lums\n", cmdId, prepMs);
}

void SplitFlapCluster::sendDone(int cmdId) {
    JsonDocument doc;
    doc["cmd"] = "done";
    doc["id"]  = cmdId;
    doc["esp"] = clusterId;
    sendLine(doc);
    CLUSTER_PRINTF("[CLUSTER] DONE sent  cmd=%d\n", cmdId);
}

// ---------------------------------------------------------------------------
// Protocol helpers
// ---------------------------------------------------------------------------

void SplitFlapCluster::sendLine(const JsonDocument &doc) {
    String json;
    serializeJson(doc, json);

    uint8_t crc = crc8(reinterpret_cast<const uint8_t *>(json.c_str()), json.length());
    char suffix[6];
    snprintf(suffix, sizeof(suffix), "|%02X\n", crc);

    Serial2.print(json);
    Serial2.print(suffix);
    Serial2.flush();
}

bool SplitFlapCluster::pollLine(String &out) {
    while (Serial2.available()) {
        char c = static_cast<char>(Serial2.read());

        if (c == '\r') continue;  // ignore CR in CRLF

        if (c == '\n') {
            if (lineBuffer.length() == 0) continue;  // skip blank lines
            out        = lineBuffer;
            lineBuffer = "";
            return true;
        }

        lineBuffer += c;

        // Guard against unbounded growth (malformed/noisy line)
        if (lineBuffer.length() > 512) {
            CLUSTER_PRINTLN("[CLUSTER] Line buffer overflow — discarding");
            lineBuffer = "";
        }
    }
    return false;
}

bool SplitFlapCluster::verifyAndStrip(String &line) {
    int pipeIdx = line.lastIndexOf('|');
    if (pipeIdx < 0 || pipeIdx + 2 > static_cast<int>(line.length())) {
        CLUSTER_PRINTF("[CLUSTER] Malformed line (no CRC suffix): '%s'\n", line.c_str());
        return false;
    }

    String crcStr  = line.substring(pipeIdx + 1);
    String jsonStr = line.substring(0, pipeIdx);

    uint8_t expected = crc8(reinterpret_cast<const uint8_t *>(jsonStr.c_str()),
                            jsonStr.length());
    uint8_t received = static_cast<uint8_t>(strtol(crcStr.c_str(), nullptr, 16));

    if (expected != received) {
        CLUSTER_PRINTF("[CLUSTER] CRC mismatch: got %02X expected %02X  json='%s'\n",
                       received, expected, jsonStr.c_str());
        return false;
    }

    line = jsonStr;
    return true;
}

uint8_t SplitFlapCluster::crc8(const uint8_t *data, size_t len) {
    // CRC-8/SMBUS: poly=0x07, init=0x00, no input/output reflection
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : (crc << 1);
        }
    }
    return crc;
}
