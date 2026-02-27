// Split Flap Display
// Morgan Manly 02/16/2025
// Jordan Hoff 03/25/2025
// Thom Koopman 03/30/2025

// Enjoy :)
#include "JsonSettings.h"
#include "SplitFlapCluster.h"
#include "SplitFlapDisplay.h"
#include "SplitFlapMqtt.h"
#include "SplitFlapWebServer.h"

#include <Arduino.h>
#include <WiFiClient.h>
#include <Preferences.h>

// --- Build-time cluster config ---
// Set ESP_CONFIG_FILE via build_flags to pick a config from src/config/.
// Example: '-D ESP_CONFIG_FILE="config/esp_1.h"' in platformio.ini
// To build and flash a specific ESP: pio run -e esp_1 -t upload  (or esp_2, esp_3, esp_4)
#ifdef ESP_CONFIG_FILE
#include ESP_CONFIG_FILE
#endif

// Fallback defaults when no config header is included (standalone / esp32_wroom env):
#ifndef DISPLAY_NAME
#define DISPLAY_NAME "My Display"
#endif
#ifndef CLUSTER_ROLE
#define CLUSTER_ROLE "standalone"
#endif
#ifndef CLUSTER_ID
#define CLUSTER_ID "1"
#endif
#ifndef CLUSTER_OFFSET
#define CLUSTER_OFFSET 0
#endif
#ifndef CLUSTER_DISPLAY_COUNT
#define CLUSTER_DISPLAY_COUNT 5
#endif
#ifndef WIRE0_MUX_ADDRS
#define WIRE0_MUX_ADDRS "112"
#endif
#ifndef WIRE0_CH_MOD_ADDRS_112
#define WIRE0_CH_MOD_ADDRS_112 "32,33,34,35,36;32,33,34,35,36;32,33,34,35,36;32,33,34,35,36;32,33,34,35,36;32,33,34,35,36;;;"
#endif
#ifndef WIRE1_MUX_ADDRS
#define WIRE1_MUX_ADDRS "112"
#endif
#ifndef WIRE1_CH_MOD_ADDRS_112
#define WIRE1_CH_MOD_ADDRS_112 ";;;;;;;;"
#endif

