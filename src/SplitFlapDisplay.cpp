#include "SplitFlapDisplay.h"

#include "JsonSettings.h"
#include "SplitFlapModule.h"
#include "SplitFlapMqtt.h"

SplitFlapDisplay::SplitFlapDisplay(JsonSettings &settings) : settings(settings) {}

void SplitFlapDisplay::init() {
    stepsPerRot = settings.getInt("stepsPerRot");
    displayOffset = settings.getInt("displayOffset");
    magnetPosition = settings.getInt("magnetPosition");
    maxVel = settings.getFloat("maxVel");
    charSetSize = settings.getInt("charset");

    // Load per-channel module counts and derive total
    std::vector<int> settingCountPerChannel = settings.getIntVector("moduleCountPerChannel");
    numModules = 0;
    memset(moduleCountPerChannel, 0, sizeof(moduleCountPerChannel));
    for (int ch = 0; ch < 8; ch++) {
        if (ch < (int)settingCountPerChannel.size()) {
            moduleCountPerChannel[ch] = settingCountPerChannel[ch];
            numModules += moduleCountPerChannel[ch];
        }
    }

    std::vector<int> settingAddresses = settings.getIntVector("moduleAddresses");
    for (int i = 0; i < numModules; i++) {
        moduleAddresses[i] = (uint8_t) settingAddresses[i];
    }

    std::vector<int> settingOffsets = settings.getIntVector("moduleOffsets");
    for (int i = 0; i < numModules; i++) {
        moduleOffsets[i] = settingOffsets[i];
    }

    // Load module-to-mux-channel mapping; default to channel 0 if not present
    std::vector<int> settingChannels = settings.getIntVector("moduleChannels");
    for (int i = 0; i < numModules; i++) {
        uint8_t ch = 0;
        if (i < (int)settingChannels.size()) {
            ch = (uint8_t) settingChannels[i];
        }
        moduleChannels[i] = ch;
    }

    // Validation: verify array lengths match moduleCount
    if ((int)settingAddresses.size() != numModules) {
        Serial.printf("WARNING: moduleAddresses length (%d) != moduleCount (%d)\n", (int)settingAddresses.size(), numModules);
    }
    if ((int)settingOffsets.size() != numModules) {
        Serial.printf("WARNING: moduleOffsets length (%d) != moduleCount (%d)\n", (int)settingOffsets.size(), numModules);
    }
    if ((int)settingChannels.size() != numModules) {
        Serial.printf("WARNING: moduleChannels length (%d) != moduleCount (%d)\n", (int)settingChannels.size(), numModules);
    }

    // Print per-channel summary
    Serial.println("\n=== Per-Channel Module Configurations ===");
    int moduleIdx = 0;
    for (int ch = 0; ch < 8; ch++) {
        if (moduleCountPerChannel[ch] > 0) {
            Serial.printf("Ch%d: %d module(s) @ ", ch, moduleCountPerChannel[ch]);
            for (int j = 0; j < moduleCountPerChannel[ch]; j++) {
                Serial.printf("0x%02X", moduleAddresses[moduleIdx]);
                if (j < moduleCountPerChannel[ch] - 1) Serial.print(", ");
                moduleIdx++;
            }
            Serial.println();
        }
    }
    Serial.println("================================\n");

    for (uint8_t i = 0; i < numModules; i++) {
        modules[i] = SplitFlapModule(
            moduleAddresses[i], stepsPerRot, moduleOffsets[i] + displayOffset, magnetPosition, charSetSize
        );
    }

    SDAPin = settings.getInt("sdaPin");
    SCLPin = settings.getInt("sclPin");

    Wire.begin(SDAPin, SCLPin);
    Wire.setClock(400000);
    
    // Scan TCA9548A multiplexer channels at startup
    Serial.println("\n=== TCA9548A I2C Multiplexer Scanner ===");
    Wire.beginTransmission(muxAddress);
    if (Wire.endTransmission() == 0) {
        Serial.printf("TCA9548A found at address 0x%02X\n", muxAddress);
        scanMuxChannels();
    } else {
        Serial.printf("WARNING: TCA9548A not detected at 0x%02X\n", muxAddress);
        Serial.println("Scanning main I2C bus...");
        selectMuxChannel(0);  // Default to channel 0
    }
    Serial.println("=== End I2C Scanner ===\n");

    for (uint8_t i = 0; i < numModules; i++) {
        selectMuxChannel(moduleChannels[i]);
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

void SplitFlapDisplay::moveTo(int targetPositions[], float speed, bool releaseMotors) {
    // TODO check length of array and return if empty

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
                selectMuxChannel(moduleChannels[i]);
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
                    selectMuxChannel(moduleChannels[i]);  // Select channel before reading sensor
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
        selectMuxChannel(moduleChannels[i]);
        modules[i].start();
    }
}

void SplitFlapDisplay::stopMotors() {
    // Serial.println("Stopping Motors");
    for (int i = 0; i < numModules; i++) {
        selectMuxChannel(moduleChannels[i]);
        modules[i].stop();
    }
}

void SplitFlapDisplay::setMqtt(SplitFlapMqtt *mqttHandler) {
    mqtt = mqttHandler;
}

// TCA9548A Multiplexer Channel Selection
void SplitFlapDisplay::selectMuxChannel(uint8_t channel) {
    if (channel > 7) return;  // TCA9548A has 8 channels (0-7)
    
    static uint8_t lastChannel = 255;  // Track last selected channel
    if (channel != lastChannel) {
        // Serial.printf("[DEBUG] MUX: Switching to channel %d\n", channel);
        lastChannel = channel;
    }
    
    Wire.beginTransmission(muxAddress);
    Wire.write(1 << channel);  // Set bit for desired channel
    byte error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("[ERROR] MUX channel select failed: error %d\n", error);
    }
}

