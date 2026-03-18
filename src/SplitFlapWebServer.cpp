#include "SplitFlapWebServer.h"
#include "SplitFlapCluster.h"
#include "SplitFlapDisplay.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>

#define AP_SSID "Split Flap Display"

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

SplitFlapWebServer::SplitFlapWebServer(JsonSettings &settings)
    : settings(settings), server(80), multiWordDelay(1000), rebootRequired(false), attemptReconnect(false),
      multiWordCurrentIndex(0), numMultiWords(0), wifiCheckInterval(1000), connectionMode(0), checkDateInterval(250),
      centering(1), display(nullptr), cluster(nullptr), displayTextsUpdated(false), displayCentering(true) {
    lastSwitchMultiTime = millis();
    currentMode = settings.getInt("mode");  // Load mode from settings on startup
    for (int i = 0; i < MAX_DISPLAY_TEXTS; i++) {
        displayTexts[i] = "";
    }
}

void SplitFlapWebServer::setDisplay(SplitFlapDisplay *displayPtr) {
    display = displayPtr;
}

void SplitFlapWebServer::setCluster(SplitFlapCluster *clusterPtr) {
    cluster = clusterPtr;
}

void SplitFlapWebServer::init() {
    if (! LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    setTimezone();
}

void SplitFlapWebServer::setTimezone() {
    const char *sntpServer = "pool.ntp.org";
    const char *defaultTz = "UTC0";
    String timezoneSetting = settings.getString("timezone");
    String posixTimezone = defaultTz;

    File file = LittleFS.open("/timezones.json", "r");
    if (! file) {
        Serial.println("Failed to open timezones.json; defaulting to UTC");
        configTzTime(defaultTz, sntpServer);
        return;
    }

    size_t size = file.size();
    std::unique_ptr<char[]> buffer(new char[size]);
    file.readBytes(buffer.get(), size);
    file.close();

    JsonDocument timezones;
    DeserializationError error = deserializeJson(timezones, buffer.get());

    if (error) {
        Serial.println("Failed to parse timezones.json: " + String(error.c_str()));
        configTzTime(defaultTz, sntpServer);
        return;
    }

    for (JsonPair kv : timezones.as<JsonObject>()) {
        String keyStr = kv.key().c_str();
        String valueStr = kv.value().as<String>();

        if (keyStr == timezoneSetting) {
            posixTimezone = valueStr;
            break;
        }
    }

    Serial.println("POSIX Timezone set to: " + posixTimezone);
    configTzTime(posixTimezone.c_str(), sntpServer);
}

// Totally didn't use AI to make these functions
//  Function to get current minute as a string
String SplitFlapWebServer::getCurrentMinute() {
    struct tm timeinfo;
    if (! getLocalTime(&timeinfo)) {
        return "";
    }
    char minuteStr[3];                           // Max "59" + null terminator
    sprintf(minuteStr, "%02d", timeinfo.tm_min); // Format as two-digit string
    return String(minuteStr);
}

// Function to get current hour as a string
String SplitFlapWebServer::getCurrentHour() {
    struct tm timeinfo;
    if (! getLocalTime(&timeinfo)) {
        return "";
    }
    char hourStr[3];                            // Max "59" + null terminator
    sprintf(hourStr, "%02d", timeinfo.tm_hour); // Format as two-digit string
    return String(hourStr);
}

// Function to get the first n characters of the day
String SplitFlapWebServer::getDayPrefix(int n) {
    struct tm timeinfo;
    if (! getLocalTime(&timeinfo)) {
        return "Err"; // Return error if time not available
    }

    // Get full weekday name
    char fullDay[10]; // Buffer for full day name
    strftime(fullDay, sizeof(fullDay), "%A", &timeinfo);

    // Extract first n characters
    char dayPrefix[n + 1];
    strncpy(dayPrefix, fullDay, n);
    dayPrefix[n] = '\0'; // Null-terminate the string

    return String(dayPrefix);
}

// Function to get the first n characters of the month
String SplitFlapWebServer::getMonthPrefix(int n) {
    struct tm timeinfo;
    if (! getLocalTime(&timeinfo)) {
        return "Err"; // Return error if time not available
    }

    // Get full month name
    char fullMonth[10]; // Buffer for full month name
    strftime(fullMonth, sizeof(fullMonth), "%B", &timeinfo);

    // Extract first n characters
    char monthPrefix[n + 1];
    strncpy(monthPrefix, fullMonth, n);
    monthPrefix[n] = '\0'; // Null-terminate the string

    return String(monthPrefix);
}

String SplitFlapWebServer::getCurrentDay() {
    struct tm timeinfo;
    if (! getLocalTime(&timeinfo)) {
        return "Err";                          // Return error if time is not available
    }

    char dayStr[3];                            // Buffer for the day number (max "31" + null terminator)
    sprintf(dayStr, "%02d", timeinfo.tm_mday); // Format as two-digit string

    return String(dayStr);
}

void SplitFlapWebServer::setMode(int targetMode) {
    currentMode = targetMode;                 // Update cached mode immediately
    settings.putInt("mode", targetMode);      // Persist to storage
}

int SplitFlapWebServer::getMode() {
    return currentMode;                        // Return cached mode (fast)
}

void SplitFlapWebServer::checkWiFi() {
    if (connectionMode == 1) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi lost! Forcing reconnect...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

bool SplitFlapWebServer::loadWiFiCredentials() {
    // Allow WIFI_SSID and WIFI_PASS to be overridden by compile-time definitions
    String ssid = String(WIFI_SSID).isEmpty() ? settings.getString("ssid") : String(WIFI_SSID);
    String password = String(WIFI_PASS).isEmpty() ? settings.getString("password") : String(WIFI_PASS);

    if (ssid != "" && password != "") {
        Serial.println("Wi-Fi credentials loaded successfully.");
        Serial.print("Connecting to Network: ");
        Serial.println(ssid);
        WiFi.mode(WIFI_STA);
#ifdef WIFI_TX_POWER
        delay(100);
        WiFi.setTxPower((wifi_power_t) WIFI_TX_POWER);
#endif
        WiFi.begin(ssid.c_str(), password.c_str());
        return true; // Return true if credentials exist
    }
    return false;    // Return false if no credentials were found
}

void SplitFlapWebServer::checkRebootRequired() {
    if (rebootRequired) {
        Serial.println("Reboot required. Restarting...");
        delay(1000);
        ESP.restart();
    }
}

void SplitFlapWebServer::handleOta() {
    ArduinoOTA.handle();
}
void SplitFlapWebServer::enableOta() {
    // Skip OTA initialisation if no password is set
    if (settings.getString("otaPass") == "") {
        return;
    }

    ArduinoOTA.setHostname(settings.getString("mdns").c_str()); // otherwise mdns name gets overwritten with default
    ArduinoOTA.setPassword(settings.getString("otaPass").c_str());

    ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {            // U_LITTLEFS
            type = "filesystem";
            LittleFS.end(); // Unmount the filesystem before update
        }
        Serial.println("Start updating " + type);
    })
        .onEnd([]() {
        Serial.println("\nEnd");
        LittleFS.begin(); // Remount filesystem
    })
        .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    }).onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        LittleFS.begin(); // Remount filesystem
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();
    Serial.println("OTA Initialized");
}