// clang-format off
JsonSettings settings = JsonSettings("config", {
    // General Settings
    {"name", JsonSetting(DISPLAY_NAME)},
    {"mdns", JsonSetting("splitflap")},
    {"otaPass", JsonSetting("")},
    {"timezone", JsonSetting("UTC0")},
    {"dateFormat", JsonSetting("{dd}-{mm}-{yy}")},
    {"timeFormat", JsonSetting("{HH}:{mm}")},
    // Wifi Settings
    {"ssid", JsonSetting("")},
    {"password", JsonSetting("")},
    // MQTT Settings
    {"mqtt_server", JsonSetting("")},
    {"mqtt_port", JsonSetting(1883)},
    {"mqtt_user", JsonSetting("")},
    {"mqtt_pass", JsonSetting("")},
    // Hardware Settings
    // I2C addresses of multiplexers (comma-separated) (112 - 120)
    {"wire0MuxAddrs", JsonSetting(WIRE0_MUX_ADDRS)},
    // Channels and module I2C addresses per mux (semicolon-separated channels, comma-separated addresses, NO SPACES)
    {"wire0ChModAddrs112", JsonSetting(WIRE0_CH_MOD_ADDRS_112)},
    // I2C addresses of multiplexers (comma-separated) (112 - 120)
    {"wire1MuxAddrs", JsonSetting(WIRE1_MUX_ADDRS)},
    // Channels and module I2C addresses per mux (semicolon-separated channels, comma-separated addresses, NO SPACES)
    {"wire1ChModAddrs112", JsonSetting(WIRE1_CH_MOD_ADDRS_112)},
    {"magnetPosition", JsonSetting(730)},
    {"displayOffset", JsonSetting(0)},
    {"useDualBus", JsonSetting(true)},
    // Primary I2C bus (Wire)
    {"wire0SdaPin", JsonSetting(SDA_PIN)},
    {"wire0SclPin", JsonSetting(SCL_PIN)},
    // Secondary I2C bus (Wire1) - only used if useDualBus=true
    {"wire1Sda1Pin", JsonSetting(SDA1_PIN)},
    {"wire1Scl1Pin", JsonSetting(SCL1_PIN)},
    {"stepsPerRot", JsonSetting(2048)},
    {"charset", JsonSetting(37)},
    // Operational Settings
    {"debugLogging", JsonSetting(false)},        // Enable general debug output (init, config, commands)
    {"perfLogging", JsonSetting(false)},         // Enable I2C bus performance metrics logging
    {"i2cTransactionTime", JsonSetting(65)},     // Estimated microseconds per I2C transaction (for util % calc)
    {"quickHome", JsonSetting(true)},            // Skip label/blank phases during home (faster)
    {"halfStepping", JsonSetting(false)},        // Use 8-phase half-stepping for 4096 steps/rot (double resolution)
    {"maxVel", JsonSetting(14.0f)},              // Motor datasheet: 15 RPM | Practical max (I2C): ~10 RPM | Typical achieved: 8-11 RPM
    // Accuracy Settings
    {"accuracyLogging", JsonSetting(false)},     // Enable detailed accuracy/calibration debug output
    {"stepSettleUs", JsonSetting(75)},           // Microseconds to wait after each step for motor settling (0=max speed, 100-200=recommended)
    {"sensorDebounceCount", JsonSetting(1)},     // Consecutive sensor reads required before triggering (1=no debounce)
    {"sensorDebugMs", JsonSetting(0)},           // Log hall sensor transitions for N ms after first move (0=disabled)
    {"sensorCheckSteps", JsonSetting(20)},       // Steps between sensor reads (0=use time-based 20ms polling)
    {"retryFailedSteps", JsonSetting(3)},        // I2C retry attempts on step failure (0=disabled)
    {"missedMagnetRecovery", JsonSetting(true)}, // Auto-home modules that miss magnet crossings (false=disabled)
    {"errorStatsTracking", JsonSetting(true)},   // Track position error statistics per module (false=disabled)
    // Cluster Settings (set via build-time config header, see config/ folder)
    {"clusterRole", JsonSetting(CLUSTER_ROLE)},                  // "standalone", "main", or "worker"
    {"clusterId", JsonSetting(CLUSTER_ID)},                      // Unique ID for this ESP in the cluster ("1", "2", etc.)
    {"clusterOffset", JsonSetting(CLUSTER_OFFSET)},              // First logical display index owned by this ESP
    {"clusterDisplayCount", JsonSetting(CLUSTER_DISPLAY_COUNT)}, // Number of displays this ESP controls
    {"clusterLogging", JsonSetting(true)},                       // Enable cluster UART communication logging
    {"clusterPingIntervalMs", JsonSetting(5000)},                // How often main broadcasts ping to workers (ms)
    {"clusterWorkerTimeoutMs", JsonSetting(15000)},              // Worker considered offline after N ms without pong
    {"clusterMaxWorkers", JsonSetting(4)},                       // Number of worker ESPs in the cluster (excludes main)
    {"clusterUartBaud", JsonSetting(460800)},                    // UART2 baud rate (must match on all ESPs)
    {"clusterUartRxPin", JsonSetting(16)},                       // UART2 RX GPIO (GPIO 16 = default UART2 on ESP32-WROOM)
    {"clusterUartTxPin", JsonSetting(17)},                       // UART2 TX GPIO (GPIO 17 = default UART2 on ESP32-WROOM)
    // Operational States
    {"mode", JsonSetting(0)}
});
// clang-format on

WiFiClient wifiClient;
SplitFlapDisplay display(settings);
SplitFlapWebServer webServer(settings);
SplitFlapMqtt splitflapMqtt(settings, wifiClient);
SplitFlapCluster cluster(settings);

void setup() {
    // put your setup code here, to run once:
    Serial.begin(SERIAL_SPEED);

#ifdef STARTUP_DELAY
    delay(STARTUP_DELAY);
#endif

    // TODO: Why do I need this all of a sudden?
    // // Initialize NVS with defaults if config is missing.
    // Preferences prefs;
    // prefs.begin("config", true);
    // bool nvsIsEmpty = !prefs.isKey("stepsPerRot");
    // prefs.end();

    // if (nvsIsEmpty) {
    //     Serial.println("[SETUP] Fresh NVS detected - saving defaults...");
    //     settings.reset();
    //     Serial.println("[SETUP] Defaults saved successfully");
    // }

    Serial.println("Init Web Server");
    webServer.init();
    cluster.begin();

    if (! webServer.connectToWifi()) {
        webServer.startAccessPoint();
        webServer.enableOta();
        webServer.startMDNS();

        display.init();
        webServer.setDisplay(&display);  // Pass display reference to web server
        cluster.setDisplay(&display);
        webServer.startWebServer();

        cluster.distributeHome(MAX_RPM, settings.getInt("quickHome") != 0);

        if (display.getNumModules() == 8) {
            display.writeString("Wifi Err");
        } else {
            display.writeChar('X');
        }
    } else {
        webServer.enableOta();
        webServer.startMDNS();

        display.init();
        webServer.setDisplay(&display);  // Pass display reference to web server
        cluster.setDisplay(&display);
        webServer.startWebServer();

        splitflapMqtt.setup();
        splitflapMqtt.setDisplay(&display);
        display.setMqtt(&splitflapMqtt);

        cluster.distributeHome(MAX_RPM, settings.getInt("quickHome") != 0);
    }
}

