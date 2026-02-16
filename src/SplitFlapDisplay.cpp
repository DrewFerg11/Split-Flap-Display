#include "SplitFlapDisplay.h"

#include "JsonSettings.h"
#include "SplitFlapModule.h"
#include "SplitFlapMqtt.h"
#include <esp_task_wdt.h>

SplitFlapDisplay::SplitFlapDisplay(JsonSettings &settings) : settings(settings) {}

void SplitFlapDisplay::init() {
    stepsPerRot = settings.getInt("stepsPerRot");
    displayOffset = settings.getInt("displayOffset");
    magnetPosition = settings.getInt("magnetPosition");
    maxVel = settings.getFloat("maxVel");
    charSetSize = settings.getInt("charset");
    bool halfStepping = settings.getInt("halfStepping") != 0;
    
    // Double steps and magnet position for half-stepping mode
    if (halfStepping) {
        int origStepsPerRot = stepsPerRot;
        int origMagnetPosition = magnetPosition;
        int origDisplayOffset = displayOffset;
        
        stepsPerRot *= 2;      // 2048 → 4096
        magnetPosition *= 2;   // 730 → 1460
        displayOffset *= 2;    // Also double displayOffset
        
        DEBUG_PRINTF("[INIT] Half-stepping ENABLED (8-phase)\n");
        DEBUG_PRINTF("[INIT]   stepsPerRot: %d → %d\n", origStepsPerRot, stepsPerRot);
        DEBUG_PRINTF("[INIT]   magnetPosition: %d → %d\n", origMagnetPosition, magnetPosition);
        DEBUG_PRINTF("[INIT]   displayOffset: %d → %d\n", origDisplayOffset, displayOffset);
    } else {
        DEBUG_PRINTF("[INIT] Half-stepping DISABLED (4-phase)\n");
        DEBUG_PRINTF("[INIT]   stepsPerRot: %d\n", stepsPerRot);
        DEBUG_PRINTF("[INIT]   magnetPosition: %d\n", magnetPosition);
        DEBUG_PRINTF("[INIT]   displayOffset: %d\n", displayOffset);
    }

    // Configure modules based on settings (parses wire0/wire1 configs)
    configureI2cModules();

    // Initialize primary I2C bus (Wire)
    SDAPin = settings.getInt("wire0SdaPin");
    SCLPin = settings.getInt("wire0SclPin");
    Wire.begin(SDAPin, SCLPin);
    Wire.setClock(400000);
    DEBUG_PRINTF("[INIT] Wire initialized: SDA=%d, SCL=%d @ 400kHz\n", SDAPin, SCLPin);
    
    // Initialize secondary I2C bus (Wire1) if dual bus mode is enabled
    if (useDualBus) {
        SDA1Pin = settings.getInt("wire1Sda1Pin");
        SCL1Pin = settings.getInt("wire1Scl1Pin");
        Wire1.begin(SDA1Pin, SCL1Pin);
        Wire1.setClock(400000);
        DEBUG_PRINTF("[INIT] Wire1 initialized: SDA=%d, SCL=%d @ 400kHz\n", SDA1Pin, SCL1Pin);
    }
    
    // Scan modules to confirm connectivity
    scanI2cModules();

    // Create and initialize all module objects
    for (uint8_t i = 0; i < numModules; i++) {
        uint8_t muxIdx = moduleMuxes[i];
        uint8_t busNum = muxBus[muxIdx];
        TwoWire &bus = (busNum == 0) ? Wire : Wire1;
        
        modules[i] = SplitFlapModule(
            moduleAddresses[i], stepsPerRot, displayOffset, magnetPosition, charSetSize, halfStepping, bus
        );
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        modules[i].init();
    }
    
    // Summary logging
    float stepsPerChar = (float)stepsPerRot / (float)charSetSize;
    DEBUG_PRINTF("[INIT] Initialized %d modules: %.1f steps/char (%d phases)\n", 
        numModules, stepsPerChar, halfStepping ? 8 : 4);
    
    // Initialize threading for parallel dual-bus execution
    initParallelExecution();
}

void SplitFlapDisplay::testAll() {
    char testChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                          'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int numChars = sizeof(testChars) / sizeof(testChars[0]);
    int targetPositions[numModules];

    int charPos;
    for (int i = 0; i < numChars; i++) {
        // Serial.print("Target Positions: [");
        // fill array with same char

        for (int j = 0; j < numModules; j++) {
            targetPositions[j] = modules[j].getCharPosition(testChars[i]);
            // Serial.print(targetPositions[j]);
            // Serial.print(" , ");
        }
        // Serial.println("]");

        moveTo(targetPositions);
        delay(500);
    }
}

void SplitFlapDisplay::testRandom(float speed) {
    char testChars[37] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                          'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    int targetPositions[numModules];
    char randChar;

    Serial.print("Target: ");
    for (int i = 0; i < numModules; i++) {
        randChar = testChars[random(0, 37)];
        targetPositions[i] = modules[i].getCharPosition(randChar);
        Serial.print(randChar);
    }
    Serial.println(" ");
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::testCount() {
    int count = 0;
    int maxCount = pow(10, numModules);
    char targetChar;
    int targetInteger;

    int targetPositions[numModules];

    for (int i = 0; i < maxCount; i++) {
        // get each character in the count integer
        for (int j = 0; j < numModules; j++) {
            targetInteger = (i % (int) pow(10, j + 1)) / (int) pow(10, j);
            targetChar = targetInteger + '0'; // convert to char
            targetPositions[numModules - j - 1] = modules[j].getCharPosition(targetChar);
        }

        moveTo(targetPositions);
        delay(250);
    }
}

void SplitFlapDisplay::home(float speed) {
    Serial.println("Homing");
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);
    char homeChar = ' ';
    int charPosition;
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::homeToString(String homeString, float speed, bool centering) {
    Serial.println("Homing");
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);
    writeString(homeString, speed, centering);
}

