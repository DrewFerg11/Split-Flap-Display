// Split Flap Display
// Morgan Manly 02/16/2025
// Jordan Hoff 03/25/2025
// Thom Koopman 03/30/2025

// Enjoy :)
#include "JsonSettings.h"
#include "SplitFlapDisplay.h"
#include "SplitFlapMqtt.h"
#include "SplitFlapWebServer.h"

#include <Arduino.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <esp_log.h>

// clang-format off
JsonSettings settings = JsonSettings("config", {
    // General Settings
    {"name", JsonSetting("My Display")},
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
    // Legacy shared hardware settings (deprecated, kept for compatibility)
    {"magnetPosition", JsonSetting(730)},
    {"stepsPerRot", JsonSetting(2048)},
    {"maxVel", JsonSetting(15.0f)},
    {"charset", JsonSetting(37)},
    // Display 1 (Wire/Bus 0) Hardware Settings - GPIO 21/22, I2C addresses
    {"d1_sdaPin", JsonSetting(21)},
    {"d1_sclPin", JsonSetting(22)},
    {"d1_magnetPosition", JsonSetting(730)},
    {"d1_stepsPerRot", JsonSetting(2048)},
    {"d1_maxVel", JsonSetting(15.0f)},
    {"d1_modCnt", JsonSetting(8)},
    {"d1_modAddrs", JsonSetting(std::vector<int>{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27})},
    {"d1_modOffs", JsonSetting(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0})},
    {"d1_dispOffs", JsonSetting(0)},
    // Display 2 (Wire1/Bus 1) Hardware Settings - GPIO 16/17, I2C addresses
    {"d2_sdaPin", JsonSetting(16)},
    {"d2_sclPin", JsonSetting(17)},
    {"d2_magnetPosition", JsonSetting(730)},
    {"d2_stepsPerRot", JsonSetting(2048)},
    {"d2_maxVel", JsonSetting(15.0f)},
    {"d2_modCnt", JsonSetting(8)},
    {"d2_modAddrs", JsonSetting(std::vector<int>{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27})},
    {"d2_modOffs", JsonSetting(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0})},
    {"d2_dispOffs", JsonSetting(0)},
    // Per-Display Modes (4 = inactive/manual control)
    {"d1_mode", JsonSetting(4)},
    {"d2_mode", JsonSetting(4)},
    // Legacy global mode (deprecated, kept for compatibility)
    {"mode", JsonSetting(4)}
});
// clang-format on

WiFiClient wifiClient;
SplitFlapDisplay display1(settings, Wire, 0);
SplitFlapDisplay display2(settings, Wire1, 1);
SplitFlapWebServer webServer(settings, display1, display2);
SplitFlapMqtt splitflapMqtt(settings, wifiClient);

