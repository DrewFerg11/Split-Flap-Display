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

    // Configure modules based on settings
    configureI2cModules();

    // Initialize I2C bus
    SDAPin = settings.getInt("sdaPin");
    SCLPin = settings.getInt("sclPin");
    Wire.begin(SDAPin, SCLPin);
    Wire.setClock(400000);
    
    // Scan modules to confirm connectivity
    scanI2cModules();

    // Create and initialize all module objects
    for (uint8_t i = 0; i < numModules; i++) {
        modules[i] = SplitFlapModule(
            moduleAddresses[i], stepsPerRot, moduleOffsets[i] + displayOffset, magnetPosition, charSetSize
        );
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        modules[i].init();
    }
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
    int targetPositions[numModules];
    // Iterate through the input string and process each character
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(inputChar);
    }
    moveTo(targetPositions, speed);
}

void SplitFlapDisplay::writeString(String inputString, float speed, bool centering) {
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
    speed = constrain(speed, 2, maxVel);
    float stepsPerSecond = (speed / 60) * stepsPerRot;
    float timePerStep = 1000000 / stepsPerSecond;

    unsigned long currentTime = micros();

    int checkIntervalUs = 20 * 1000; // How often to check each modules hall effect sensor, less
    // than 20ms causes issues with bouncing
    int startStopDelay = 200; // time to wait to let motor realign itself to
    // magnetic field on stop and start

    bool resetLatches[numModules] = {}; // Initialize to false //start with latch on to prevent case where the
    // motion starts with the magnet over the sensor
    bool needsStepping[numModules] = {};             // Initialize to false; //modules that still require moving
    unsigned long lastStepTimes[numModules] = {};    // Initialize to false; //track when each module was last stepped
    unsigned long lastSensorCheckTime = currentTime; // track when we last read all the hall effect sensors

    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = constrain(
            targetPositions[i],
            0,
            stepsPerRot - 1
        ); // Constrain to avoid errors with incorrect inputs
        resetLatches[i] = true;
        lastStepTimes[i] = currentTime;
        if (modules[i].getPosition() != targetPositions[i]) {
            needsStepping[i] = true;
        } else {
            needsStepping[i] = false;
        }
    }

    startMotors(); // not sure if this helps or not, likely that it does not based
    // on testing
    delay(startStopDelay); // give the motor time to align to magnetic field

    bool isFinished = checkAllFalse(needsStepping, numModules);
    while (! isFinished) {
        currentTime = micros();
        
        // Step motors - select channel only when it changes
        for (int i = 0; i < numModules; i++) {
            if (((currentTime - lastStepTimes[i]) > timePerStep) && needsStepping[i]) {
                selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
                modules[i].step();
                lastStepTimes[i] = micros();
                if (modules[i].getPosition() == targetPositions[i]) { // this module is not in the correct position,
                    // requires stepping
                    needsStepping[i] = false;
                }
            }
        }

        if ((currentTime - lastSensorCheckTime) > checkIntervalUs) { // check hall effect sensor every checkIntervalMs
            // check every modules sensor - select channel only when it changes
            for (int i = 0; i < numModules; i++) {
                if (needsStepping[i]) {
                    selectMuxChannel(moduleMuxes[i], moduleChannels[i]);  // Select channel before reading sensor
                    if (modules[i].readHallEffectSensor() == true) { // only check sensors where the module is still moving
                        if (! resetLatches[i]) {
                            // UNCOMMENTING THIS WILL PROBBALY MAKE THE MOTORS INACCURATE, DUE
                            // TO TIME TAKEN TO PRINT
                            //  Serial.print("Module: ");
                            //  Serial.print(i);
                            //  Serial.print(" Magnet Position: ");
                            //  Serial.print(modules[i].getMagnetPosition());
                            //  Serial.print(" Actual Position: ");
                            //  Serial.print(modules[i].getPosition());
                            //  Serial.print(" Error: ");
                            //  Serial.println((modules[i].getMagnetPosition() -
                            //  modules[i].getPosition()));
                            modules[i].magnetDetected(); // update position to the modules
                            // magnet position
                            resetLatches[i] = true;
                        }
                    } else if (resetLatches[i] == true) {
                        resetLatches[i] = false;
                    }
                }
            }
            isFinished = checkAllFalse(needsStepping, numModules);
            lastSensorCheckTime = currentTime; // recall micros because for loop may
            // take a moment to execute
        }
        
        // TODO: Decide if this is necessary based on testing
        // // Yield to prevent watchdog timeout during long motor movements
        // yield();
        // esp_task_wdt_reset();  // Reset watchdog timer
    }
    if (releaseMotors) {
        delay(startStopDelay); // allow all motors time to settle
        stopMotors();
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
    
    static uint8_t lastMuxIndex = 255;
    static uint8_t lastChannel = 255;
    
    // Only switch if needed (optimization to avoid excessive I2C traffic)
    if (muxIndex == lastMuxIndex && channel == lastChannel) {
        return;  // Already on the correct mux and channel
    }
    
    // First, disable ALL muxes (including target) to ensure clean state
    for (int i = 0; i < numMuxes; i++) {
        Wire.beginTransmission(muxAddrs[i]);
        Wire.write(0x00);  // Disable all channels
        Wire.endTransmission();
    }
    
    // Small delay to let muxes settle
    delayMicroseconds(10);
    
    // Now enable only the target channel on the target mux
    Wire.beginTransmission(muxAddr);
    Wire.write(1 << channel);  // Set bit for desired channel
    byte error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("[ERROR] MUX 0x%02X channel select failed: error %d\n", muxAddr, error);
    }
    
    // Small delay to let the mux channel activate
    delayMicroseconds(10);
    
    lastMuxIndex = muxIndex;
    lastChannel = channel;
}

