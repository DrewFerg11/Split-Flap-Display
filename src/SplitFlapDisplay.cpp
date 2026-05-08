#include "SplitFlapDisplay.h"

#include "JsonSettings.h"
#include "SplitFlapModule.h"
#include "SplitFlapMqtt.h"
#include <freertos/semphr.h>

SplitFlapDisplay::SplitFlapDisplay(JsonSettings &settings) : settings(settings) {}

void SplitFlapDisplay::init() {
    stepsPerRot = settings.getInt("stepsPerRot");
    displayOffset = settings.getInt("displayOffset");
    magnetPosition = settings.getInt("magnetPosition");
    maxVel = settings.getFloat("maxVel");
    charSetSize = settings.getInt("charset");

    // Wire (Bus 1)
    std::vector<int> wAddrs   = settings.getIntVector("wire_moduleAddresses");
    std::vector<int> wOffsets = settings.getIntVector("wire_moduleOffsets");
    wireCount = wAddrs.size();
    for (int i = 0; i < wireCount; i++) {
        wireAddresses[i] = (uint8_t) wAddrs[i];
        wireOffsets[i]   = wOffsets[i];
    }

#ifdef ENABLE_DUAL_I2C
    // Wire1 (Bus 2)
    std::vector<int> w1Addrs   = settings.getIntVector("wire1_moduleAddresses");
    std::vector<int> w1Offsets = settings.getIntVector("wire1_moduleOffsets");
    wire1Count = w1Addrs.size();
    for (int i = 0; i < wire1Count; i++) {
        wire1Addresses[i] = (uint8_t) w1Addrs[i];
        wire1Offsets[i]   = w1Offsets[i];
    }
#else
    wire1Count = 0;
#endif

    numModules = wireCount + wire1Count;

    // Init Wire (Bus 1)
    SDAPin = settings.getInt("sdaPin");
    SCLPin = settings.getInt("sclPin");
    Wire.begin(SDAPin, SCLPin);
    Wire.setClock(400000);

#ifdef ENABLE_DUAL_I2C
    // Init Wire1 (Bus 2)
    SDA2Pin = settings.getInt("sda2Pin");
    SCL2Pin = settings.getInt("scl2Pin");
    bool wire1Ok = Wire1.begin(SDA2Pin, SCL2Pin);
    Wire1.setClock(400000);
    if (!wire1Ok) {
        Serial.printf("[ERROR] Wire1.begin() failed (SDA=%d, SCL=%d)\n", SDA2Pin, SCL2Pin);
    }
#endif

    // Wire modules fill indices 0..(wireCount-1)
    for (int i = 0; i < wireCount; i++) {
        modules[i] = SplitFlapModule(
            wireAddresses[i], stepsPerRot, wireOffsets[i] + displayOffset, magnetPosition, charSetSize, &Wire
        );
    }

#ifdef ENABLE_DUAL_I2C
    // Wire1 modules fill indices wireCount..(numModules-1)
    for (int i = 0; i < wire1Count; i++) {
        modules[wireCount + i] = SplitFlapModule(
            wire1Addresses[i], stepsPerRot, wire1Offsets[i] + displayOffset, magnetPosition, charSetSize, &Wire1
        );
    }
#endif

    configI2cModules();
    scanI2cModules();

    for (int i = 0; i < numModules; i++) {
        modules[i].init();
    }
}

void SplitFlapDisplay::configI2cModules() {
    Serial.println("\n=== I2C Configuration ===");

    Serial.printf("%-6s (SDA=%d, SCL=%d): ", "Wire", SDAPin, SCLPin);
    for (int i = 0; i < wireCount; i++) {
        Serial.printf("[%d]0x%02X ", i, wireAddresses[i]);
    }
    if (wireCount == 0) Serial.print("(none)");
    Serial.println();

#ifdef ENABLE_DUAL_I2C
    Serial.printf("%-6s (SDA=%d, SCL=%d): ", "Wire1", SDA2Pin, SCL2Pin);
    for (int i = 0; i < wire1Count; i++) {
        Serial.printf("[%d]0x%02X ", wireCount + i, wire1Addresses[i]);
    }
    if (wire1Count == 0) Serial.print("(none)");
    Serial.println();
#endif

    Serial.printf("Total modules: %d\n", numModules);
    Serial.println("=========================");
}