bool SplitFlapWebServer::connectToWifi() {
    if (loadWiFiCredentials()) {
        unsigned long startAttemptTime = millis();
        const unsigned long timeout = 20000; // 20 seconds
        unsigned long lastPrintTime = startAttemptTime;

        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - startAttemptTime >= timeout) {
                Serial.println("_");
                Serial.println("Wi-Fi connection failed! Timeout reached.");
                return false; // Return false if unable to connect in 30 seconds
            }
            if ((millis() - lastPrintTime) > 1000) {
                Serial.print(".");
                lastPrintTime = millis();
            }
            yield();
        }

        // connected succesfully
        connectionMode = 1;
        WiFi.softAPdisconnect(); // Turns off SoftAP mode only after connected to
        // actual network
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true); // Saves Wi-Fi settings to flash memory
        WiFi.setSleep(false);
        Serial.println("Connected to Wi-Fi!");
        Serial.println("IP Address: http://" + WiFi.localIP().toString());
        return true;
    }
    return false;
}

void SplitFlapWebServer::startAccessPoint() {
    connectionMode = 0;
    const char *apSSID = AP_SSID;
    WiFi.softAP(apSSID);
#ifdef WIFI_TX_POWER
    delay(100);
    WiFi.setTxPower((wifi_power_t) WIFI_TX_POWER);
#endif
    Serial.println("AP Mode Started!");
    Serial.println("Connect to: " + String(apSSID));
    Serial.println("AP IP Address: http://" + WiFi.softAPIP().toString());
}