// I2C Bus Scanner - logs all devices found on specified bus
void scanI2CBus(TwoWire &wire, const char* busName, uint8_t sdaPin, uint8_t sclPin) {
    Serial.printf("\n=== Scanning I2C Bus: %s (SDA=%d, SCL=%d) ===\n", busName, sdaPin, sclPin);
    Serial.flush();
    
    int devicesFound = 0;
    // Scan only the range where PCF8575 modules are expected (0x20-0x27)
    for (uint8_t address = 0x20; address <= 0x27; address++) {        
        wire.beginTransmission(address);
        byte error = wire.endTransmission(true);  // Send stop bit
        
        if (error == 0) {
            Serial.printf("  Device found at address 0x%02X\n", address);
            devicesFound++;
        } else if (error == 4) {
            Serial.printf("  Unknown error at address 0x%02X\n", address);
        }
        Serial.flush();
        delay(5);  // Delay between scans to prevent bus lockup
    }
    
    if (devicesFound == 0) {
        Serial.printf("  No I2C devices found on %s\n", busName);
    } else {
        Serial.printf("  Total devices found on %s: %d\n", busName, devicesFound);
    }
    Serial.println("==========================================\n");
    Serial.flush();
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(SERIAL_SPEED);

#ifdef STARTUP_DELAY
    delay(STARTUP_DELAY);
#endif

    // Suppress NVS error messages (they're expected when settings schema changes)
    esp_log_level_set("Preferences", ESP_LOG_NONE);
    esp_log_level_set("nvs", ESP_LOG_NONE);
    esp_log_level_set("*", ESP_LOG_WARN);  // Set all components to WARNING level or higher

    Serial.println("\n=== Split-Flap Display Startup ===");
    
    // Initialize I2C Bus 0 (Wire) - Display 1
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);  // Reduced from 400kHz to 100kHz for stability
    Serial.printf("Initialized I2C Bus 0 (Wire): SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
    
    // Initialize I2C Bus 1 (Wire1) - Display 2
#if defined(SDA2_PIN) && defined(SCL2_PIN)
    Wire1.begin(SDA2_PIN, SCL2_PIN);
    Wire1.setClock(100000);  // Reduced from 400kHz to 100kHz for stability
    Serial.printf("Initialized I2C Bus 1 (Wire1): SDA=%d, SCL=%d\n", SDA2_PIN, SCL2_PIN);
#else
    Serial.println("WARNING: SDA2_PIN/SCL2_PIN not defined - Display 2 disabled");
#endif

    // Scan both I2C buses for connected devices
    scanI2CBus(Wire, "Wire (Display 1)", SDA_PIN, SCL_PIN);
#if defined(SDA2_PIN) && defined(SCL2_PIN)
    scanI2CBus(Wire1, "Wire1 (Display 2)", SDA2_PIN, SCL2_PIN);
#endif

    Serial.println("Init Web Server");
    webServer.init();

    if (! webServer.connectToWifi()) {
        webServer.startAccessPoint();
        webServer.enableOta();
        webServer.startMDNS();
        webServer.startWebServer();

        // Initialize both displays
        display1.init();
#if defined(SDA2_PIN) && defined(SCL2_PIN)
        display2.init();
#endif

        display1.homeToString("");

        if (display1.getNumModules() == 8) {
            display1.writeString("Wifi Err");
        } else {
            display1.writeChar('X');
        }
    } else {
        webServer.enableOta();
        webServer.startMDNS();
        webServer.startWebServer();

        // Initialize both displays
        display1.init();
#if defined(SDA2_PIN) && defined(SCL2_PIN)
        display2.init();
#endif

        splitflapMqtt.setup();
        splitflapMqtt.setDisplay(&display1);
        display1.setMqtt(&splitflapMqtt);

        display1.homeToString("OK");
        delay(250);

        //// // TEST CODE: Verify both displays work independently
        //// Serial.println("\n=== Testing Display Independence ===");
        
        //// // IMPORTANT: moveTo() is BLOCKING - each display must complete its movement before starting the next
        //// // Home and write to Display 1 first
        display1.writeString("");
        
#if defined(SDA2_PIN) && defined(SCL2_PIN)
        // Now home and write to Display 2 (after Display 1 completes)
        display2.homeToString("OK");  // Home Display 2
        delay(250);
        display2.writeString("");
#endif
        //// Serial.println("Test strings written - Display 1: 'BUS0', Display 2: 'BUS1'");
    }
}

void loop() {
    splitflapMqtt.loop();

    // Check per-display modes
    int d1Mode = settings.getInt("d1_mode");
    int d2Mode = settings.getInt("d2_mode");
    
    // Handle Display 1 mode
    switch (d1Mode) {
        case 0: singleInputMode(); break;
        case 1: multiInputMode(); break;
        case 2: dateMode(); break;
        case 3: timeMode(); break;
        case 4: break; // Manual control via /text endpoint
        case 5: randomTest(); break;
        default: break;
    }
    
    // Handle Display 2 mode (placeholder for future implementation)
    switch (d2Mode) {
        case 4: break; // Manual control via /text endpoint
        // Other modes not yet implemented for display2
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
        display1.writeString(userInput, MAX_RPM, webServer.getCentering());
        webServer.setWrittenString(userInput);
    }
}

void multiInputMode() {
    if (millis() - webServer.getLastSwitchMultiTime() > webServer.getMultiWordDelay()) {
        // get user input, extract correct word from index using webserver counter, and display
        String userInput = webServer.getMultiInputString();
        String currWord = extractFromCSV(userInput, webServer.getMultiWordCurrentIndex());
        if (currWord != webServer.getWrittenString()) {
            display1.writeString(currWord, MAX_RPM, webServer.getCentering());
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

        if (result.length() <= display1.getNumModules() && result != webServer.getWrittenString()) {
            display1.writeString(result, MAX_RPM);
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
            display1.writeString(result, MAX_RPM);
            webServer.setWrittenString(result);
        }
    }
}

void randomTest() {
    display1.testRandom();
    delay(2500);
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
        display1.writeString("");
        if (! webServer.connectToWifi()) {
            webServer.startAccessPoint();
            webServer.enableOta();
            webServer.endMDNS();
            webServer.startMDNS();
            display1.writeChar('X');
        } else {
            webServer.enableOta();
            webServer.endMDNS();
            webServer.startMDNS();
            display1.writeString("OK");
            webServer.setWrittenString("OK");
            delay(500);
            display1.writeString("");
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

    return trimToModuleCount(String(buf), display1.getNumModules());
}

String renderTime(const String &format) {
    char buf[64];
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);

    strftime(buf, sizeof(buf), format.c_str(), timeinfo);

    return trimToModuleCount(String(buf), display1.getNumModules());
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