void SplitFlapDisplay::homeToChar(char homeChar, float speed) {
    Serial.println("Homing");
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);

    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, true, speed);
}

void SplitFlapDisplay::writeChar(char inputChar, float speed) {
    DEBUG_PRINTF("[CMD] writeChar: '%c' to all %d modules (speed=%.1f)\n", 
        inputChar, numModules, speed);
    
    int targetPositions[numModules];
    // Iterate through the input string and process each character
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(inputChar);
    }
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::writeString(String inputString, float speed, bool centering) {
    DEBUG_PRINTF("[CMD] writeString: '%s' (speed=%.1f, centering=%s)\n", 
        inputString.c_str(), speed, centering ? "true" : "false");
    
    String displayString = inputString.substring(0, numModules);

    if (centering) {
        int totalPadding = numModules - displayString.length();
        int paddingLeft = totalPadding / 2;
        int paddingRight = totalPadding - paddingLeft;

        // Add padding to the left
        String result = "";
        for (int i = 0; i < paddingLeft; i++) {
            result += " ";
        }

        // Add the original string
        result += displayString;

        // Add padding to the right
        for (int i = 0; i < paddingRight; i++) {
            result += " ";
        }
        displayString = result;
    } else {                                          // pad blanks to end, if no centering
        while (displayString.length() < numModules) { // Pad with spaces
            displayString += " ";                     // Padding with space
        }
    }

    int targetPositions[numModules];
    // Iterate through the input string and process each character
    for (int i = 0; i < displayString.length(); i++) {
        char currentChar = displayString[i];
        // Serial.println(currentChar);
        targetPositions[i] = modules[i].getCharPosition(currentChar);
    }
    moveTo(targetPositions, speed);

    if (mqtt && mqtt->isConnected()) {
        mqtt->publishState(displayString);
    }
}

void SplitFlapDisplay::writeStringPerChannel(String channelStrings[], float speed, bool centering) {
    int targetPositions[numModules];
    
    // Build target positions for all modules across all channels
    int moduleIdx = 0;
    for (int ch = 0; ch < 8; ch++) {
        if (moduleCountPerChannel[ch] == 0) continue;  // Skip empty channels
        
        int numChannelModules = moduleCountPerChannel[ch];
        String displayString = channelStrings[ch];
        
        // Truncate if too long
        if (displayString.length() > numChannelModules) {
            displayString = displayString.substring(0, numChannelModules);
        }
        
        // Handle centering or padding for this channel
        if (centering) {
            int totalPadding = numChannelModules - displayString.length();
            int paddingLeft = totalPadding / 2;
            int paddingRight = totalPadding - paddingLeft;
            
            String result = "";
            for (int i = 0; i < paddingLeft; i++) {
                result += " ";
            }
            result += displayString;
            for (int i = 0; i < paddingRight; i++) {
                result += " ";
            }
            displayString = result;
        } else {
            // Pad with spaces to fill channel width
            while (displayString.length() < numChannelModules) {
                displayString += " ";
            }
        }
        Serial.printf("Channel %d Target: '%s'\n", ch, displayString.c_str());
        
        // Set target positions for this channel's modules
        for (int i = 0; i < numChannelModules; i++) {
            char currentChar = displayString[i];
            targetPositions[moduleIdx] = modules[moduleIdx].getCharPosition(currentChar);
            moduleIdx++;
        }
    }
    
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::writeDisplays(String displayTexts[], float speed, bool centering) {
    DEBUG_PRINTF("[CMD] writeDisplays: %d displays (speed=%.1f, centering=%s)\n", 
        numDisplays, speed, centering ? "true" : "false");
    for (int i = 0; i < numDisplays; i++) {
        DEBUG_PRINTF("  Display %d: '%s'\n", i, displayTexts[i].c_str());
    }
    
    int targetPositions[numModules];
    
    // Initialize all target positions to current positions (no movement by default)
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getPosition();
    }
    
    // Build target positions by iterating through displays
    for (int displayIdx = 0; displayIdx < numDisplays; displayIdx++) {
        uint8_t muxIdx = displayMux[displayIdx];
        uint8_t chIdx = displayChannel[displayIdx];
        int numModules = displayModuleCount[displayIdx];
        String displayString = displayTexts[displayIdx];
        
        // Truncate if too long
        if (displayString.length() > numModules) {
            displayString = displayString.substring(0, numModules);
        }
        
        // Handle centering or padding
        if (centering) {
            int totalPadding = numModules - displayString.length();
            int paddingLeft = totalPadding / 2;
            int paddingRight = totalPadding - paddingLeft;
            
            String result = "";
            for (int i = 0; i < paddingLeft; i++) {
                result += " ";
            }
            result += displayString;
            for (int i = 0; i < paddingRight; i++) {
                result += " ";
            }
            displayString = result;
        } else {
            // Pad with spaces to fill display width
            while (displayString.length() < numModules) {
                displayString += " ";
            }
        }
        
        // Set target positions for this display's modules
        // Find modules that belong to this display (matching mux and channel)
        int charIdx = 0;
        for (int i = 0; i < this->numModules; i++) {
            if (moduleMuxes[i] == muxIdx && moduleChannels[i] == chIdx) {
                char currentChar = displayString[charIdx];
                targetPositions[i] = modules[i].getCharPosition(currentChar);
                charIdx++;
            }
        }
    }
    
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::moveTo(int targetPositions[], float speed, bool releaseMotors) {
    // Copy target positions for bus 0
    if (xSemaphoreTake(bus0Mutex, portMAX_DELAY)) {
        memcpy(bus0Movement.targetPositions, targetPositions, sizeof(int) * numModules);
        bus0Movement.speed = speed;
        bus0Movement.releaseMotors = releaseMotors;
        bus0Movement.active = true;
        bus0Movement.complete = false;
        xSemaphoreGive(bus0Mutex);
    }
    
    // Copy target positions for bus 1 if dual bus is enabled
    if (useDualBus) {
        if (xSemaphoreTake(bus1Mutex, portMAX_DELAY)) {
            memcpy(bus1Movement.targetPositions, targetPositions, sizeof(int) * numModules);
            bus1Movement.speed = speed;
            bus1Movement.releaseMotors = releaseMotors;
            bus1Movement.active = true;
            bus1Movement.complete = false;
            xSemaphoreGive(bus1Mutex);
        }
    }
    
    // Wait for both buses to complete
    bool bus0Done = false;
    bool bus1Done = !useDualBus; // If not using dual bus, bus1 is always "done"
    
    while (!bus0Done || !bus1Done) {
        if (!bus0Done && xSemaphoreTake(bus0Mutex, 10 / portTICK_PERIOD_MS)) {
            bus0Done = bus0Movement.complete;
            xSemaphoreGive(bus0Mutex);
        }
        
        if (!bus1Done && xSemaphoreTake(bus1Mutex, 10 / portTICK_PERIOD_MS)) {
            bus1Done = bus1Movement.complete;
            xSemaphoreGive(bus1Mutex);
        }
        
        vTaskDelay(1); // Small delay to avoid busy-waiting
    }
}

bool SplitFlapDisplay::checkAllFalse(bool array[], int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] == true) {
            return false;              // As soon as a true value is found, return false
        }
    }
    return true;                       // All values were false
}