void fourOhFour(AsyncWebServerRequest *request) {
    Serial.println("Request: " + request->url());
    Serial.println("Method: " + String(request->methodToString()));
    request->send(404);
}

void SplitFlapWebServer::endMDNS() {
    MDNS.end();
    Serial.println("mDNS responder stopped");
}

void SplitFlapWebServer::startMDNS() {
    if (! MDNS.begin(settings.getString("mdns").c_str())) {
        Serial.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("mDNS: http://" + settings.getString("mdns") + ".local");
}

void SplitFlapWebServer::startWebServer() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { request->redirect("/index.html"); });

    File root = LittleFS.open("/");
    if (! root || ! root.isDirectory()) {
        Serial.println("Failed to open directory or not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (String(file.name()).endsWith(".gz")) {
            const char *filename = file.name();
            String tempFilename = (String("/") + String(filename));
            tempFilename.replace(".gz", "");
            filename = tempFilename.c_str();

            server.serveStatic(filename, LittleFS, filename, "max-age=600");
        }
        file = root.openNextFile();
    }

    server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "application/json", settings.toJson().as<String>());
    });

    server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        settings.reset();

        JsonDocument response;
        response["message"] = "Settings reset successfully! Reconnect to the " + String(AP_SSID) + " network";
        response["persistent"] = true;

        request->send(200, "application/json", response.as<String>());

        this->attemptReconnect = true;
    });

    // GET /api/display-config - Return display configuration info
    server.on("/api/display-config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument response;
        JsonArray displays = response["displays"].to<JsonArray>();

        // Local displays — offset by cluster offset so indices are globally consistent
        int localOffset = settings.getInt("clusterOffset");
        int numLocalDisplays = display ? display->getNumDisplays() : 0;
        for (int i = 0; i < numLocalDisplays; i++) {
            JsonObject disp = displays.add<JsonObject>();
            disp["index"]   = localOffset + i;
            disp["mux"]     = display->getDisplayMux(i);
            disp["channel"] = display->getDisplayChannel(i);
            disp["modules"] = display->getDisplayModuleCount(i);
            disp["local"]   = true;
        }

        // Remote displays (worker ESPs) — only available when this ESP is the cluster main
        if (cluster != nullptr && cluster->isMain()) {
            int maxW = cluster->getMaxWorkers();
            for (int w = 1; w <= maxW; w++) {
                if (!cluster->isWorkerAlive(w)) continue;
                int wOffset = cluster->getWorkerOffset(w);
                int wCount  = cluster->getWorkerDisplayCount(w);
                for (int d = 0; d < wCount; d++) {
                    JsonObject disp = displays.add<JsonObject>();
                    disp["index"]   = wOffset + d;
                    disp["mux"]     = 0;
                    disp["channel"] = d;
                    disp["modules"] = 0;  // module detail not reported by workers
                    disp["local"]   = false;
                    disp["worker"]  = w;
                }
            }
        }

        request->send(200, "application/json", response.as<String>());
    });

    // GET /api/cluster-status - Return cluster health and worker status
    server.on("/api/cluster-status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument response;

        String role = settings.getString("clusterRole");
        response["role"] = role;
        response["id"]   = settings.getString("clusterId");

        if (display) {
            response["localModules"] = display->getNumModules();
        }

        if (cluster != nullptr) {
            response["totalDisplays"] = cluster->getTotalDisplayCount();

            if (cluster->isMain()) {
                response["aliveWorkers"] = cluster->aliveWorkerCount();
                JsonArray workers = response["workers"].to<JsonArray>();
                int maxW = cluster->getMaxWorkers();
                int mainId = settings.getString("clusterId").toInt();
                for (int w = 1; w <= maxW; w++) {
                    if (w == mainId) continue;  // skip self — main never pongs itself
                    JsonObject wObj = workers.add<JsonObject>();
                    wObj["id"]           = w;
                    wObj["alive"]        = cluster->isWorkerAlive(w);
                    wObj["displayCount"] = cluster->getWorkerDisplayCount(w);
                    wObj["offset"]       = cluster->getWorkerOffset(w);
                }
            } else if (cluster->isWorker()) {
                response["mainAlive"] = cluster->isMainAlive();
                unsigned long lastMs = cluster->getLastExecutedMs();
                response["lastCmdAgoSecs"] = lastMs > 0 ? (long)((millis() - lastMs) / 1000) : -1;
                if (lastMs > 0) {
                    JsonArray lastTexts = response["lastTexts"].to<JsonArray>();
                    for (int i = 0; i < cluster->getLastExecutedCount(); i++) {
                        lastTexts.add(cluster->getLastExecutedText(i));
                    }
                }
            }
        } else {
            // No cluster object (should not happen at runtime, but guard anyway)
            response["totalDisplays"] = display ? display->getNumDisplays() : 0;
            response["aliveWorkers"]  = 0;
        }

        request->send(200, "application/json", response.as<String>());
    });

    // POST /api/displays - Store text for each display (Mode 7)
    server.addHandler(new AsyncCallbackJsonWebHandler(
        "/api/displays",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonDocument response;
            
            if (!json.is<JsonObject>()) {
                response["error"] = "Invalid JSON format";
                request->send(400, "application/json", response.as<String>());
                return;
            }
            
            if (!display) {
                response["error"] = "Display not initialized";
                request->send(500, "application/json", response.as<String>());
                return;
            }
            
            JsonObject obj = json.as<JsonObject>();

            // Total displays = cluster count when main, local count otherwise
            int numDisplays = 0;
            if (cluster != nullptr && cluster->isMain()) {
                numDisplays = cluster->getTotalDisplayCount();
            } else {
                numDisplays = display->getNumDisplays();
            }
            numDisplays = max(numDisplays, 1);
            numDisplays = min(numDisplays, MAX_DISPLAY_TEXTS);
            
            // Parse centering preference (default to true)
            bool center = obj["center"].is<bool>() ? obj["center"].as<bool>() : true;
            displayCentering = center;
            
            // Parse display texts for all displays
            String* texts = new String[numDisplays];
            for (int i = 0; i < numDisplays; i++) {
                String key = "dis" + String(i + 1);
                texts[i] = obj[key].is<String>() ? obj[key].as<String>() : "";
            }
            
            // Store display texts (with dynamic count) and set update flag
            setDisplayTexts(texts, numDisplays);
            
            // Set mode to 7 (per-display mode)
            setMode(7);
            
            // Clean up
            delete[] texts;
            
            response["success"] = true;
            response["message"] = "Display texts updated";
            request->send(200, "application/json", response.as<String>());
        }
    ));

    // GET /api/test-mode - Return current test mode status
    server.on("/api/test-mode", HTTP_GET, [this](AsyncWebServerRequest *request) {
        JsonDocument response;

        response["mode"] = getMode();
        response["delayMs"] = testModeDelay;
        response["skip"] = testModeSkip;
        response["charIndex"] = testModeCharIndex;
        response["currentChar"] = String(testModeCurrentChar);
        response["cycleCount"] = testModeCycleCount;
        response["lastChange"] = lastTestModeTime;

        request->send(200, "application/json", response.as<String>());
    });

    // POST /api/test-mode - Start all display test mode (Mode 8)
    server.addHandler(new AsyncCallbackJsonWebHandler(
        "/api/test-mode",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
            JsonDocument response;
            
            if (!json.is<JsonObject>()) {
                response["error"] = "Invalid JSON format";
                request->send(400, "application/json", response.as<String>());
                return;
            }
            
            JsonObject obj = json.as<JsonObject>();
            
            // Parse delay (in seconds, convert to ms), default 5 seconds
            if (obj["delay"].is<int>()) {
                testModeDelay = obj["delay"].as<int>() * 1000;
            } else {
                testModeDelay = 5000;
            }
            
            // Parse skip count, default 1
            if (obj["skip"].is<int>()) {
                testModeSkip = constrain(obj["skip"].as<int>(), 1, 36);
            } else {
                testModeSkip = 1;
            }
            
            bool alreadyInTestMode = (getMode() == 8);

            // Set mode to 8 (all display test mode)
            setMode(8);

            // Reset only when entering test mode for the first time
            if (!alreadyInTestMode) {
                testModeCharIndex = 0;
                lastTestModeTime = 0;
                testModeCurrentChar = ' ';
                testModeCycleCount = 0;
            }
            
            response["success"] = true;
            response["message"] = "Test mode started";
            request->send(200, "application/json", response.as<String>());
        }
    ));

    server.addHandler(new AsyncCallbackJsonWebHandler(
        "/settings",
        [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() != HTTP_POST) {
            return request->send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
        }

        Serial.println("Received settings update request");
        Serial.println(json.as<String>());

        bool rebootRequired = false;
        bool reconnect = false;
        JsonDocument response;
        response["message"] = "Settings saved successfully!";

        // TODO Refactor this it's gross
        if ((json["ssid"].is<String>() && json["ssid"].as<String>() != settings.getString("ssid")) ||
            (json["password"].is<String>() && json["password"].as<String>() != settings.getString("password"))) {
            reconnect = true;
            response["message"] = "Settings updated successfully, Network " "settings have changed, reconnect to the " +
                json["ssid"].as<String>() + " network";
        }

        if (json["otaPass"].is<String>() && json["otaPass"].as<String>() != settings.getString("otaPass")) {
            rebootRequired = true; // OTA password change can only be applied by rebooting
            response["message"] = "Settings updated successfully, OTA Password has changed. Rebooting...";
        }

        if (json["mdns"].is<String>() && json["mdns"].as<String>() != settings.getString("mdns")) {
            reconnect = true;
            response["message"] =
                "Settings updated successfully, mDNS name has changed, " "automatically redirecting to http://" +
                json["mdns"].as<String>() + ".local...";
            response["redirect"] = "http://" + json["mdns"].as<String>() + ".local/settings.html";
        }

        if ((json["mqtt_server"].is<String>() && json["mqtt_server"].as<String>() != settings.getString("mqtt_server")
            ) ||
            (json["mqtt_port"].is<int>() && json["mqtt_port"].as<int>() != settings.getInt("mqtt_port")) ||
            (json["mqtt_user"].is<String>() && json["mqtt_user"].as<String>() != settings.getString("mqtt_user")) ||
            (json["mqtt_pass"].is<String>() && json["mqtt_pass"].as<String>() != settings.getString("mqtt_pass"))) {
            response["message"] = "Mqtt settings have changed, reconnecting...";
            reconnect = true;
        }

        if (! settings.fromJson(json)) {
            response["message"] = "Failed to save settings";
            response["type"] = "error";
            response["errors"]["key"] = settings.getLastValidationKey();
            response["errors"]["message"] = settings.getLastValidationError();
            return request->send(400, "application/json", response.as<String>());
        }

        response["type"] = "success";
        response["persistent"] = reconnect;

        request->send(200, "application/json", response.as<String>());

        this->rebootRequired = rebootRequired;
        this->attemptReconnect = reconnect;
    }
    ));

    server
        .addHandler(new AsyncCallbackJsonWebHandler("/text", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() != HTTP_POST) {
            return request->send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
        }

        Serial.println("Received text update request");
        Serial.println(json.as<String>());

        // {"mode":"single","words":["adfasdf"],"delay":1,"center":false}
        // {"mode":"multiple","words":["asdf","asdfasdf","fffff"],"delay":"14","center":true}
        JsonDocument response;

        if (! json["mode"].is<String>()) {
            response["message"] = "Invalid mode type";
        }

        if (! json["words"].is<JsonArray>()) {
            response["message"] = "Invalid words array";
        }

        float delay = json["delay"].as<float>();
        if (delay < 1) {
            response["message"] = "Invalid delay type / value";
        }

        if (! json["center"].is<bool>()) {
            response["message"] = "Invalid center type";
        }

        if (response["message"].is<String>()) {
            response["type"] = "error";
            return request->send(400, "application/json", response.as<String>());
        }

        this->setMultiDelay(delay * 1000);
        Serial.println("Delay: " + String(this->getMultiWordDelay()));

        centering = json["center"].as<bool>() ? 1 : 0;
        Serial.println("centering: " + String(centering ? "true" : "false"));

        if (json["mode"] == "single") {
            String word = decodeURIComponent(json["words"][0].as<String>());
            Serial.println("Single Word: " + word);
            this->setInputString(word);
            this->setMode(0); // change mode last once all variables updated
        }

        if (json["mode"] == "multiple") {
            JsonArray wordsArray = json["words"].as<JsonArray>();
            String words = "";
            for (JsonVariant v : wordsArray) {
                words += decodeURIComponent(v.as<String>()) + ",";
            }
            if (words.length() > 0) {
                words.remove(words.length() - 1);
            }

            this->setMultiInputString(words);
            this->numMultiWords = wordsArray.size();
            Serial.println("Multiple Words: " + words);
            Serial.println("Number of Words: " + String(this->numMultiWords));

            this->setMode(1);
        }

        response["message"] = "Text updated successfully!";
        response["type"] = "success";

        request->send(200, "application/json", response.as<String>());
    }));

    server.onNotFound(fourOhFour);

    server.begin();
}