// Scan all TCA9548A channels for I2C devices
void SplitFlapDisplay::scanMuxChannels() {
    for (uint8_t channel = 0; channel < 8; channel++) {
        selectMuxChannel(channel);
        
        Serial.printf("\nScanning channel %d: ", channel);
        bool foundDevice = false;
        
        // Full address sweep from 0x08 to 0x77 (excluding reserved addresses)
        for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
            // Skip the mux address itself (it's on the main bus, not downstream)
            if (addr == muxAddress) continue;
            
            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                if (!foundDevice) {
                    Serial.print("[");
                    foundDevice = true;
                } else {
                    Serial.print(", ");
                }
                Serial.printf("0x%02X", addr);
            }
        }
        
        if (foundDevice) {
            Serial.print("]");
        } else {
            Serial.print("[]");
        }
    }
    Serial.println();
}

// Home all active channels serially
void SplitFlapDisplay::homeAllChannels(float speed) {
    Serial.println("\n=== Homing All Channels ===");
    
    // Process each channel that has modules
    for (uint8_t ch = 0; ch < 8; ch++) {
        if (moduleCountPerChannel[ch] == 0) continue;  // Skip empty channels
        
        Serial.printf("Homing channel %d (%d modules)...\n", ch, moduleCountPerChannel[ch]);
        
        // Find the starting module index for this channel
        int startIdx = 0;
        for (int prevCh = 0; prevCh < ch; prevCh++) {
            startIdx += moduleCountPerChannel[prevCh];
        }
        
        int numChannelModules = moduleCountPerChannel[ch];
        
        // Build home string for this channel (first module "O", second "K", rest blank)
        String homeString = "";
        if (numChannelModules >= 2) {
            homeString = "OK";
            for (int i = 2; i < numChannelModules; i++) {
                homeString += " ";
            }
        } else if (numChannelModules == 1) {
            homeString = "O";  // Single module shows "O"
        }
        
        // Phase 1: Step back one to trigger homing for this channel's modules
        int targetPositions[numModules];
        for (int i = 0; i < numModules; i++) {
            targetPositions[i] = modules[i].getPosition();  // No movement
        }
        for (int i = 0; i < numChannelModules; i++) {
            int moduleIdx = startIdx + i;
            targetPositions[moduleIdx] = (modules[moduleIdx].getPosition() - 1 + stepsPerRot) % stepsPerRot;
        }
        moveTo(targetPositions, speed, false);
        
        // Phase 2: Move this channel to "OK" (or "O" or blank)
        for (int i = 0; i < numModules; i++) {
            targetPositions[i] = modules[i].getPosition();  // No movement
        }
        for (int i = 0; i < numChannelModules; i++) {
            int moduleIdx = startIdx + i;
            char targetChar = (i < homeString.length()) ? homeString[i] : ' ';
            int charPos = modules[moduleIdx].getCharPosition(targetChar);
            targetPositions[moduleIdx] = charPos;
        }
        moveTo(targetPositions, speed, false);
        delay(500);
        
        // Phase 3: Clear this channel to blanks
        for (int i = 0; i < numModules; i++) {
            targetPositions[i] = modules[i].getPosition();  // No movement
        }
        for (int i = 0; i < numChannelModules; i++) {
            int moduleIdx = startIdx + i;
            targetPositions[moduleIdx] = modules[moduleIdx].getCharPosition(' ');
        }
        bool releaseLast = (ch == 7 || moduleCountPerChannel[ch + 1] == 0);  // Release on last channel
        moveTo(targetPositions, speed, releaseLast);
        
        Serial.printf("Channel %d homing complete\n", ch);
    }
    
    Serial.println("=== All Channels Homed ===\n");
}