void SplitFlapDisplay::startMotors() {
    for (int i = 0; i < numModules; i++) {
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        modules[i].start();
    }
}

void SplitFlapDisplay::stopMotors() {
    // Serial.println("Stopping Motors");
    for (int i = 0; i < numModules; i++) {
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        modules[i].stop();
    }
}

void SplitFlapDisplay::setMqtt(SplitFlapMqtt *mqttHandler) {
    mqtt = mqttHandler;
}

// TCA9548A Multiplexer Channel Selection
void SplitFlapDisplay::selectMuxChannel(uint8_t muxIndex, uint8_t channel) {
    if (channel > 7) return;  // TCA9548A has 8 channels (0-7)
    if (muxIndex >= numMuxes) {
        Serial.printf("[ERROR] Invalid mux index %d (max %d)\n", muxIndex, numMuxes - 1);
        return;
    }
    
    uint8_t muxAddr = muxAddrs[muxIndex];
    uint8_t busNum = muxBus[muxIndex];
    TwoWire &bus = (busNum == 0) ? Wire : Wire1;
    
    // Track last mux/channel per bus (not shared across buses)
    static uint8_t lastMuxIndex[2] = {255, 255};  // One for Wire, one for Wire1
    static uint8_t lastChannel[2] = {255, 255};
    
    // Only switch if needed (optimization to avoid excessive I2C traffic)
    if (muxIndex == lastMuxIndex[busNum] && channel == lastChannel[busNum]) {
        return;  // Already on the correct mux and channel for this bus
    }
    
    // If switching mux on this bus, disable the old mux
    if (muxIndex != lastMuxIndex[busNum] && lastMuxIndex[busNum] < numMuxes) {
        // Only disable if the old mux was on the same bus
        if (muxBus[lastMuxIndex[busNum]] == busNum) {
            bus.beginTransmission(muxAddrs[lastMuxIndex[busNum]]);
            bus.write(0x00);  // Disable previous mux
            bus.endTransmission();
        }
    }
    
    // Enable the target channel on the target mux
    bus.beginTransmission(muxAddr);
    bus.write(1 << channel);  // Set bit for desired channel
    byte error = bus.endTransmission();
    if (error != 0) {
        Serial.printf("[ERROR] MUX 0x%02X channel select failed: error %d\n", muxAddr, error);
    }
    
    // Small delay to let the mux channel activate
    delayMicroseconds(10);
    
    lastMuxIndex[busNum] = muxIndex;
    lastChannel[busNum] = channel;
}

