// Split Flap Display
// Morgan Manly 02/16/2025
// Jordan Hoff 03/25/2025
// Thom Koopman 03/30/2025

// Enjoy :)
#include "DisplayCommand.h"
#include "JsonSettings.h"
#include "SplitFlapDisplay.h"
#include "SplitFlapMqtt.h"
#include "SplitFlapWebServer.h"

#include <Arduino.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <esp_log.h>

// Forward declarations
void randomMode(SplitFlapDisplay &display, int displayNum);
void display1LoopTask(void *parameter);
void display2LoopTask(void *parameter);

// FreeRTOS task handles for parallel homing
TaskHandle_t homeDisplay1TaskHandle = NULL;
TaskHandle_t homeDisplay2TaskHandle = NULL;
volatile bool display1HomeComplete = false;
volatile bool display2HomeComplete = false;

// Persistent task handles for parallel operation
TaskHandle_t display1LoopTaskHandle = NULL;
TaskHandle_t display2LoopTaskHandle = NULL;

// Command queues for non-blocking display updates
QueueHandle_t display1Queue = NULL;
QueueHandle_t display2Queue = NULL;

// clang-format off
JsonSettings settings = JsonSettings("config", {
    // General Settings
    {"name", JsonSetting("My Display")},
    {"mdns", JsonSetting("splitflap")},
    {"otaPass", JsonSetting("")},
    {"timezone", JsonSetting("UTC0")},
    {"dateFormat", JsonSetting("{dd}-{mm}-{yy}")},
    {"timeFormat", JsonSetting("{HH}:{MM}")},
    // Wifi Settings
    {"ssid", JsonSetting("")},
    {"password", JsonSetting("")},
    // MQTT Settings
    {"mqtt_server", JsonSetting("")},
    {"mqtt_port", JsonSetting(1883)},
    {"mqtt_user", JsonSetting("")},
    {"mqtt_pass", JsonSetting("")},
    // Display Enable/Disable
    {"d1_enabled", JsonSetting(true)},
    {"d2_enabled", JsonSetting(true)},
    // Legacy shared hardware settings (deprecated, kept for compatibility)
    {"magnetPosition", JsonSetting(730)},
    {"stepsPerRot", JsonSetting(2048)},
    {"maxVel", JsonSetting(15.0f)},
    {"charset", JsonSetting(37)},
    // Display 1 (Wire/Bus 0) Hardware Settings - GPIO 21/22, I2C addresses
    {"d1_sdaPin", JsonSetting(21)},
    {"d1_sclPin", JsonSetting(22)},
    {"d1_magnetPos", JsonSetting(730)},
    {"d1_stepsRot", JsonSetting(2048)},
    {"d1_maxVel", JsonSetting(15.0f)},
    {"d1_modCnt", JsonSetting(8)},
    {"d1_modAddrs", JsonSetting(std::vector<int>{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27})},
    {"d1_modOffs", JsonSetting(std::vector<int>{0, 0, 0, 0, 0, 0, 0, 0})},
    {"d1_dispOffs", JsonSetting(0)},
    // Display 2 (Wire1/Bus 1) Hardware Settings - GPIO 16/17, I2C addresses
    {"d2_sdaPin", JsonSetting(16)},
    {"d2_sclPin", JsonSetting(17)},
    {"d2_magnetPos", JsonSetting(730)},
    {"d2_stepsRot", JsonSetting(2048)},
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

// Task to home Display 1 (receives SplitFlapDisplay pointer as parameter)
void homeDisplay1Task(void *parameter) {
    SplitFlapDisplay *display = (SplitFlapDisplay *)parameter;
    display->homeToString("D1");
    delay(250);
    display->writeString("");
    display1HomeComplete = true;
    vTaskDelete(NULL); // Delete this task when done
}

// Task to home Display 2 (receives SplitFlapDisplay pointer as parameter)
void homeDisplay2Task(void *parameter) {
    SplitFlapDisplay *display = (SplitFlapDisplay *)parameter;
    display->homeToString("D2");
    delay(250);
    display->writeString("");
    display2HomeComplete = true;
    vTaskDelete(NULL); // Delete this task when done
}

// Persistent task for Display 1 operations (runs continuously on Core 0)
void display1LoopTask(void *parameter) {
    while (true) {
        bool d1Enabled = settings.getInt("d1_enabled") != 0;
        int d1Mode = settings.getInt("d1_mode");
        
        // Check for queued text commands
        DisplayCommand cmd;
        if (xQueueReceive(display1Queue, &cmd, 0) == pdTRUE) {
            Serial.println("Display 1: Processing queued text update");
            display1.writeString(cmd.text, MAX_RPM, cmd.centerText);
        } else if (d1Enabled) {
            // Only run mode logic if no queued commands
            switch (d1Mode) {
                case 4: break; // Manual control via /text endpoint
                case 5: randomMode(display1, 1); break;
                default: break;
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent task starvation
    }
}

// Persistent task for Display 2 operations (runs continuously on Core 1)
void display2LoopTask(void *parameter) {
    while (true) {
        bool d2Enabled = settings.getInt("d2_enabled") != 0;
        int d2Mode = settings.getInt("d2_mode");
        
        // Check for queued text commands
        DisplayCommand cmd;
        if (xQueueReceive(display2Queue, &cmd, 0) == pdTRUE) {
            Serial.println("Display 2: Processing queued text update");
            display2.writeString(cmd.text, MAX_RPM, cmd.centerText);
        } else if (d2Enabled) {
            // Only run mode logic if no queued commands
            switch (d2Mode) {
                case 4: break; // Manual control via /text endpoint
                case 5: randomMode(display2, 2); break;
                default: break;
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent task starvation
    }
}

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

    // Initialize settings with defaults if this is first boot
    // This prevents INVALID_HANDLE errors when toJson() is called before any settings are saved
    Preferences prefs;
    prefs.begin("config", true);  // Read-only check
    bool hasSettings = prefs.isKey("name");  // Check if any setting exists
    prefs.end();
    
    if (!hasSettings) {
        Serial.println("First boot detected - initializing settings with defaults");
        settings.reset();  // This will write all default values to NVS
    }

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
        
        // Create command queues for non-blocking display updates
        display1Queue = xQueueCreate(5, sizeof(DisplayCommand));
        display2Queue = xQueueCreate(5, sizeof(DisplayCommand));
        
        if (display1Queue == NULL || display2Queue == NULL) {
            Serial.println("ERROR: Failed to create display queues!");
        } else {
            Serial.println("Display command queues created");
            // Pass queues to web server for non-blocking updates
            webServer.setDisplayQueues(display1Queue, display2Queue);
        }
        
        webServer.startWebServer();

        // Initialize both displays
        display1.init();
#if defined(SDA2_PIN) && defined(SCL2_PIN)
        display2.init();
#endif

        splitflapMqtt.setup();
        splitflapMqtt.setDisplay(&display1);
        display1.setMqtt(&splitflapMqtt);

        // Home both displays in parallel using FreeRTOS tasks\
        bool d1Enabled = settings.getInt("d1_enabled") != 0;
        bool d2Enabled = settings.getInt("d2_enabled") != 0;
        
        // Create homing tasks
        if (d1Enabled) {
            xTaskCreatePinnedToCore(
                homeDisplay1Task,           // Task function
                "HomeDisplay1",             // Name
                4096,                       // Stack size (bytes)
                &display1,                  // Pass display1 as parameter
                1,                          // Priority
                &homeDisplay1TaskHandle,    // Task handle
                0                           // Core 0
            );
        } else {
            display1HomeComplete = true; // Mark as complete if disabled
        }
        
#if defined(SDA2_PIN) && defined(SCL2_PIN)
        if (d2Enabled) {
            xTaskCreatePinnedToCore(
                homeDisplay2Task,           // Task function
                "HomeDisplay2",             // Name
                4096,                       // Stack size (bytes)
                &display2,                  // Pass display2 as parameter
                1,                          // Priority
                &homeDisplay2TaskHandle,    // Task handle
                1                           // Core 1
            );
        } else {
            display2HomeComplete = true; // Mark as complete if disabled
        }
#else
        display2HomeComplete = true; // Mark as complete if not compiled
#endif

        // Wait for both homing tasks to complete
        while (!display1HomeComplete || !display2HomeComplete) {
            delay(100);
        }
        
        Serial.println("Homing Complete\n");
        
        // Create persistent tasks for parallel display operation
        Serial.println("Creating persistent display tasks...");
        
        xTaskCreatePinnedToCore(
            display1LoopTask,           // Task function
            "Display1Loop",             // Name
            8192,                       // Stack size (bytes) - larger for continuous operation
            NULL,                       // Parameters
            1,                          // Priority
            &display1LoopTaskHandle,    // Task handle
            0                           // Core 0
        );
        
        xTaskCreatePinnedToCore(
            display2LoopTask,           // Task function
            "Display2Loop",             // Name
            8192,                       // Stack size (bytes)
            NULL,                       // Parameters
            1,                          // Priority
            &display2LoopTaskHandle,    // Task handle
            1                           // Core 1
        );
        
        Serial.println("Display tasks created - operating in parallel mode\n");
    }
}

void loop() {
    splitflapMqtt.loop();

    // Display mode handling is now done in parallel tasks (display1LoopTask and display2LoopTask)
    // This loop only handles web server, OTA, and WiFi management

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

void randomMode(SplitFlapDisplay &display, int displayNum) {
    // Use separate timing variables for each display
    static unsigned long lastRandomD1 = 0;
    static unsigned long lastRandomD2 = 0;
    
    unsigned long &lastRandom = (displayNum == 1) ? lastRandomD1 : lastRandomD2;
    
    if (millis() - lastRandom > 2500) {
        display.testRandom();
        lastRandom = millis();
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
