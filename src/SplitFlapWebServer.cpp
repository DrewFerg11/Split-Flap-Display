#include "SplitFlapWebServer.h"

#include "BackgroundTick.h"
#include "Log.h"
#include "LogRedaction.h"
#include "SplitFlapDisplay.h"

#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <algorithm>
#include <memory>
#include <new>

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
      centering(1), accuracyCharIndex(0), accuracyDelay(5000), accuracyStepSize(1), lastAccuracyStepTime(0) {
    lastSwitchMultiTime = millis();
}

void SplitFlapWebServer::init() {
    __atomic_store_n(&diagnosticMqttConfigured, ! settings.getString("mqtt_server").isEmpty(), __ATOMIC_RELAXED);
    if (! LittleFS.begin()) {
        Log.println("An Error has occurred while mounting LittleFS");
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
        Log.println("Failed to open timezones.json; defaulting to UTC");
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
        Log.println("Failed to parse timezones.json: " + String(error.c_str()));
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

    Log.println("POSIX Timezone set to: " + posixTimezone);
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
    settings.putInt("mode", targetMode);
}

int SplitFlapWebServer::getMode() {
    return settings.getInt("mode");
}

void SplitFlapWebServer::checkWiFi() {
    if (connectionMode == 1) {
        if (WiFi.status() != WL_CONNECTED) {
            Log.println("Wi-Fi lost! Forcing reconnect...");
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
        Log.println("Wi-Fi credentials loaded successfully.");
        Log.print("Connecting to Network: ");
        Log.println(ssid);
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
        Log.println("Reboot required. Restarting...");
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
        Log.println("Start updating " + type);
    })
        .onEnd([]() {
        Log.println("\nEnd");
        LittleFS.begin(); // Remount filesystem
    })
        .onProgress([](unsigned int progress, unsigned int total) {
        Log.printf("Progress: %u%%\r", (progress / (total / 100)));
    }).onError([](ota_error_t error) {
        Log.printf("Error[%u]: ", error);
        LittleFS.begin(); // Remount filesystem
        if (error == OTA_AUTH_ERROR) {
            Log.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Log.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Log.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Log.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Log.println("End Failed");
        }
    });

    ArduinoOTA.begin();
    Log.println("OTA Initialized");
}

bool SplitFlapWebServer::connectToWifi() {
    if (loadWiFiCredentials()) {
        unsigned long startAttemptTime = millis();
        const unsigned long timeout = 20000; // 20 seconds
        unsigned long lastPrintTime = startAttemptTime;

        while (WiFi.status() != WL_CONNECTED) {
            backgroundTick();
            if (millis() - startAttemptTime >= timeout) {
                Log.println("_");
                Log.println("Wi-Fi connection failed! Timeout reached.");
                return false; // Return false if unable to connect within the timeout
            }
            if ((millis() - lastPrintTime) > 1000) {
                Log.print(".");
                lastPrintTime = millis();
            }
            yield();
        }

        // connected succesfully
        __atomic_store_n(&connectionMode, 1, __ATOMIC_RELAXED);
        WiFi.softAPdisconnect(); // Turns off SoftAP mode only after connected to
        // actual network
        WiFi.setAutoReconnect(true);
        WiFi.persistent(true); // Saves Wi-Fi settings to flash memory
        WiFi.setSleep(false);
        Log.println("Connected to Wi-Fi!");
        Log.println("IP Address: http://" + WiFi.localIP().toString());
        return true;
    }
    return false;
}

void SplitFlapWebServer::startAccessPoint() {
    __atomic_store_n(&connectionMode, 0, __ATOMIC_RELAXED);
    const char *apSSID = AP_SSID;
    WiFi.softAP(apSSID);
#ifdef WIFI_TX_POWER
    delay(100);
    WiFi.setTxPower((wifi_power_t) WIFI_TX_POWER);
#endif
    Log.println("AP Mode Started!");
    Log.println("Connect to: " + String(apSSID));
    Log.println("AP IP Address: http://" + WiFi.softAPIP().toString());
}

void fourOhFour(AsyncWebServerRequest *request) {
    Log.println("Request: " + request->url());
    Log.println("Method: " + String(request->methodToString()));
    request->send(404);
}

void SplitFlapWebServer::endMDNS() {
    MDNS.end();
    Log.println("mDNS responder stopped");
}

void SplitFlapWebServer::startMDNS() {
    if (! MDNS.begin(settings.getString("mdns").c_str())) {
        Log.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
        }
    }

    Log.println("mDNS: http://" + settings.getString("mdns") + ".local");
}