void SplitFlapDisplay::configureI2cModules() {
    // Check if dual bus mode is enabled
    useDualBus = settings.getInt("useDualBus") != 0;
    DEBUG_PRINTF("[CONFIG] Dual bus mode: %s\n", useDualBus ? "ENABLED" : "DISABLED");
    
    // Parse multiplexer addresses for Wire (bus 0)
    String wire0MuxAddrsStr = settings.getString("wire0MuxAddrs");
    numMuxes = 0;
    memset(muxAddrs, 0, sizeof(muxAddrs));
    memset(muxBus, 0, sizeof(muxBus));
    
    // Parse Wire0 mux addresses
    int start = 0;
    for (int i = 0; wire0MuxAddrsStr.length() > 0 && i <= wire0MuxAddrsStr.length() && numMuxes < 8; i++) {
        if (i == wire0MuxAddrsStr.length() || wire0MuxAddrsStr[i] == ',') {
            String addrStr = wire0MuxAddrsStr.substring(start, i);
            addrStr.trim();
            if (addrStr.length() > 0) {
                muxAddrs[numMuxes] = (uint8_t)addrStr.toInt();
                muxBus[numMuxes] = 0;  // Wire (primary bus)
                DEBUG_PRINTF("[CONFIG] Mux%d: 0x%02X on Wire (bus 0)\n", numMuxes, muxAddrs[numMuxes]);
                numMuxes++;
            }
            start = i + 1;
        }
    }
    
    // Parse Wire1 mux addresses if dual bus is enabled
    if (useDualBus) {
        String wire1MuxAddrsStr = settings.getString("wire1MuxAddrs");
        start = 0;
        for (int i = 0; wire1MuxAddrsStr.length() > 0 && i <= wire1MuxAddrsStr.length() && numMuxes < 8; i++) {
            if (i == wire1MuxAddrsStr.length() || wire1MuxAddrsStr[i] == ',') {
                String addrStr = wire1MuxAddrsStr.substring(start, i);
                addrStr.trim();
                if (addrStr.length() > 0) {
                    muxAddrs[numMuxes] = (uint8_t)addrStr.toInt();
                    muxBus[numMuxes] = 1;  // Wire1 (secondary bus)
                    DEBUG_PRINTF("[CONFIG] Mux%d: 0x%02X on Wire1 (bus 1)\n", numMuxes, muxAddrs[numMuxes]);
                    numMuxes++;
                }
                start = i + 1;
            }
        }
    }
    
    // Default to single mux at 0x70 on Wire if not configured
    if (numMuxes == 0) {
        muxAddrs[0] = 0x70;
        muxBus[0] = 0;
        numMuxes = 1;
        DEBUG_PRINTLN("[CONFIG] Using default: Mux0 at 0x70 on Wire");
    }
    
    // Load module configuration from per-mux settings
    numModules = 0;
    memset(moduleCountPerChannel, 0, sizeof(moduleCountPerChannel));
    memset(moduleMuxes, 0, sizeof(moduleMuxes));
    
    // Track displays (one per channel with modules)
    numDisplays = 0;
    memset(displayMux, 0, sizeof(displayMux));
    memset(displayChannel, 0, sizeof(displayChannel));
    memset(displayModuleCount, 0, sizeof(displayModuleCount));
    
    int flatIdx = 0;
    
    // Process each configured multiplexer
    for (int muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        // Determine which bus this mux is on and build the config key
        String busPrefix = (muxBus[muxIdx] == 0) ? "wire0" : "wire1";
        String addrsKey = busPrefix + "ChModAddrs" + String(muxAddrs[muxIdx]);
        String moduleAddrStr = settings.getString(addrsKey.c_str());
        
        DEBUG_PRINTF("[CONFIG] Loading %s for Mux%d\n", addrsKey.c_str(), muxIdx);
        
        if (moduleAddrStr.length() == 0) {
            Serial.printf("[WARN] Missing chModAddrs%d config\n", muxAddrs[muxIdx]);
            continue;
        }
        
        // Parse channel addresses and derive counts: "32;;;;;;;;;" or "32;33,34;;;;;;;"
        int chIdx = 0;
        start = 0;
        for (int i = 0; i <= moduleAddrStr.length() && chIdx < 8; i++) {
            if (i == moduleAddrStr.length() || moduleAddrStr[i] == ';') {
                String channelAddrs = moduleAddrStr.substring(start, i);
                channelAddrs.trim();
                
                if (channelAddrs.length() > 0) {
                    int addrStart = 0, addrCount = 0;
                    for (int j = 0; j <= channelAddrs.length(); j++) {
                        if (j == channelAddrs.length() || channelAddrs[j] == ',') {
                            String addrStr = channelAddrs.substring(addrStart, j);
                            addrStr.trim();
                            if (addrStr.length() > 0 && flatIdx < MAX_MODULES) {
                                moduleAddresses[flatIdx] = (uint8_t)addrStr.toInt();
                                moduleChannels[flatIdx] = chIdx;
                                moduleMuxes[flatIdx] = muxIdx;
                                numModules++;
                                flatIdx++;
                                addrCount++;
                            }
                            addrStart = j + 1;
                        }
                    }
                    // Track this as a display (one per channel with modules)
                    if (addrCount > 0 && numDisplays < 64) {
                        displayMux[numDisplays] = muxIdx;
                        displayChannel[numDisplays] = chIdx;
                        displayModuleCount[numDisplays] = addrCount;
                        numDisplays++;
                    }
                }
                chIdx++;
                start = i + 1;
            }
        }
    }

    // Print I2C configuration summary
    Serial.printf("\n=== I2C Configuration ===\n");
    for (int muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        bool muxHasModules = false;
        for (int i = 0; i < numModules; i++) {
            if (moduleMuxes[i] == muxIdx) {
                muxHasModules = true;
                break;
            }
        }
        
        if (muxHasModules) {
            const char* busName = (muxBus[muxIdx] == 0) ? "Wire" : "Wire1";
            char muxName[16];
            snprintf(muxName, sizeof(muxName), "%s_Mux%d", busName, muxAddrs[muxIdx] - 0x70);
            Serial.printf("%-10s (0x%02X): ", muxName, muxAddrs[muxIdx]);
            for (int ch = 0; ch < 8; ch++) {
                bool first = true;
                for (int i = 0; i < numModules; i++) {
                    if (moduleMuxes[i] == muxIdx && moduleChannels[i] == ch) {
                        if (first) {
                            Serial.printf("Ch%d[", ch);
                            first = false;
                        } else {
                            Serial.print(",");
                        }
                        Serial.printf("0x%02X", moduleAddresses[i]);
                    }
                }
                if (!first) Serial.print("] ");
            }
            Serial.println();
        }
    }
    Serial.println("=========================");
}