void SplitFlapDisplay::scanI2cModules() {
    Serial.println("\n=== I2C Scanner ===");

    struct BusCfg {
        TwoWire  *bus;
        const char *name;
        uint8_t  *addrs;
        int       count;
        int       idxOffset;
    };

    BusCfg buses[] = {
        {&Wire, "Wire", wireAddresses, wireCount, 0},
#ifdef ENABLE_DUAL_I2C
        {&Wire1, "Wire1", wire1Addresses, wire1Count, wireCount},
#endif
    };
    int numBuses = sizeof(buses) / sizeof(buses[0]);

    for (int b = 0; b < numBuses; b++) {
        TwoWire    *bus      = buses[b].bus;
        const char *busName  = buses[b].name;
        uint8_t    *cfgAddrs = buses[b].addrs;
        int         cfgCount = buses[b].count;
        int         idxOff   = buses[b].idxOffset;

        // Full address range scan so we can see unexpected devices and diagnose
        // buses that appear dead vs modules at wrong addresses
        bool cfgFound[MAX_MODULES] = {};
        bool anyFound = false;

        Serial.printf("%-6s:", busName);

        for (uint8_t addr = 1; addr < 127; addr++) {
            bus->beginTransmission(addr);
            uint8_t err = bus->endTransmission();
            if (err != 0) continue;

            anyFound = true;

            // Match against configured addresses
            int modIdx = -1;
            for (int i = 0; i < cfgCount; i++) {
                if (cfgAddrs[i] == addr) {
                    modIdx = idxOff + i;
                    cfgFound[i] = true;
                    break;
                }
            }

            if (modIdx >= 0) {
                Serial.printf(" [%d]0x%02X", modIdx, addr);
            } else {
                Serial.printf(" 0x%02X(unexpected)", addr);
            }
        }

        if (!anyFound) Serial.print(" (none found)");
        Serial.println();

        // Report any configured modules that did not respond
        for (int i = 0; i < cfgCount; i++) {
            if (!cfgFound[i]) {
                Serial.printf("  -> [%d]0x%02X MISSING\n", idxOff + i, cfgAddrs[i]);
            }
        }
    }

    Serial.println("===================\n");
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

#ifdef ENABLE_DUAL_I2C
    if (wire1Count > 0) {
        homeDual(speed);
        return;
    }
#endif

    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors(modules, numModules);
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
    startMotors(modules, numModules);
    moveTo(targetPositions, speed, false);
    writeString(homeString, speed, centering);
}

void SplitFlapDisplay::homeToChar(char homeChar, float speed) {
    Serial.println("Homing");
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors(modules, numModules);
    moveTo(targetPositions, speed, false);

    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, speed);
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
#ifdef ENABLE_DUAL_I2C
    if (wire1Count > 0) {
        moveToDual(targetPositions, speed, releaseMotors);
        return;
    }
#endif
    moveModules(modules, numModules, targetPositions, speed, releaseMotors, stepsPerRot, maxVel);
}