void SplitFlapWebServer::startWebServer() {
    server.on("/log", HTTP_GET, [this](AsyncWebServerRequest *request) { sendLog(request); });
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) { sendStatus(request); });
    logEvents.authorizeConnect([this](AsyncWebServerRequest *) { return logEvents.count() < 2; });
    logEvents.onConnect([this](AsyncEventSourceClient *) {
        // onConnect runs under the library's client-list mutex: do not call
        // count(), send(), or close() from this callback.
        // AsyncTCP only requests a snapshot. All SSE sends happen on the main task.
        __atomic_store_n(&logSnapshotRequested, true, __ATOMIC_RELEASE);
    });
    server.addHandler(&logEvents);
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) { request->redirect("/index.html"); });

    File root = LittleFS.open("/");
    if (! root || ! root.isDirectory()) {
        Log.println("Failed to open directory or not a directory");
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

    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument response;
        response["version"] = FIRMWARE_VERSION;
        String output;
        serializeJson(response, output);
        request->send(200, "application/json", output);
    });

    server.on("/settings/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        settings.reset();
        __atomic_store_n(&diagnosticMqttConfigured, false, __ATOMIC_RELAXED);

        JsonDocument response;
        response["message"] = "Settings reset successfully! Reconnect to the " + String(AP_SSID) + " network";
        response["persistent"] = true;

        request->send(200, "application/json", response.as<String>());

        this->attemptReconnect = true;
    });

    server.addHandler(
        new AsyncCallbackJsonWebHandler("/settings", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() != HTTP_POST) {
            return request->send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
        }

        Log.println("Received settings update request");
        JsonDocument loggedSettings;
        if (loggedSettings.set(json)) {
            redactSecrets(loggedSettings);
            if (! loggedSettings.overflowed()) {
                Log.println(loggedSettings.as<String>());
            }
        }

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
            rebootRequired = true;
            response["message"] = "Settings updated successfully, OTA Password has changed. Rebooting...";
        }

        if ((json["wireAddresses"].is<String>() &&
             json["wireAddresses"].as<String>() != settings.getString("wireAddresses")) ||
            (json["wireOffsets"].is<String>() &&
             json["wireOffsets"].as<String>() != settings.getString("wireOffsets")) ||
            (json["wire1Addresses"].is<String>() &&
             json["wire1Addresses"].as<String>() != settings.getString("wire1Addresses")) ||
            (json["wire1Offsets"].is<String>() &&
             json["wire1Offsets"].as<String>() != settings.getString("wire1Offsets")) ||
            (json["sdaPin"].is<int>() && json["sdaPin"].as<int>() != settings.getInt("sdaPin")) ||
            (json["sclPin"].is<int>() && json["sclPin"].as<int>() != settings.getInt("sclPin")) ||
            (json["sda2Pin"].is<int>() && json["sda2Pin"].as<int>() != settings.getInt("sda2Pin")) ||
            (json["scl2Pin"].is<int>() && json["scl2Pin"].as<int>() != settings.getInt("scl2Pin"))) {
            rebootRequired = true;
            response["message"] = "Hardware settings changed. Rebooting to apply...";
        }

        if (json["mdns"].is<String>() && json["mdns"].as<String>() != settings.getString("mdns")) {
            reconnect = true;
            response["message"] =
                "Settings updated successfully, mDNS name has changed, " "automatically redirecting to http://" +
                json["mdns"].as<String>() + ".local...";
            response["redirect"] = "http://" + json["mdns"].as<String>() + ".local/settings.html";
        }

        if ((json["mqtt_server"].is<String>() &&
             json["mqtt_server"].as<String>() != settings.getString("mqtt_server")) ||
            (json["mqtt_port"].is<int>() && json["mqtt_port"].as<int>() != settings.getInt("mqtt_port")) ||
            (json["mqtt_user"].is<String>() && json["mqtt_user"].as<String>() != settings.getString("mqtt_user")) ||
            (json["mqtt_pass"].is<String>() && json["mqtt_pass"].as<String>() != settings.getString("mqtt_pass"))) {
            response["message"] = "Mqtt settings have changed, reconnecting...";
            reconnect = true;
        }

        if (! settings.fromJson(json)) {
            response["message"] = settings.getLastValidationError();
            response["type"] = "error";
            response["errors"]["key"] = settings.getLastValidationKey();
            response["errors"]["message"] = settings.getLastValidationError();
            return request->send(400, "application/json", response.as<String>());
        }

        if (json["mqtt_server"].is<String>()) {
            __atomic_store_n(&diagnosticMqttConfigured, ! json["mqtt_server"].as<String>().isEmpty(), __ATOMIC_RELAXED);
        }
        response["type"] = "success";
        response["persistent"] = reconnect;

        request->send(200, "application/json", response.as<String>());

        this->rebootRequired = rebootRequired;
        this->attemptReconnect = reconnect;
    })
    );

    server.addHandler(
        new AsyncCallbackJsonWebHandler("/text", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() != HTTP_POST) {
            return request->send(405, "application/json", "{\"error\":\"Method Not Allowed\"}");
        }

        Log.println("Received text update request");
        Log.println(json.as<String>());

        JsonDocument response;

        if (! json["mode"].is<String>()) {
            response["message"] = "Invalid mode";
            response["type"] = "error";
            return request->send(400, "application/json", response.as<String>());
        }

        String mode = json["mode"].as<String>();
        centering = json["center"].is<bool>() ? (json["center"].as<bool>() ? 1 : 0) : 0;

        auto buildCsv = [this](JsonArray arr) -> String {
            String csv = "";
            for (JsonVariant v : arr) {
                csv += decodeURIComponent(v.as<String>()) + ",";
            }
            if (csv.length() > 0) csv.remove(csv.length() - 1);
            return csv;
        };

        if (mode == "single") {
            if (! json["words"].is<JsonArray>()) {
                response["message"] = "Invalid words array";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            this->setInputString(decodeURIComponent(json["words"][0].as<String>()));
            this->setMode(0);
            this->writtenString = "";

        } else if (mode == "multiple") {
            if (! json["words"].is<JsonArray>()) {
                response["message"] = "Invalid words array";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            float delay = json["delay"].as<float>();
            if (delay < 1) {
                response["message"] = "Delay must be at least 1 second";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            this->setMultiDelay(delay * 1000);
            JsonArray arr = json["words"].as<JsonArray>();
            this->setMultiInputString(buildCsv(arr));
            this->numMultiWords = arr.size();
            this->inputString = "";
            this->setMode(1);
            this->writtenString = "";

        } else if (mode == "dual-single") {
            if (! json["row1"].is<String>() || ! json["row2"].is<String>()) {
                response["message"] = "Invalid row1 or row2";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            this->dualRow1String = decodeURIComponent(json["row1"].as<String>());
            this->dualRow2String = decodeURIComponent(json["row2"].as<String>());
            this->inputString = "";
            this->setMode(7);
            this->writtenString = "";

        } else if (mode == "dual-multiple") {
            if (! json["words"].is<JsonArray>()) {
                response["message"] = "Invalid words array";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            float delay = json["delay"].as<float>();
            if (delay < 1) {
                response["message"] = "Delay must be at least 1 second";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            this->setMultiDelay(delay * 1000);
            JsonArray arr = json["words"].as<JsonArray>();
            this->setMultiInputString(buildCsv(arr));
            this->numMultiWords = arr.size();
            this->multiWordCurrentIndex = 0;
            this->inputString = "";
            this->setMode(8);
            this->writtenString = "";

        } else if (mode == "accuracy") {
            float delay = json["delay"].as<float>();
            if (delay < 1) {
                response["message"] = "Delay must be at least 1 second";
                response["type"] = "error";
                return request->send(400, "application/json", response.as<String>());
            }
            int stepSize = json["stepSize"].as<int>();
            if (stepSize < 1) stepSize = 1;
            this->accuracyCharIndex = 0;
            this->accuracyDelay = (int) (delay * 1000);
            this->accuracyStepSize = stepSize;
            this->lastAccuracyStepTime = 0;
            this->inputString = "";
            this->setMode(9);
            this->writtenString = "";

        } else {
            response["message"] = "Unknown mode: " + mode;
            response["type"] = "error";
            return request->send(400, "application/json", response.as<String>());
        }
        response["message"] = "Text updated successfully!";
        response["type"] = "success";
        request->send(200, "application/json", response.as<String>());
    })
    );

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

void SplitFlapWebServer::setDiagnostics(SplitFlapDisplay *display, const char *resetReason) {
    diagnosticDisplay = display;
    diagnosticResetReason = resetReason;
}

void SplitFlapWebServer::sendLog(AsyncWebServerRequest *request) {
    std::unique_ptr<char[]> buffer(new (std::nothrow) char[LOG_BUFFER_SIZE]);
    if (! buffer) return request->send(503, "text/plain", "Log snapshot unavailable");
    uint64_t end;
    size_t length = Log.snapshot(buffer.get(), LOG_BUFFER_SIZE, end);
    String output;
    if (! output.reserve(length + 1)) return request->send(503, "text/plain", "Log snapshot unavailable");
    output.concat(buffer.get(), length);
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain; charset=utf-8", output);
    char cursor[24];
    snprintf(cursor, sizeof(cursor), "%llu", (unsigned long long) end);
    response->addHeader("X-Log-Cursor", cursor);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

void SplitFlapWebServer::sendStatus(AsyncWebServerRequest *request) {
    JsonDocument status;
    status["version"] = FIRMWARE_VERSION;
    status["buildSource"] = FIRMWARE_BUILD_SOURCE;
    status["chip"] = ESP.getChipModel();
    status["uptimeMs"] = millis();
    status["resetReason"] = diagnosticResetReason;
    status["freeHeap"] = ESP.getFreeHeap();
    status["minFreeHeap"] = ESP.getMinFreeHeap();
    bool sta = __atomic_load_n(&connectionMode, __ATOMIC_RELAXED) == 1;
    bool connected = WiFi.status() == WL_CONNECTED;
    JsonObject wifi = status["wifi"].to<JsonObject>();
    wifi["connected"] = connected;
    wifi["ssid"] = sta ? WiFi.SSID() : String(AP_SSID);
    wifi["rssi"] = connected ? WiFi.RSSI() : 0;
    wifi["ip"] = sta ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    wifi["mode"] = sta ? "sta" : "ap";
    status["mqtt"]["configured"] = __atomic_load_n(&diagnosticMqttConfigured, __ATOMIC_RELAXED);
    status["mqtt"]["connected"] = __atomic_load_n(&diagnosticMqttConnected, __ATOMIC_RELAXED);
    JsonArray modules = status["modules"].to<JsonArray>();
    if (diagnosticDisplay != nullptr) {
        SplitFlapDisplay::ModuleStatus module;
        for (int i = 0; diagnosticDisplay->getModuleStatus(i, module); i++) {
            JsonObject entry = modules.add<JsonObject>();
            entry["index"] = module.index;
            entry["bus"] = module.bus;
            entry["address"] = module.address;
            entry["ok"] = module.ok;
            entry["hasErrored"] = module.hasErrored;
        }
    }
    String output;
    serializeJson(status, output);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

void SplitFlapWebServer::pollDiagnostics(bool mqttConnected) {
    __atomic_store_n(&diagnosticMqttConnected, mqttConnected, __ATOMIC_RELAXED);
    unsigned long now = millis();
    if (now - lastLogFlush < 100) return;
    lastLogFlush = now;
    if (logEvents.count() == 0) return;

    bool snapshot = __atomic_exchange_n(&logSnapshotRequested, false, __ATOMIC_ACQ_REL);
    size_t capacity = snapshot ? LOG_BUFFER_SIZE : 2048;
    std::unique_ptr<char[]> buffer(new (std::nothrow) char[capacity + 1]);
    if (! buffer) {
        __atomic_store_n(&logSnapshotRequested, true, __ATOMIC_RELEASE);
        return;
    }
    uint64_t start = logCursor;
    bool dropped = false;
    size_t length;
    if (snapshot) {
        length = Log.snapshot(buffer.get(), capacity, logCursor);
        start = logCursor - length;
        // Replay snapshots in small chunks too, keeping slow-client queues bounded.
        length = std::min(length, size_t(2048));
        logCursor = start + length;
    } else {
        length = Log.readSince(logCursor, buffer.get(), capacity, dropped);
    }
    if (length == 0 && ! snapshot) return;
    buffer[length] = '\0';
    if (dropped) {
        // Resynchronize on the next main-task tick rather than sending a partial tail.
        __atomic_store_n(&logSnapshotRequested, true, __ATOMIC_RELEASE);
        return;
    }
    if (length > 0) {
        // A chunk may end halfway through a UTF-8 character. Leave its bytes in
        // the ring for the next tick instead of emitting invalid JSON text.
        size_t lead = length - 1;
        while (lead > 0 && (uint8_t(buffer[lead]) & 0xC0) == 0x80) lead--;
        uint8_t byte = buffer[lead];
        size_t width = (byte & 0xF8) == 0xF0 ? 4 : (byte & 0xF0) == 0xE0 ? 3 : (byte & 0xE0) == 0xC0 ? 2 : 1;
        if (length - lead < width) {
            logCursor -= length - lead;
            length = lead;
            buffer[length] = '\0';
        }
        if (length == 0) return;
    }
    const char *text = buffer.get();
    if (snapshot) {
        // Wrapping can leave the oldest byte inside a UTF-8 character.
        while (*text != '\0' && (uint8_t(*text) & 0xC0) == 0x80) text++;
    }
    JsonDocument packet;
    packet["text"] = text;
    packet["start"] = start;
    packet["end"] = logCursor;
    String output;
    serializeJson(packet, output);
    if (packet.overflowed() || output.length() == 0) {
        __atomic_store_n(&logSnapshotRequested, true, __ATOMIC_RELEASE);
        return;
    }
    if (logEvents.send(output.c_str(), snapshot ? "snapshot" : "log", 0, 1000) != AsyncEventSource::ENQUEUED) {
        __atomic_store_n(&logSnapshotRequested, true, __ATOMIC_RELEASE);
    }
}