void SplitFlapDisplay::scanI2cModules() {
    Serial.println("\n=== I2C Scanner ===");
    
    // Scan each bus separately
    for (int busNum = 0; busNum < 2; busNum++) {
        // Skip Wire1 if dual bus is not enabled
        if (busNum == 1 && !useDualBus) continue;
        
        TwoWire &bus = (busNum == 0) ? Wire : Wire1;
        const char* busName = (busNum == 0) ? "Wire" : "Wire1";
        
        // Scan all 8 possible TCA9548A addresses on this bus
        for (uint8_t addr = 0x70; addr <= 0x77; addr++) {
            bus.beginTransmission(addr);
            if (bus.endTransmission() == 0) {
                // Check if this is a configured mux on this bus
                bool isConfigured = false;
                uint8_t configIdx = 0;
                for (uint8_t i = 0; i < numMuxes; i++) {
                    if (muxAddrs[i] == addr && muxBus[i] == busNum) {
                        isConfigured = true;
                        configIdx = i;
                        break;
                    }
                }
                
                // Print in same format as config output
                char muxName[16];
                snprintf(muxName, sizeof(muxName), "%s_Mux%d", busName, addr - 0x70);
                Serial.printf("%-10s (0x%02X)", muxName, addr);
                if (!isConfigured) {
                    Serial.print(" (found)");
                }
                Serial.print(": ");
                
                // Scan all channels and print inline
                bool foundAny = false;
                for (uint8_t channel = 0; channel < 8; channel++) {
                    // Disable ALL multiplexers on this bus first to prevent crosstalk
                    for (uint8_t a = 0x70; a <= 0x77; a++) {
                        bus.beginTransmission(a);
                        bus.write(0x00);
                        bus.endTransmission();
                    }
                    delay(10);
                    
                    // Enable only the target channel on the target mux
                    bus.beginTransmission(addr);
                    bus.write(1 << channel);
                    bus.endTransmission();
                    delay(10);
                    
                    // Check if this channel is configured for this mux
                    bool channelConfigured = false;
                    if (isConfigured) {
                        for (int i = 0; i < numModules; i++) {
                            if (moduleMuxes[i] == configIdx && moduleChannels[i] == channel) {
                                channelConfigured = true;
                                break;
                            }
                        }
                    }
                    
                    // Scan for devices on this channel
                    bool foundOnChannel = false;
                    for (uint8_t devAddr = 0x08; devAddr <= 0x77; devAddr++) {
                        // Skip mux addresses
                        bool isMuxAddr = false;
                        for (uint8_t i = 0; i < numMuxes; i++) {
                            if (devAddr == muxAddrs[i]) {
                                isMuxAddr = true;
                                break;
                            }
                        }
                        if (isMuxAddr) continue;
                        
                        bus.beginTransmission(devAddr);
                        if (bus.endTransmission() == 0) {
                            if (!foundOnChannel) {
                                if (foundAny) Serial.print(" ");
                                Serial.printf("Ch%d[", channel);
                                foundOnChannel = true;
                                foundAny = true;
                            } else {
                                Serial.print(",");
                            }
                            Serial.printf("0x%02X", devAddr);
                        }
                    }
                    
                    if (foundOnChannel) {
                        Serial.print("]");
                        if (!channelConfigured) {
                            Serial.print(" (found)");
                        }
                    }
                }
                
                // Disable all multiplexers on this bus after scan
                for (uint8_t a = 0x70; a <= 0x77; a++) {
                    bus.beginTransmission(a);
                    bus.write(0x00);
                    bus.endTransmission();
                }
                
                Serial.println();
            }
        }
    }
    
    Serial.println("===================\n");
}

// Initialize FreeRTOS tasks for parallel dual-bus execution
void SplitFlapDisplay::initParallelExecution() {
    // Initialize movement structs
    bus0Movement.active = false;
    bus0Movement.complete = true;
    bus1Movement.active = false;
    bus1Movement.complete = true;
    
    // Create mutexes for thread-safe access
    bus0Mutex = xSemaphoreCreateMutex();
    bus1Mutex = xSemaphoreCreateMutex();
    
    // Create bus 0 task on core 0
    xTaskCreatePinnedToCore(
        bus0TaskFunction,
        "Bus0Task",
        4096,
        this,
        1,
        &bus0TaskHandle,
        0  // Core 0
    );
    DEBUG_PRINTLN("[INIT] Bus0Task created on Core 0");
    
    // Create bus 1 task on core 1 if dual bus is enabled
    if (useDualBus) {
        xTaskCreatePinnedToCore(
            bus1TaskFunction,
            "Bus1Task",
            4096,
            this,
            1,
            &bus1TaskHandle,
            1  // Core 1
        );
        DEBUG_PRINTLN("[INIT] Bus1Task created on Core 1");
    }
    
    DEBUG_PRINTLN("[INIT] Parallel execution initialized");
}

// Home all active displays in parallel
void SplitFlapDisplay::homeAllChannels(float speed, bool quickHome) {
    // Phase 1: Trigger homing for all modules
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, true);  // Release motors after homing to prevent overshoot
    delay(1000);
    
    // Skip phases 2 and 3 if quickHome is enabled
    if (quickHome) {
        return;
    }
    
    // Phase 2: Display labels on each configured display
    String* displayStrings = new String[numDisplays];
    for (int i = 0; i < numDisplays; i++) {
        int moduleCount = displayModuleCount[i];
        if (moduleCount == 1) {
            // Single module: just show the number
            displayStrings[i] = String(i + 1);
        } else if (moduleCount >= 4) {
            // 4+ modules: show "DIS1", "DIS2", etc
            displayStrings[i] = "DIS" + String(i + 1);
        } else {
            // 2-3 modules: show "D1", "D2", etc
            displayStrings[i] = "D" + String(i + 1);
        }
    }
    writeDisplays(displayStrings, speed, true);
    delete[] displayStrings;
    delay(1000);
    
    // Phase 3: Clear all modules to blank
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(' ');
    }
    moveTo(targetPositions, speed, true);
}

