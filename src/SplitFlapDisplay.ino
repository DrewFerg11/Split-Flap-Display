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
    // Hardware Settings - Wire (Bus 1)
    {"wireAddresses", JsonSetting({0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27})},
    {"wireOffsets",   JsonSetting({0, 0, 0, 0, 0, 0, 0, 0})},
    {"sdaPin",        JsonSetting(SDA_PIN)},
    {"sclPin",        JsonSetting(SCL_PIN)},
#ifdef ENABLE_DUAL_I2C
    // Hardware Settings - Wire1 (Bus 2)
    {"wire1Addresses", JsonSetting({0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27})},
    {"wire1Offsets",   JsonSetting({0, 0, 0, 0, 0, 0, 0, 0})},
    {"sda2Pin",        JsonSetting(SDA2_PIN)},
    {"scl2Pin",        JsonSetting(SCL2_PIN)},
#endif
    {"magnetPosition", JsonSetting(730)},
    {"displayOffset", JsonSetting(0)},
    {"stepsPerRot", JsonSetting(2048)},
    {"maxVel", JsonSetting(15.0f)},
    {"charset", JsonSetting(37)},
    // Operational States
    {"mode", JsonSetting(0)}
});
// clang-format on

WiFiClient wifiClient;
SplitFlapDisplay display(settings);
SplitFlapWebServer webServer(settings);
SplitFlapMqtt splitflapMqtt(settings, wifiClient);

// Forward declarations for functions defined after loop()
void singleInputMode();
void multiInputMode();
void dateMode();
void timeMode();
void randomTest();
void accuracyTestMode();
void checkConnection();
void reconnectIfNeeded();
#ifdef ENABLE_DUAL_I2C
void dualSingleInputMode();
void dualMultiInputMode();
void dateTimeMode();
#endif

void setup() {
    // put your setup code here, to run once:
    Serial.begin(SERIAL_SPEED);

#ifdef STARTUP_DELAY
    delay(STARTUP_DELAY);
#endif

    Serial.println("Init Web Server");
    webServer.init();

    if (! webServer.connectToWifi()) {
        webServer.startAccessPoint();
        webServer.enableOta();
        webServer.startMDNS();
        webServer.startWebServer();

        display.init();
        display.homeToString("");

        if (display.getNumModules() == 8) {
            display.writeString("Wifi Err");
        } else {
            display.writeChar('X');
        }
    } else {
        webServer.enableOta();
        webServer.startMDNS();
        webServer.startWebServer();

        display.init();
        splitflapMqtt.setDisplay(&display);
        splitflapMqtt.setup();
        display.setMqtt(&splitflapMqtt);

#ifdef ENABLE_DUAL_I2C
        if (display.getWire1Count() > 0) {
            display.homeToStringDual("OK", "");
        } else {
            display.homeToString("OK");
        }
#else
        display.homeToString("OK");
#endif
        delay(250);
        display.writeString("");
    }
}

void loop() {
    splitflapMqtt.loop();

    // check what mode the display is in, this value is updated by the web server
    switch (webServer.getMode()) {
        case 0: singleInputMode(); break;
        case 1: multiInputMode(); break;
        case 2: dateMode(); break;
        case 3: timeMode(); break;
        case 4: break;
        case 5: randomTest(); break;
        case 9: accuracyTestMode(); break;
#ifdef ENABLE_DUAL_I2C
        case 7: dualSingleInputMode(); break;
        case 8: dualMultiInputMode(); break;
        case 10: dateTimeMode(); break;
#endif
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
    if (userInput.isEmpty()) return;
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
#ifdef ENABLE_DUAL_I2C
            if (display.getWire1Count() > 0) {
                display.writeStringDual(result, "", MAX_RPM, true);
            } else {
                display.writeString(result, MAX_RPM);
            }
#else
            display.writeString(result, MAX_RPM);
#endif
            webServer.setWrittenString(result);
        }
    }
}

void timeMode() {
    if (millis() - webServer.getLastCheckDateTime() > webServer.getDateCheckInterval()) {
        webServer.setLastCheckDateTime(millis());

        String userFormat = settings.getString("timeFormat").length() > 0 ? settings.getString("timeFormat") : "HH:mm";
        String result = renderTime(convertToStrftime(userFormat));

        if (result != webServer.getWrittenString()) {
#ifdef ENABLE_DUAL_I2C
            if (display.getWire1Count() > 0) {
                display.writeStringDual(result, "", MAX_RPM, true);
            } else {
                display.writeString(result, MAX_RPM);
            }
#else
            display.writeString(result, MAX_RPM);
#endif
            webServer.setWrittenString(result);
        }
    }
}

void randomTest() {
    display.testRandom();
    delay(2500);
}

#ifdef ENABLE_DUAL_I2C
void dateTimeMode() {
    if (millis() - webServer.getLastCheckDateTime() > webServer.getDateCheckInterval()) {
        webServer.setLastCheckDateTime(millis());

        String timeUserFormat =
            settings.getString("timeFormat").length() > 0 ? settings.getString("timeFormat") : "{HH}:{MM}";
        String timeResult = renderTime(convertToStrftime(timeUserFormat));
        String dateResult = renderDate(convertToStrftime(settings.getString("dateFormat")));

        String combined = timeResult + "|" + dateResult;
        if (combined != webServer.getWrittenString()) {
            display.writeStringDual(timeResult, dateResult, MAX_RPM, true);
            webServer.setWrittenString(combined);
        }
    }
}

void dualSingleInputMode() {
    String row1 = webServer.getDualRow1String();
    String row2 = webServer.getDualRow2String();
    String combined = row1 + "|" + row2;
    if (combined != webServer.getWrittenString()) {
        display.writeStringDual(row1, row2, MAX_RPM, webServer.getCentering());
        webServer.setWrittenString(combined);
    }
}

void dualMultiInputMode() {
    if (millis() - webServer.getLastSwitchMultiTime() > webServer.getMultiWordDelay()) {
        String userInput = webServer.getMultiInputString();
        int idx = webServer.getMultiWordCurrentIndex();
        String row1 = extractFromCSV(userInput, idx);
        String row2 = (idx + 1 < webServer.getNumMultiWords()) ? extractFromCSV(userInput, idx + 1) : "";
        String combined = row1 + "|" + row2;
        if (combined != webServer.getWrittenString()) {
            display.writeStringDual(row1, row2, MAX_RPM, webServer.getCentering());
            webServer.setWrittenString(combined);
        }
        webServer.setLastSwitchMultiTime(millis());
        int nextIdx = idx + 2;
        if (nextIdx >= webServer.getNumMultiWords()) nextIdx = 0;
        webServer.setMultiWordCurrentIndex(nextIdx);
    }
}
#endif

void accuracyTestMode() {
    int charSetSize = display.getCharsetSize();
    const char *charSet = (charSetSize == 48) ? SplitFlapModule::ExtendedChars : SplitFlapModule::StandardChars;
    unsigned long now = millis();
    if (now - webServer.getLastAccuracyStepTime() >= (unsigned long) webServer.getAccuracyDelay()) {
        int idx = webServer.getAccuracyCharIndex();
        display.writeChar(charSet[idx]);
        webServer.setAccuracyCharIndex((idx + webServer.getAccuracyStepSize()) % charSetSize);
        webServer.setLastAccuracyStepTime(now);
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