void loop() {
    splitflapMqtt.loop();
    cluster.loop();

    // check what mode the display is in, this value is updated by the web server
    int mode = webServer.getMode();
    
    switch (mode) {
        // case 0: singleInputMode(); break;
        // case 1: multiInputMode(); break;
        // case 2: dateMode(); break;
        // case 3: timeMode(); break;
        // case 4: break;
        // case 5: randomTest(); break;
        case 7: perDisplayMode(); break;
        case 8: allDisplayTestMode(); break;
        default: break;
    }

    webServer.handleOta();
    checkConnection();

    reconnectIfNeeded();

    webServer.checkRebootRequired();
    yield();
}

void singleInputMode() {
    String userInput = webServer.getInputString();
    if (userInput != webServer.getWrittenString()) {
        display.writeString(userInput, MAX_RPM, webServer.getCentering());
        webServer.setWrittenString(userInput);
    }
}

void multiInputMode() {
    if (millis() - webServer.getLastSwitchMultiTime() > webServer.getMultiWordDelay()) {
        // get user input, extract correct word from index using webserver counter, and display
        String userInput = webServer.getMultiInputString();
        String currWord = extractFromCSV(userInput, webServer.getMultiWordCurrentIndex());
        if (currWord != webServer.getWrittenString()) {
            display.writeString(currWord, MAX_RPM, webServer.getCentering());
            webServer.setWrittenString(currWord);
        }
        webServer.setLastSwitchMultiTime(millis());
        webServer.setMultiWordCurrentIndex((webServer.getMultiWordCurrentIndex() + 1) % (webServer.getNumMultiWords()));
    }
}

void dateMode() {
    if (millis() - webServer.getLastCheckDateTime() > webServer.getDateCheckInterval()) {
        webServer.setLastCheckDateTime(millis());

        String format = settings.getString("dateFormat");
        String strftimeFormat = convertToStrftime(format);
        String result = renderDate(strftimeFormat);

        if (result.length() <= display.getNumModules() && result != webServer.getWrittenString()) {
            display.writeString(result, MAX_RPM);
            webServer.setWrittenString(result);
        }
    }
}

void timeMode() {
    if (millis() - webServer.getLastCheckDateTime() > webServer.getDateCheckInterval()) {
        webServer.setLastCheckDateTime(millis());

        // Get user-friendly format from settings (fallback to "HH:mm")
        String userFormat = settings.getString("timeFormat").length() > 0 ? settings.getString("timeFormat") : "HH:mm";

        // Convert to strftime-compatible format
        String strftimeFormat = convertToStrftime(userFormat);
        String result = renderTime(strftimeFormat);

        // Write to display if it changed
        if (result != webServer.getWrittenString()) {
            display.writeString(result, MAX_RPM);
            webServer.setWrittenString(result);
        }
    }
}

void randomTest() {
    display.testRandom();
    delay(2500);
}

void perDisplayMode() {
    if (webServer.hasDisplayTextsUpdated()) {
        if (cluster.isWorker()) {
            // Workers receive display commands from main via PREPARE/GO,
            // not from their local web UI.
            webServer.clearDisplayTextsUpdated();
            return;
        }

        // Build texts array indexed by logical display (0-based global).
        // In standalone mode numTexts == local display count.
        // In main cluster mode numTexts == full cluster display count
        // (grows as workers check in via pong).
        int numTexts = cluster.isStandalone()
                           ? display.getNumDisplays()
                           : cluster.getTotalDisplayCount();
        numTexts = max(numTexts, 1);  // guard against zero before workers pong

        String* texts = new String[numTexts];
        for (int i = 0; i < numTexts; i++) {
            texts[i] = (i < 8) ? webServer.getDisplayText(i) : String("");
        }

        cluster.distributeWrite(texts, numTexts, MAX_RPM, webServer.getDisplayCentering());
        delete[] texts;
        webServer.clearDisplayTextsUpdated();
    }
}