// Generic FreeRTOS task for I2C bus (handles both Wire and Wire1)
void SplitFlapDisplay::busTaskFunction(void* parameter, uint8_t busNum) {
    SplitFlapDisplay* display = static_cast<SplitFlapDisplay*>(parameter);
    
    // Select the correct mutex and movement struct based on bus number
    SemaphoreHandle_t &busMutex = (busNum == 0) ? display->bus0Mutex : display->bus1Mutex;
    BusMovement &busMovement = (busNum == 0) ? display->bus0Movement : display->bus1Movement;
    
    while (true) {
        bool shouldMove = false;
        
        // Check if there's work to do
        if (xSemaphoreTake(busMutex, 10 / portTICK_PERIOD_MS)) {
            shouldMove = busMovement.active;
            xSemaphoreGive(busMutex);
        }
        
        if (shouldMove) {
            // Get movement parameters
            int targetPositions[MAX_MODULES];
            float speed;
            bool releaseMotors;
            
            if (xSemaphoreTake(busMutex, portMAX_DELAY)) {
                memcpy(targetPositions, busMovement.targetPositions, sizeof(targetPositions));
                speed = busMovement.speed;
                releaseMotors = busMovement.releaseMotors;
                busMovement.active = false;
                xSemaphoreGive(busMutex);
            }
            
            // Execute movement on this bus
            display->moveToOnBus(busNum, targetPositions, speed, releaseMotors);
            
            // Mark complete
            if (xSemaphoreTake(busMutex, portMAX_DELAY)) {
                busMovement.complete = true;
                xSemaphoreGive(busMutex);
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Wrapper functions for FreeRTOS task creation
void SplitFlapDisplay::bus0TaskFunction(void* parameter) {
    busTaskFunction(parameter, 0);
}

void SplitFlapDisplay::bus1TaskFunction(void* parameter) {
    busTaskFunction(parameter, 1);
}

// Execute movement on a specific bus
void SplitFlapDisplay::moveToOnBus(uint8_t busNum, int targetPositions[], float speed, bool releaseMotors) {
    speed = constrain(speed, 2, maxVel);
    float stepsPerSecond = (speed / 60) * stepsPerRot;
    float timePerStep = 1000000 / stepsPerSecond;

    unsigned long moveStartTime = micros();
    unsigned long currentTime = moveStartTime;
    
    // Performance metrics (only tracked if perfLogging enabled)
    bool perfEnabled = settings.getInt("perfLogging") != 0;
    int perfStepCount = 0;
    int perfMuxSelects = 0;
    int perfSensorReads = 0;
    int perfTotalModulesOnBus = 0;
    
    // Accuracy settings (read once at start for efficiency)
    bool accEnabled = settings.getInt("accuracyLogging") != 0;
    int stepSettleUs = settings.getInt("stepSettleUs");
    int sensorDebounceCount = settings.getInt("sensorDebounceCount");
    int sensorDebugMs = settings.getInt("sensorDebugMs");
    int sensorCheckSteps = settings.getInt("sensorCheckSteps");
    int retryFailedSteps = settings.getInt("retryFailedSteps");
    bool missedMagnetRecovery = settings.getInt("missedMagnetRecovery") != 0;
    bool errorStatsTracking = settings.getInt("errorStatsTracking") != 0;

    static unsigned long sensorDebugStartMs = 0;
    if (sensorDebugStartMs == 0 && sensorDebugMs > 0) {
        sensorDebugStartMs = millis();
    }
    
    // Accuracy metrics (only tracked if accuracyLogging enabled)
    int accRetryCount = 0;
    int accMagnetCorrections = 0;

    int checkIntervalUs = 20 * 1000;
    bool useStepBasedSensorChecks = sensorCheckSteps > 0;
    int startStopDelay = 200;

    // Build list of modules on this bus that need to move
    int activeModules[numModules];
    int numActive = 0;
    
    bool resetLatches[numModules] = {};
    unsigned long lastStepTimes[numModules] = {};
    unsigned long lastSensorCheckTime = currentTime;
    int stepsSinceSensorCheck[numModules] = {};
    
    // Sensor debounce tracking (per-module consecutive read counts)
    int sensorDebounceHigh[numModules] = {};  // Consecutive HIGH reads
    int sensorDebounceLow[numModules] = {};   // Consecutive LOW reads
    bool lastSensorState[numModules] = {};
    bool lastSensorStateInit[numModules] = {};

    for (int i = 0; i < numModules; i++) {
        // Only process modules on the current bus
        if (muxBus[moduleMuxes[i]] != busNum) continue;
        
        if (perfEnabled) perfTotalModulesOnBus++;
        
        targetPositions[i] = constrain(targetPositions[i], 0, stepsPerRot - 1);
        lastStepTimes[i] = currentTime;
        
        if (modules[i].getPosition() != targetPositions[i]) {
            activeModules[numActive++] = i;
            resetLatches[i] = true;
            stepsSinceSensorCheck[i] = 0;
            
            // Priority 1: Calculate expected magnet crossings for this movement
            if (missedMagnetRecovery) {
                int distance = targetPositions[i] - modules[i].getPosition();
                if (distance < 0) distance += stepsPerRot;  // Handle wraparound
                int expectedCrossings = distance / stepsPerRot;
                modules[i].resetMagnetCrossings(expectedCrossings);
            }
        }
    }
    
    int perfActiveModules = numActive;  // Save initial count before loop decrements it
    
    if (numActive == 0) return;
    
    // Start motors
    for (int j = 0; j < numActive; j++) {
        int i = activeModules[j];
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        if (perfEnabled) perfMuxSelects++;
        modules[i].start();
    }
    delay(startStopDelay);

    auto processSensorCheck = [&](int i, bool ensureMuxSelected) {
        if (ensureMuxSelected) {
            selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
            if (perfEnabled) perfMuxSelects++;
        }
        if (perfEnabled) perfSensorReads++;

        bool sensorHigh = modules[i].readHallEffectSensor();

        bool sensorDebugActive = (sensorDebugMs > 0) &&
            (millis() - sensorDebugStartMs < (unsigned long)sensorDebugMs);
        if (sensorDebugActive) {
            if (!lastSensorStateInit[i]) {
                lastSensorState[i] = sensorHigh;
                lastSensorStateInit[i] = true;
            } else if (sensorHigh != lastSensorState[i]) {
                Serial.printf("[SENSOR Bus%d] Mod%d: %s at pos=%d\n",
                    busNum, i, sensorHigh ? "HIGH" : "LOW", modules[i].getPosition());
                lastSensorState[i] = sensorHigh;
            }
        }

        // Debounce logic: require consecutive consistent reads
        if (sensorDebounceCount > 1) {
            if (sensorHigh) {
                sensorDebounceHigh[i]++;
                sensorDebounceLow[i] = 0;
            } else {
                sensorDebounceLow[i]++;
                sensorDebounceHigh[i] = 0;
            }

            // Only trigger on debounced transitions
            bool debouncedHigh = (sensorDebounceHigh[i] >= sensorDebounceCount);
            bool debouncedLow = (sensorDebounceLow[i] >= sensorDebounceCount);

            if (debouncedHigh && !resetLatches[i]) {
                // Magnet detected (debounced rising edge)
                int oldPos = modules[i].getPosition();
                modules[i].magnetDetected();

                // Priority 1: Increment actual magnet crossings
                if (missedMagnetRecovery) {
                    modules[i].incrementMagnetCrossings();
                }

                // Priority 2: Record position error statistics
                if (errorStatsTracking && oldPos != modules[i].getPosition()) {
                    int error = oldPos - modules[i].getPosition();
                    modules[i].recordPositionError(error);
                }

                if (accEnabled && oldPos != modules[i].getPosition()) {
                    accMagnetCorrections++;
                    ACC_PRINTF("[ACC Bus%d] Mod%d magnet correction: %d -> %d\n",
                        busNum, i, oldPos, modules[i].getPosition());
                }
                resetLatches[i] = true;
            } else if (debouncedLow && resetLatches[i]) {
                resetLatches[i] = false;
            }
        } else {
            // No debounce - original behavior
            if (sensorHigh) {
                if (!resetLatches[i]) {
                    int oldPos = modules[i].getPosition();
                    modules[i].magnetDetected();

                    // Priority 1: Increment actual magnet crossings
                    if (missedMagnetRecovery) {
                        modules[i].incrementMagnetCrossings();
                    }

                    // Priority 2: Record position error statistics
                    if (errorStatsTracking && oldPos != modules[i].getPosition()) {
                        int error = oldPos - modules[i].getPosition();
                        modules[i].recordPositionError(error);
                    }

                    if (accEnabled && oldPos != modules[i].getPosition()) {
                        accMagnetCorrections++;
                        ACC_PRINTF("[ACC Bus%d] Mod%d magnet correction: %d -> %d\n",
                            busNum, i, oldPos, modules[i].getPosition());
                    }
                    resetLatches[i] = true;
                }
            } else if (resetLatches[i]) {
                resetLatches[i] = false;
            }
        }
    };

    // Main stepping loop
    while (numActive > 0) {
        currentTime = micros();
        
        // Step motors
        for (int j = 0; j < numActive; j++) {
            int i = activeModules[j];
            if ((currentTime - lastStepTimes[i]) > timePerStep) {
                selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
                if (perfEnabled) perfMuxSelects++;
                
                // Use accuracy-aware step if any accuracy features enabled
                if (stepSettleUs > 0 || retryFailedSteps > 0) {
                    modules[i].step(stepSettleUs, retryFailedSteps);
                } else {
                    modules[i].step();
                }
                if (perfEnabled) perfStepCount++;
                lastStepTimes[i] = micros();

                if (useStepBasedSensorChecks) {
                    stepsSinceSensorCheck[i]++;
                    if (stepsSinceSensorCheck[i] >= sensorCheckSteps) {
                        processSensorCheck(i, false);
                        stepsSinceSensorCheck[i] = 0;
                    }
                }

                if (modules[i].getPosition() == targetPositions[i]) {
                    if (accEnabled) {
                        ACC_PRINTF("[ACC Bus%d] Mod%d arrived at pos=%d (target=%d)\n",
                            busNum, i, modules[i].getPosition(), targetPositions[i]);
                    }
                    activeModules[j] = activeModules[numActive - 1];
                    numActive--;
                    j--;
                }
            }
        }

        // Check sensors
        if (!useStepBasedSensorChecks && (currentTime - lastSensorCheckTime) > checkIntervalUs) {
            for (int j = 0; j < numActive; j++) {
                int i = activeModules[j];
                processSensorCheck(i, true);
            }
            lastSensorCheckTime = currentTime;
        }
    }
    
    if (releaseMotors) {
        delay(startStopDelay);
        for (int i = 0; i < numModules; i++) {
            if (muxBus[moduleMuxes[i]] == busNum && modules[i].getPosition() == targetPositions[i]) {
                selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
                if (perfEnabled) perfMuxSelects++;
                modules[i].stop();
            }
        }
    }
    
    // Priority 1: Auto-recover modules that missed magnet crossings
    if (missedMagnetRecovery) {
        for (int i = 0; i < numModules; i++) {
            if (muxBus[moduleMuxes[i]] == busNum) {
                if (modules[i].hasMissedMagnetCrossings()) {
                    // Log warning (always, independent of accuracyLogging)
                    Serial.printf("[RECOVERY Bus%d] Mod%d missed magnet: expected %d, got %d crossings - auto-homing\n",
                        busNum, i, modules[i].getExpectedCrossings(), modules[i].getActualCrossings());
                    
                    // Home this specific module by moving back one step and detecting magnet
                    selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
                    int homeTarget = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
                    
                    // Start motor and move until magnet detected
                    modules[i].start();
                    bool magnetFound = false;
                    int maxHomeSteps = stepsPerRot + 100;  // Safety limit
                    int homeSteps = 0;
                    
                    while (!magnetFound && homeSteps < maxHomeSteps) {
                        modules[i].step(stepSettleUs, retryFailedSteps, true);
                        homeSteps++;
                        
                        if (homeSteps % 20 == 0) {  // Check sensor every 20 steps
                            bool sensorHigh = modules[i].readHallEffectSensor();
                            if (sensorHigh) {
                                modules[i].magnetDetected();
                                magnetFound = true;
                                Serial.printf("[RECOVERY Bus%d] Mod%d homed successfully at step %d\n", busNum, i, homeSteps);
                            }
                        }
                    }
                    
                    modules[i].stop();
                    
                    if (!magnetFound) {
                        Serial.printf("[ERROR Bus%d] Mod%d failed to home after %d steps\n", busNum, i, homeSteps);
                    }
                    
                    // Now move to the original target position
                    selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
                    modules[i].start();
                    int currentPos = modules[i].getPosition();
                    int stepsNeeded = (targetPositions[i] - currentPos + stepsPerRot) % stepsPerRot;
                    
                    for (int s = 0; s < stepsNeeded; s++) {
                        modules[i].step(stepSettleUs, retryFailedSteps, true);
                    }
                    
                    modules[i].stop();
                    Serial.printf("[RECOVERY Bus%d] Mod%d repositioned to target %d\n", busNum, i, targetPositions[i]);
                }
            }
        }
    }
    
    // Priority 2: Log error statistics summary (only if enabled and accuracyLogging on)
    if (errorStatsTracking && accEnabled) {
        for (int i = 0; i < numModules; i++) {
            if (muxBus[moduleMuxes[i]] == busNum) {
                const auto& stats = modules[i].getAccuracyStats();
                if (stats.totalCorrections > 0) {
                    ACC_PRINTF("[STATS Bus%d] Mod%d: corrections=%d, maxError=%d, avgError=%.1f\n",
                        busNum, i, stats.totalCorrections, stats.maxError, stats.avgError);
                }
            }
        }
    }
    
    // Performance logging (skip all calculations if disabled)
    if (perfEnabled) {
        unsigned long moveEndTime = micros();
        unsigned long wallTimeUs = moveEndTime - moveStartTime;
        int totalI2cOps = perfStepCount + perfMuxSelects + perfSensorReads;
        int i2cTransactionTimeUs = settings.getInt("i2cTransactionTime");
        unsigned long estimatedI2cTimeUs = totalI2cOps * i2cTransactionTimeUs;
        float utilizationPct = (wallTimeUs > 0) ? (estimatedI2cTimeUs * 100.0f / wallTimeUs) : 0.0f;
        
        // Calculate average steps per module
        int avgStepsPerModule = (perfActiveModules > 0) ? (perfStepCount / perfActiveModules) : 0;
        
        // Calculate achieved speed per module
        float avgStepsPerSecPerModule = (wallTimeUs > 0 && perfActiveModules > 0) ? 
            (perfStepCount * 1000000.0f / (wallTimeUs * perfActiveModules)) : 0.0f;
        float avgRPMPerModule = (avgStepsPerSecPerModule / stepsPerRot) * 60.0f;
        
        // Calculate percentage of max configured speed achieved
        float maxVel = settings.getFloat("maxVel");
        float speedPct = (maxVel > 0) ? (avgRPMPerModule / maxVel * 100.0f) : 0.0f;
        
        // Calculate I2C throughput
        int opsPerSec = (wallTimeUs > 0) ? (totalI2cOps * 1000000 / wallTimeUs) : 0;
        
        Serial.printf("[PERF Bus%d] mods=%d/%d steps=%d(~%d/mod) i2c=%d dur=%lums rpm=%.1f(%.0f%%) ops=%d/s util=%.1f%%\n",
            busNum, perfActiveModules, perfTotalModulesOnBus, perfStepCount, avgStepsPerModule, totalI2cOps, 
            wallTimeUs / 1000, avgRPMPerModule, speedPct, opsPerSec, utilizationPct);
    }
    
    // Accuracy logging summary (skip if disabled)
    if (accEnabled && accMagnetCorrections > 0) {
        ACC_PRINTF("[ACC Bus%d] Summary: %d magnet corrections applied\n", busNum, accMagnetCorrections);
    }
}