void SplitFlapDisplay::moveModules(SplitFlapModule *mods, int count, int *targets, float speed, 
                                   bool releaseMotors, int stepsPerRot, float maxVel) {
    if (count == 0) return;

    speed = constrain(speed, 2.0f, maxVel);
    float stepsPerSecond = (speed / 60.0f) * stepsPerRot;
    float timePerStep = 1000000.0f / stepsPerSecond;

    unsigned long currentTime = micros();

    int checkIntervalUs = 20 * 1000; // How often to check each modules hall effect sensor, less
    // than 20ms causes issues with bouncing
    int startStopDelay = 200; // time to wait to let motor realign itself to
    // magnetic field on stop and start

    bool resetLatches[count] = {}; // Initialize to false //start with latch on to prevent case where the
    // motion starts with the magnet over the sensor
    bool needsStepping[count] = {};             // Initialize to false; //modules that still require moving
    unsigned long lastStepTimes[count] = {};    // Initialize to false; //track when each module was last stepped
    unsigned long lastSensorCheckTime = currentTime; // track when we last read all the hall effect sensors

    for (int i = 0; i < count; i++) {
        targets[i] = constrain(
            targets[i],
            0,
            stepsPerRot - 1
        ); // Constrain to avoid errors with incorrect inputs
        resetLatches[i] = true;
        lastStepTimes[i] = currentTime;
        needsStepping[i] = (mods[i].getPosition() != targets[i]);
    }

    // Re-energize coils and give the motor time to align to magnetic field
    startMotors(mods, count);
    delay(startStopDelay);

    bool isFinished = checkAllFalse(needsStepping, count);

    while (! isFinished) {
        currentTime = micros();
        for (int i = 0; i < count; i++) {
            if (needsStepping[i] && ((currentTime - lastStepTimes[i]) > timePerStep)) {
                mods[i].step();
                lastStepTimes[i] = micros();
                if (mods[i].getPosition() == targets[i]) { // this module is not in the correct position,
                    // requires stepping
                    needsStepping[i] = false;
                }
            }
        }

        if ((currentTime - lastSensorCheckTime) > checkIntervalUs) { // check hall effect sensor every checkIntervalMs
            // check every modules sensor
            for (int i = 0; i < count; i++) {
                if (needsStepping[i] && mods[i].readHallEffectSensor()) {
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
                        mods[i].magnetDetected(); // update position to the modules
                        // magnet position
                        resetLatches[i] = true;
                    }
                } else if (resetLatches[i] == true) {
                    resetLatches[i] = false;
                }
            }
            isFinished = checkAllFalse(needsStepping, count);
            lastSensorCheckTime = currentTime; // recall micros because for loop may
            // take a moment to execute
        }
    }
    if (releaseMotors) {
        delay(startStopDelay);
        stopMotors(mods, count);
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

void SplitFlapDisplay::startMotors(SplitFlapModule *mods, int count) {
    for (int i = 0; i < count; i++) {
        mods[i].start();
    }
}

void SplitFlapDisplay::stopMotors(SplitFlapModule *mods, int count) {
    for (int i = 0; i < count; i++) {
        mods[i].stop();
    }
}

void SplitFlapDisplay::setMqtt(SplitFlapMqtt *mqttHandler) {
    mqtt = mqttHandler;
}

// =============================================================================
// Dual I2C bus support — all code below only compiled when ENABLE_DUAL_I2C is
// defined (WROOM build). The two buses drive physically separate rows of
// modules and run their movement loops on independent FreeRTOS tasks.
// =============================================================================
#ifdef ENABLE_DUAL_I2C

struct BusMoveParams {
    SplitFlapModule *modules;
    int count;
    int *targets;
    float speed;
    bool releaseMotors;
    int stepsPerRot;
    float maxVel;
    SemaphoreHandle_t doneSem;
};

static void busMovementTask(void *param) {
    BusMoveParams *p = static_cast<BusMoveParams *>(param);
    SplitFlapDisplay::moveModules(
        p->modules, p->count, p->targets,
        p->speed, p->releaseMotors, p->stepsPerRot, p->maxVel
    );
    xSemaphoreGive(p->doneSem);
    vTaskDelete(NULL);
}

void SplitFlapDisplay::moveToDual(int *targetPositions, float speed, bool releaseMotors) {
    int tasksToWait = 0;
    SemaphoreHandle_t doneSem = xSemaphoreCreateCounting(2, 0);

    BusMoveParams wireParams = {
        modules, wireCount, targetPositions,
        speed, releaseMotors, stepsPerRot, maxVel, doneSem
    };
    BusMoveParams wire1Params = {
        modules + wireCount, wire1Count, targetPositions + wireCount,
        speed, releaseMotors, stepsPerRot, maxVel, doneSem
    };

    if (wireCount > 0) {
        xTaskCreatePinnedToCore(busMovementTask, "bus1Move", 4096, &wireParams, 1, NULL, 1);
        tasksToWait++;
    }
    if (wire1Count > 0) {
        xTaskCreatePinnedToCore(busMovementTask, "bus2Move", 4096, &wire1Params, 1, NULL, 0);
        tasksToWait++;
    }

    for (int i = 0; i < tasksToWait; i++) {
        xSemaphoreTake(doneSem, portMAX_DELAY);
    }

    vSemaphoreDelete(doneSem);
}

void SplitFlapDisplay::homeDual(float speed) {
    Serial.println("Homing (dual bus)");
    int targetPositions[numModules];

    // Phase 1: spin nearly full rotation to find magnets on both buses in parallel
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    moveToDual(targetPositions, speed, false);

    // Phase 2: move all modules to space character
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(' ');
    }
    moveToDual(targetPositions, speed, true);
}

void SplitFlapDisplay::writeStringDual(String row1, String row2, float speed, bool centering) {
    int targetPositions[numModules];

    // Process row 1 (Wire bus — top row)
    String displayRow1 = row1.substring(0, wireCount);
    if (centering) {
        int padding = wireCount - displayRow1.length();
        int padLeft = padding / 2;
        String padded = "";
        for (int i = 0; i < padLeft; i++) padded += " ";
        padded += displayRow1;
        while ((int) padded.length() < wireCount) padded += " ";
        displayRow1 = padded;
    } else {
        while ((int) displayRow1.length() < wireCount) displayRow1 += " ";
    }
    for (int i = 0; i < wireCount; i++) {
        targetPositions[i] = modules[i].getCharPosition(displayRow1[i]);
    }

    // Process row 2 (Wire1 bus — bottom row)
    String displayRow2 = row2.substring(0, wire1Count);
    if (centering) {
        int padding = wire1Count - displayRow2.length();
        int padLeft = padding / 2;
        String padded = "";
        for (int i = 0; i < padLeft; i++) padded += " ";
        padded += displayRow2;
        while ((int) padded.length() < wire1Count) padded += " ";
        displayRow2 = padded;
    } else {
        while ((int) displayRow2.length() < wire1Count) displayRow2 += " ";
    }
    for (int i = 0; i < wire1Count; i++) {
        targetPositions[wireCount + i] = modules[wireCount + i].getCharPosition(displayRow2[i]);
    }

    moveToDual(targetPositions, speed, true);

    if (mqtt && mqtt->isConnected()) {
        mqtt->publishState(displayRow1 + "|" + displayRow2);
    }
}

#endif  // ENABLE_DUAL_I2C