// Mode 8: All Display Test - cycles through all characters on all modules
void allDisplayTestMode() {
    // Character set in drum order: space, A-Z, 0-9
    static const char testChars[37] = {
        ' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };
    const int numChars = 37;
    
    if (millis() - webServer.getLastTestModeTime() > webServer.getTestModeDelay()) {
        int charIndex = webServer.getTestModeCharIndex();
        char currentChar = testChars[charIndex];
        
        Serial.printf("[TEST MODE] Displaying character: '%c' (index %d)\n", currentChar, charIndex);
        display.writeChar(currentChar, MAX_RPM);
        webServer.setTestModeCurrentChar(currentChar);
        
        // Advance to next character with skip
        int skip = webServer.getTestModeSkip();
        int nextIndex = (charIndex + skip) % numChars;
        if (charIndex + skip >= numChars) {
            webServer.setTestModeCycleCount(webServer.getTestModeCycleCount() + 1);
        }
        charIndex = nextIndex;
        webServer.setTestModeCharIndex(charIndex);
        webServer.setLastTestModeTime(millis());
    }
}

void checkConnection() {
    if (millis() - webServer.getLastCheckWifiTime() >
        webServer.getWifiCheckInterval()) { // check wifi to see if disconnected
        webServer.checkWiFi();
        webServer.setLastCheckWifiTime(millis());
    }
}

void reconnectIfNeeded() {
    if (webServer.getAttemptReconnect()) { // check if the device should attempt reconnection to wifi
        webServer.setAttemptReconnect(false);
        display.writeString("");
        if (! webServer.connectToWifi()) {
            webServer.startAccessPoint();
            webServer.enableOta();
            webServer.endMDNS();
            webServer.startMDNS();
            display.writeChar('X');
        } else {
            webServer.enableOta();
            webServer.endMDNS();
            webServer.startMDNS();
            display.writeString("OK");
            webServer.setWrittenString("OK");
            delay(500);
            display.writeString("");
            webServer.setWrittenString("");
        }

        splitflapMqtt.setup();
    }
}

String extractFromCSV(String str, int index) {
    int startIndex = 0;
    int endIndex = str.length();

    int commaCount = 0;
    for (int i = 0; i < str.length(); i++) {
        if (str[i] == ',') {
            commaCount++;
            if (commaCount == index) {
                startIndex = i + 1; // skip past the comma
            } else if (commaCount == index + 1) {
                endIndex = i;
            }
        }
    }

    return str.substring(startIndex, endIndex);
}

String renderDate(const String &format) {
    char buf[64];
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    strftime(buf, sizeof(buf), format.c_str(), timeinfo);

    return trimToModuleCount(String(buf), display.getNumModules());
}

String renderTime(const String &format) {
    char buf[64];
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    strftime(buf, sizeof(buf), format.c_str(), timeinfo);

    return trimToModuleCount(String(buf), display.getNumModules());
}

String trimToModuleCount(const String &str, int maxLen) {
    return str.length() > maxLen ? str.substring(0, maxLen) : str;
}

String convertToStrftime(String userFormat) {
    struct FormatToken
    {
        const char *token;
        const char *strftime;
    };

    FormatToken tokens[] = {
        // Date formats
        {"{yyyy}", "%Y"}, // 4-digit year (e.g. 2025)
        {"{dddd}", "%A"}, // Full weekday name (e.g. Monday)
        {"{mmmm}", "%B"}, // Full month name (e.g. January)
        {"{ddd}", "%a"},  // Abbreviated weekday name (e.g. Mon)
        {"{mmm}", "%b"},  // Abbreviated month name (e.g. Apr)
        {"{dd}", "%d"},   // 2-digit day of month, zero-padded (01–31)
        {"{mm}", "%m"},   // 2-digit month number, zero-padded (01–12)
        {"{yy}", "%y"},   // 2-digit year (e.g. 25)
        {"{ww}", "%V"},   // ISO 8601 week number (01–53)
        {"{D}", "%j"},    // Day of the year (001–366)

        // Time formats
        {"{HH}", "%H"},   // Hours (24-hour clock, 00–23)
        {"{hh}", "%I"},   // Hours (12-hour clock, 01–12)
        {"{MM}", "%M"},   // Minutes (00–59)
        {"{AMPM}", "%p"}, // AM or PM
    };

    for (auto &t : tokens) {
        userFormat.replace(t.token, t.strftime);
    }

    return userFormat;
}