void SplitFlapDisplay::configureI2cModules() {
    // Parse multiplexer addresses (e.g., "112,113" -> 0x70,0x71)
    String muxAddrsStr = settings.getString("muxAddrs");
    numMuxes = 0;
    memset(muxAddrs, 0, sizeof(muxAddrs));
    
    int start = 0;
    for (int i = 0; muxAddrsStr.length() > 0 && i <= muxAddrsStr.length() && numMuxes < 8; i++) {
        if (i == muxAddrsStr.length() || muxAddrsStr[i] == ',') {
            String addrStr = muxAddrsStr.substring(start, i);
            addrStr.trim();
            if (addrStr.length() > 0) {
                muxAddrs[numMuxes++] = (uint8_t)addrStr.toInt();
            }
            start = i + 1;
        }
    }
    
    // Default to single mux at 0x70 if not configured
    if (numMuxes == 0) {
        muxAddrs[0] = 0x70;
        numMuxes = 1;
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
        String addrsKey = "chModAddrs" + String(muxAddrs[muxIdx]);
        String moduleAddrStr = settings.getString(addrsKey.c_str());
        
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
                                moduleOffsets[flatIdx] = 0;
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
    Serial.printf("\n=== I2C Configurations ===\n");
    for (int muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        bool muxHasModules = false;
        for (int i = 0; i < numModules; i++) {
            if (moduleMuxes[i] == muxIdx) {
                muxHasModules = true;
                break;
            }
        }
        
        if (muxHasModules) {
            Serial.printf("Mux%d (0x%02X): ", muxIdx, muxAddrs[muxIdx]);
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
    Serial.println("==========================");
}

void SplitFlapDisplay::scanI2cModules() {
    Serial.println("\n=== I2C Scanner ===");
    
    // Scan all 8 possible TCA9548A addresses
    for (uint8_t addr = 0x70; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            // Check if this is a configured mux
            bool isConfigured = false;
            uint8_t configIdx = 0;
            for (uint8_t i = 0; i < numMuxes; i++) {
                if (muxAddrs[i] == addr) {
                    isConfigured = true;
                    configIdx = i;
                    break;
                }
            }
            
            // Print in same format as config output
            Serial.printf("Mux%d (0x%02X)", configIdx, addr);
            if (!isConfigured) {
                Serial.print(" (found)");
            }
            Serial.print(": ");
            
            // Scan all channels and print inline
            bool foundAny = false;
            for (uint8_t channel = 0; channel < 8; channel++) {
                // Disable ALL multiplexers first to prevent crosstalk
                for (uint8_t a = 0x70; a <= 0x77; a++) {
                    Wire.beginTransmission(a);
                    Wire.write(0x00);
                    Wire.endTransmission();
                }
                delay(10);
                
                // Enable only the target channel on the target mux
                Wire.beginTransmission(addr);
                Wire.write(1 << channel);
                Wire.endTransmission();
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
                    
                    Wire.beginTransmission(devAddr);
                    if (Wire.endTransmission() == 0) {
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
            
            // Disable all multiplexers after scan
            for (uint8_t a = 0x70; a <= 0x77; a++) {
                Wire.beginTransmission(a);
                Wire.write(0x00);
                Wire.endTransmission();
            }
            
            Serial.println();
        }
    }
    
    Serial.println("===================\n");
}

// Home all active displays in parallel
void SplitFlapDisplay::homeAllChannels(float speed) {
    // Phase 1: Trigger homing for all modules
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false);
    delay(2000);
    
    // Phase 2: Display labels on each configured display
    String* displayStrings = new String[numDisplays];
    for (int i = 0; i < numDisplays; i++) {
        displayStrings[i] = "D" + String(i + 1);
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
