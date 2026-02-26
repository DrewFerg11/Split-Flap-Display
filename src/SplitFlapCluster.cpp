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

            long modules = doc["modules"] | -1L;
            long uptime  = doc["uptime"]  | -1L;

            if (!wasAlive) {
                CLUSTER_PRINTF("[CLUSTER] Worker %d connected  modules=%ld  uptime=%lds\n",
                               id, modules, uptime);
            } else {
                CLUSTER_PRINTF("[CLUSTER] Pong from worker %d  modules=%ld  uptime=%lds\n",
                               id, modules, uptime);
            }
        }
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
    } else {
        // Phase 2 will add: "prepare", "go", "home", "config", "status"
        CLUSTER_PRINTF("[CLUSTER] Unhandled cmd from main: '%s'\n", cmd);
    }
}

void SplitFlapCluster::sendPong() {
    JsonDocument doc;
    doc["cmd"]    = "pong";
    doc["esp"]    = clusterId;
    doc["uptime"] = (long)(millis() / 1000);

    // Report module count if display is available (set after display.init())
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