String SplitFlapWebServer::decodeURIComponent(String encodedString) {
    String decodedString = encodedString;
    // Replace common URL-encoded characters with their actual symbols
    decodedString.replace("%20", " ");  // space
    decodedString.replace("%21", "!");  // exclamation mark
    decodedString.replace("%22", "\""); // double quote
    decodedString.replace("%23", "#");  // hash
    decodedString.replace("%24", "$");  // dollar sign
    decodedString.replace("%25", "%");  // percent
    decodedString.replace("%26", "&");  // ampersand
    decodedString.replace("%27", "'");  // single quote
    decodedString.replace("%28", "(");  // left parenthesis
    decodedString.replace("%29", ")");  // right parenthesis
    decodedString.replace("%2A", "*");  // asterisk
    decodedString.replace("%2B", "+");  // plus
    decodedString.replace("%2C", ",");  // comma
    decodedString.replace("%2D", "-");  // hyphen
    decodedString.replace("%2E", ".");  // period
    decodedString.replace("%2F", "/");  // forward slash
    decodedString.replace("%3A", ":");  // colon
    decodedString.replace("%3B", ";");  // semicolon
    decodedString.replace("%3C", "<");  // less than
    decodedString.replace("%3D", "=");  // equal sign
    decodedString.replace("%3E", ">");  // greater than
    decodedString.replace("%3F", "?");  // question mark
    decodedString.replace("%40", "@");  // at symbol
    decodedString.replace("%5B", "[");  // left bracket
    decodedString.replace("%5C", "\\"); // backslash
    decodedString.replace("%5D", "]");  // right bracket
    decodedString.replace("%5E", "^");  // caret
    decodedString.replace("%5F", "_");  // underscore
    decodedString.replace("%60", "`");  // grave accent
    decodedString.replace("%7B", "{");  // left brace
    decodedString.replace("%7C", "|");  // vertical bar
    decodedString.replace("%7D", "}");  // right brace
    decodedString.replace("%7E", "~");  // tilde

    return decodedString;
}
