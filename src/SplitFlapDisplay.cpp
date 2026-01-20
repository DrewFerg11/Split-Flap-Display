#include "SplitFlapDisplay.h"

#include "JsonSettings.h"
#include "SplitFlapModule.h"
#include "SplitFlapMqtt.h"
#include <esp_task_wdt.h>

SplitFlapDisplay::SplitFlapDisplay(JsonSettings &settings) : settings(settings) {
    // Initialize FreeRTOS mutexes for thread safety
    i2cMutex = xSemaphoreCreateMutex();
    moduleStateMutex = xSemaphoreCreateMutex();
    numDisplayTasks = 1;  // Default to single task to avoid I2C mutex contention
    
    // Initialize completion flags
    for (int i = 0; i < MAX_DISPLAY_TASKS; i++) {
        taskCompleteFlags[i] = true;
    }
}

void SplitFlapDisplay::init() {
    stepsPerRot = settings.getInt("stepsPerRot");
    displayOffset = settings.getInt("displayOffset");
    magnetPosition = settings.getInt("magnetPosition");
    maxVel = settings.getFloat("maxVel");
    charSetSize = settings.getInt("charset");

    // Read dual bus setting FIRST (needed by configureI2cModules)
    useDualBus = settings.getInt("useDualBus") != 0;

    // Configure modules based on settings (uses useDualBus flag)
    configureI2cModules();

    // Initialize primary I2C bus (Wire)
    SDAPin = settings.getInt("wire0SdaPin");
    SCLPin = settings.getInt("wire0SclPin");
    Wire.begin(SDAPin, SCLPin);
    Wire.setClock(400000);  // 400kHz - most reliable for all hardware
    
    // Initialize secondary I2C bus (Wire1) if configured
    if (useDualBus) {
        SDA1Pin = settings.getInt("wire1Sda1Pin");
        SCL1Pin = settings.getInt("wire1Scl1Pin");
        Wire1.begin(SDA1Pin, SCL1Pin);
        Wire1.setClock(400000);  // 400kHz - most reliable for all hardware
        Serial.printf("Dual I2C enabled: Wire1 on SDA=%d, SCL=%d\n", SDA1Pin, SCL1Pin);
    }
    
    // Scan modules to confirm connectivity
    scanI2cModules();

    // Create and initialize all module objects
    for (uint8_t i = 0; i < numModules; i++) {
        modules[i] = SplitFlapModule(
            moduleAddresses[i], stepsPerRot, moduleOffsets[i] + displayOffset, magnetPosition, charSetSize
        );
        TwoWire& wire = getWireForMux(moduleMuxes[i]);
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        modules[i].init(wire);
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
    moveTo(targetPositions, speed, false, true);
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
    moveTo(targetPositions, speed, false, true);
    writeString(homeString, speed, centering);
}

void SplitFlapDisplay::homeToChar(char homeChar, float speed) {
    Serial.println("Homing");
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false, true);

    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = modules[i].getCharPosition(homeChar);
    }
    moveTo(targetPositions, speed, true);
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
    
    Serial.printf("=== writeString ===\n");
    Serial.printf("Input: '%s' (len=%d)\n", inputString.c_str(), inputString.length());
    Serial.printf("Display: '%s' (len=%d)\n", displayString.c_str(), displayString.length());
    Serial.printf("NumModules: %d\n", numModules);
    
    // Iterate through the input string and process each character
    for (int i = 0; i < displayString.length(); i++) {
        char currentChar = displayString[i];
        targetPositions[i] = modules[i].getCharPosition(currentChar);
        Serial.printf("M%d: char='%c' (0x%02X) -> pos=%d\n", i, currentChar, (unsigned char)currentChar, targetPositions[i]);
    }
    Serial.println("===================");
    
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

void SplitFlapDisplay::moveTo(int targetPositions[], float speed, bool releaseMotors, bool enableHallSensors) {
    speed = constrain(speed, 2, maxVel);
    float stepsPerSecond = (speed / 60) * stepsPerRot;
    unsigned long stepIntervalUs = 1000000 / stepsPerSecond;
    
    int checkIntervalSteps = enableHallSensors ? 20 : 0; // Check sensors every N steps (reduced from 10 to 20)
    
    // Constrain all target positions first
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = constrain(targetPositions[i], 0, stepsPerRot - 1);
    }
    
    // Use dual-bus parallel execution if dual bus is enabled
    if (useDualBus && numMuxes > 1) {
#if SPLITFLAP_DEBUG
        Serial.printf("[DualBus] Routing to parallel execution (useDualBus=%d, numMuxes=%d)\n", useDualBus, numMuxes);
#endif
        moveToDualBus(targetPositions, stepIntervalUs, checkIntervalSteps, releaseMotors, enableHallSensors);
        return;
    }
    
    // Use multi-threaded execution if we have multiple displays
    // and threading is enabled (numDisplayTasks > 1)
    if (numDisplays > 1 && numDisplayTasks > 1) {
        moveToThreaded(targetPositions, stepIntervalUs, checkIntervalSteps * stepIntervalUs, releaseMotors, enableHallSensors);
        return;
    }
    
    // ==========================================================================
    // SYNCHRONIZED BURST STEPPING
    // Instead of checking each module's timing individually, we step ALL active
    // modules in a tight burst at a fixed cadence. This maximizes I2C throughput.
    // ==========================================================================
    
    int startStopDelay = 200;

    // Debug instrumentation
    unsigned long totalI2cTimeUs = 0;
    unsigned long totalSteps = 0;
    unsigned long burstCount = 0;
    unsigned long moveStartTime = micros();

    // Track which modules still need to move and their remaining steps
    int stepsRemaining[numModules];
    bool isActive[numModules];
    bool resetLatches[numModules] = {};
    int numActive = 0;
    
    for (int i = 0; i < numModules; i++) {
        int current = modules[i].getPosition();
        int target = targetPositions[i];
        
        if (current != target) {
            // Calculate steps needed (always move forward, wrapping around)
            stepsRemaining[i] = (target - current + stepsPerRot) % stepsPerRot;
            if (stepsRemaining[i] == 0) stepsRemaining[i] = stepsPerRot; // Full rotation if same position
            isActive[i] = true;
            resetLatches[i] = true;
            numActive++;
        } else {
            stepsRemaining[i] = 0;
            isActive[i] = false;
        }
    }
    
    if (numActive == 0) return;  // Nothing to do
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Burst] Starting: %d modules\n", numActive);
#endif

    // Start all active motors (grouped by mux/channel for efficiency)
    for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        for (uint8_t channel = 0; channel < 8; channel++) {
            bool selected = false;
            TwoWire* wire = nullptr;
            for (int i = 0; i < numModules; i++) {
                if (!isActive[i]) continue;
                if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                
                if (!selected) {
                    selectMuxChannel(muxIdx, channel);
                    wire = &getWireForMux(muxIdx);
                    selected = true;
                }
                modules[i].start(*wire);
            }
        }
    }
    delay(startStopDelay);

    // Main stepping loop - synchronized burst stepping
    unsigned long lastBurstTime = micros();
    int stepsSinceLastSensorCheck = 0;
    
    // Calculate minimum I2C time per burst (estimate)
    // If I2C takes longer than step interval, skip the wait
    unsigned long estimatedBurstTimeUs = numActive * 150;  // ~150μs per I2C write
    bool i2cLimited = (estimatedBurstTimeUs > stepIntervalUs);
    
    while (numActive > 0) {
        unsigned long now = micros();
        
        // Only wait if we're ahead of schedule (not I2C limited)
        if (!i2cLimited && (now - lastBurstTime) < stepIntervalUs) {
            continue;  // Busy-wait for timing precision
        }
        lastBurstTime = now;
        burstCount++;
        
        // BURST: Step ALL active modules as fast as possible
        // Group by mux/channel to minimize mux switching
        for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
            for (uint8_t channel = 0; channel < 8; channel++) {
                bool selected = false;
                TwoWire* wire = nullptr;
                
                for (int i = 0; i < numModules; i++) {
                    if (!isActive[i]) continue;
                    if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                    
                    if (!selected) {
                        selectMuxChannel(muxIdx, channel);
                        wire = &getWireForMux(muxIdx);
                        selected = true;
                    }
                    
                    unsigned long i2cStart = micros();
                    modules[i].step(true, *wire);
                    totalI2cTimeUs += micros() - i2cStart;
                    totalSteps++;
                    
                    stepsRemaining[i]--;
                    if (stepsRemaining[i] <= 0) {
                        isActive[i] = false;
                        numActive--;
                        // Recalculate if we're still I2C limited
                        i2cLimited = (numActive * 150 > stepIntervalUs);
                    }
                }
            }
        }
        
        // Check hall sensors periodically (every N steps)
        if (enableHallSensors && checkIntervalSteps > 0) {
            stepsSinceLastSensorCheck++;
            if (stepsSinceLastSensorCheck >= checkIntervalSteps) {
                stepsSinceLastSensorCheck = 0;
                
                for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
                    for (uint8_t channel = 0; channel < 8; channel++) {
                        bool selected = false;
                        TwoWire* wire = nullptr;
                        
                        for (int i = 0; i < numModules; i++) {
                            if (!isActive[i]) continue;
                            if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                            
                            if (!selected) {
                                selectMuxChannel(muxIdx, channel);
                                wire = &getWireForMux(muxIdx);
                                selected = true;
                            }
                            
                            unsigned long i2cStart = micros();
                            bool sensorTriggered = modules[i].readHallEffectSensor(*wire);
                            totalI2cTimeUs += micros() - i2cStart;
                            
                            if (sensorTriggered) {
                                if (!resetLatches[i]) {
                                    modules[i].magnetDetected();
                                    resetLatches[i] = true;
                                }
                            } else if (resetLatches[i]) {
                                resetLatches[i] = false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Stop all motors
    if (releaseMotors) {
        delay(startStopDelay);
        for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
            for (uint8_t channel = 0; channel < 8; channel++) {
                bool selected = false;
                TwoWire* wire = nullptr;
                for (int i = 0; i < numModules; i++) {
                    if (modules[i].getPosition() != targetPositions[i]) continue;
                    if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                    
                    if (!selected) {
                        selectMuxChannel(muxIdx, channel);
                        wire = &getWireForMux(muxIdx);
                        selected = true;
                    }
                    modules[i].stop(*wire);
                }
            }
        }
    }

    unsigned long totalTimeMs = (micros() - moveStartTime) / 1000;
    float stepsPerSec = totalSteps * 1000000.0f / (micros() - moveStartTime);
    float i2cUtilization = 100.0f * totalI2cTimeUs / (micros() - moveStartTime);
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Burst] DONE: %lu steps in %lums (%.0f steps/sec)\n", 
                  totalSteps, totalTimeMs, stepsPerSec);
    Serial.printf("[Burst] I2C: %lums (%.1f%% util), Bursts: %lu\n",
                  totalI2cTimeUs / 1000, i2cUtilization, burstCount);
#endif
}

// Dual-bus parallel version of moveTo - runs two FreeRTOS tasks in parallel (one per I2C bus)
void SplitFlapDisplay::moveToDualBus(int targetPositions[], unsigned long stepIntervalUs, int checkIntervalSteps, bool releaseMotors, bool enableHallSensors) {
    // Slow down parallel execution to prevent missed steps
    // Increase step interval by 50% to give I2C more time during parallel execution
    stepIntervalUs = (stepIntervalUs * 3) / 2;
    
#if SPLITFLAP_DEBUG
    Serial.println("[DualBus] Starting parallel execution on Wire0 and Wire1");
    Serial.printf("[DualBus] Step interval: %lu us (slowed for parallel)\n", stepIntervalUs);
    Serial.print("[DualBus] Target positions: ");
    for (int i = 0; i < numModules; i++) {
        Serial.printf("M%d=%d ", i, targetPositions[i]);
    }
    Serial.println();
#endif
    
    // Create separate target position arrays for each bus to prevent race conditions
    int* targetPositions0 = new int[numModules];
    int* targetPositions1 = new int[numModules];
    memcpy(targetPositions0, targetPositions, numModules * sizeof(int));
    memcpy(targetPositions1, targetPositions, numModules * sizeof(int));
    
    // Create task completion flags
    volatile bool task0Complete = false;
    volatile bool task1Complete = false;
    
    // Prepare task parameters for both buses
    DualBusTaskParams taskParams[2];
    
    // Task 0: Handle all modules on Wire0 (bus 0)
    taskParams[0].display = this;
    taskParams[0].targetPositions = targetPositions0;  // Use separate copy
    taskParams[0].busIndex = 0;
    taskParams[0].stepIntervalUs = stepIntervalUs;
    taskParams[0].checkIntervalSteps = checkIntervalSteps;
    taskParams[0].releaseMotors = releaseMotors;
    taskParams[0].enableHallSensors = enableHallSensors;
    taskParams[0].taskHandle = NULL;
    taskParams[0].taskComplete = &task0Complete;
    
    // Task 1: Handle all modules on Wire1 (bus 1)
    taskParams[1].display = this;
    taskParams[1].targetPositions = targetPositions1;  // Use separate copy
    taskParams[1].busIndex = 1;
    taskParams[1].stepIntervalUs = stepIntervalUs;
    taskParams[1].checkIntervalSteps = checkIntervalSteps;
    taskParams[1].releaseMotors = releaseMotors;
    taskParams[1].enableHallSensors = enableHallSensors;
    taskParams[1].taskHandle = NULL;
    taskParams[1].taskComplete = &task1Complete;
    
    // Create task for Wire0
    BaseType_t result0 = xTaskCreatePinnedToCore(
        dualBusTaskFunction,           // Task function
        "BusTask0",                    // Task name
        DISPLAY_TASK_STACK_SIZE,       // Stack size
        &taskParams[0],                // Parameters
        DISPLAY_TASK_PRIORITY,         // Priority
        &taskParams[0].taskHandle,     // Task handle
        0                              // Pin to Core 0
    );
    
    // Create task for Wire1
    BaseType_t result1 = xTaskCreatePinnedToCore(
        dualBusTaskFunction,           // Task function
        "BusTask1",                    // Task name
        DISPLAY_TASK_STACK_SIZE,       // Stack size
        &taskParams[1],                // Parameters
        DISPLAY_TASK_PRIORITY,         // Priority
        &taskParams[1].taskHandle,     // Task handle
        1                              // Pin to Core 1
    );
    
    if (result0 != pdPASS) {
        Serial.println("[ERROR] Failed to create Wire0 task");
        task0Complete = true;
    }
    if (result1 != pdPASS) {
        Serial.println("[ERROR] Failed to create Wire1 task");
        task1Complete = true;
    }
    
    // Wait for both tasks to complete
    while (!task0Complete || !task1Complete) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Check every 10ms
    }
    
    // Clean up allocated memory
    delete[] targetPositions0;
    delete[] targetPositions1;
    
#if SPLITFLAP_DEBUG
    Serial.println("[DualBus] Both buses complete");
#endif
}

// Multi-threaded version of moveTo - divides work among multiple FreeRTOS tasks
void SplitFlapDisplay::moveToThreaded(int targetPositions[], float timePerStep, int checkIntervalUs, bool releaseMotors, bool enableHallSensors) {
    // Determine how many tasks to use (don't use more tasks than displays)
    int tasksToUse = min(numDisplayTasks, numDisplays);
    tasksToUse = min(tasksToUse, MAX_DISPLAY_TASKS);
    
    if (tasksToUse <= 0) return;
    
    // Calculate displays per task
    int displaysPerTask = (numDisplays + tasksToUse - 1) / tasksToUse;  // Round up
    
    Serial.printf("[Threading] Using %d tasks for %d displays (%d displays/task)\n", 
                  tasksToUse, numDisplays, displaysPerTask);
    
    // Prepare task parameters
    DisplayTaskParams taskParams[MAX_DISPLAY_TASKS];
    
    for (int t = 0; t < tasksToUse; t++) {
        taskCompleteFlags[t] = false;
        
        taskParams[t].display = this;
        taskParams[t].targetPositions = targetPositions;
        taskParams[t].startDisplayIndex = t * displaysPerTask;
        taskParams[t].endDisplayIndex = min((t + 1) * displaysPerTask, numDisplays);
        taskParams[t].timePerStep = timePerStep;
        taskParams[t].checkIntervalUs = checkIntervalUs;
        taskParams[t].taskComplete = &taskCompleteFlags[t];
        taskParams[t].taskHandle = NULL;
        taskParams[t].taskId = t;
        taskParams[t].enableHallSensors = enableHallSensors;
    }
    
    // Create tasks
    for (int t = 0; t < tasksToUse; t++) {
        char taskName[16];
        snprintf(taskName, sizeof(taskName), "DispTask%d", t);
        
        BaseType_t result = xTaskCreatePinnedToCore(
            displayTaskFunction,           // Task function
            taskName,                      // Task name
            DISPLAY_TASK_STACK_SIZE,       // Stack size
            &taskParams[t],                // Parameters
            DISPLAY_TASK_PRIORITY,         // Priority
            &taskParams[t].taskHandle,     // Task handle
            1                              // Pin to Core 1 (app core)
        );
        
        if (result != pdPASS) {
            Serial.printf("[ERROR] Failed to create task %d\n", t);
            taskCompleteFlags[t] = true;  // Mark as complete so we don't wait forever
        }
    }
    
    // Wait for all tasks to complete
    bool allComplete = false;
    while (!allComplete) {
        allComplete = true;
        for (int t = 0; t < tasksToUse; t++) {
            if (!taskCompleteFlags[t]) {
                allComplete = false;
                break;
            }
        }
        
        if (!allComplete) {
            vTaskDelay(pdMS_TO_TICKS(10));  // Check every 10ms
        }
    }
    
    Serial.println("[Threading] All tasks complete");
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
        TwoWire& wire = getWireForMux(moduleMuxes[i]);
        modules[i].start(wire);
    }
}

void SplitFlapDisplay::stopMotors() {
    // Serial.println("Stopping Motors");
    for (int i = 0; i < numModules; i++) {
        selectMuxChannel(moduleMuxes[i], moduleChannels[i]);
        TwoWire& wire = getWireForMux(moduleMuxes[i]);
        modules[i].stop(wire);
    }
}

void SplitFlapDisplay::setMqtt(SplitFlapMqtt *mqttHandler) {
    mqtt = mqttHandler;
}

// ============================================================================
// Thread-safe I2C operations
// These methods acquire the I2C mutex before performing any I2C communication
// and release it afterward. Use these from multi-threaded contexts.
// ============================================================================

void SplitFlapDisplay::i2cStepModule(int moduleIndex) {
    if (moduleIndex < 0 || moduleIndex >= numModules) return;
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        selectMuxChannel(moduleMuxes[moduleIndex], moduleChannels[moduleIndex]);
        TwoWire& wire = getWireForMux(moduleMuxes[moduleIndex]);
        modules[moduleIndex].step(true, wire);
        xSemaphoreGive(i2cMutex);
    }
}

bool SplitFlapDisplay::i2cReadSensor(int moduleIndex) {
    if (moduleIndex < 0 || moduleIndex >= numModules) return false;
    
    bool result = false;
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        selectMuxChannel(moduleMuxes[moduleIndex], moduleChannels[moduleIndex]);
        TwoWire& wire = getWireForMux(moduleMuxes[moduleIndex]);
        result = modules[moduleIndex].readHallEffectSensor(wire);
        xSemaphoreGive(i2cMutex);
    }
    return result;
}

void SplitFlapDisplay::i2cStartModule(int moduleIndex) {
    if (moduleIndex < 0 || moduleIndex >= numModules) return;
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        selectMuxChannel(moduleMuxes[moduleIndex], moduleChannels[moduleIndex]);
        TwoWire& wire = getWireForMux(moduleMuxes[moduleIndex]);
        modules[moduleIndex].start(wire);
        xSemaphoreGive(i2cMutex);
    }
}

void SplitFlapDisplay::i2cStopModule(int moduleIndex) {
    if (moduleIndex < 0 || moduleIndex >= numModules) return;
    
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        selectMuxChannel(moduleMuxes[moduleIndex], moduleChannels[moduleIndex]);
        TwoWire& wire = getWireForMux(moduleMuxes[moduleIndex]);
        modules[moduleIndex].stop(wire);
        xSemaphoreGive(i2cMutex);
    }
}

void SplitFlapDisplay::i2cMagnetDetected(int moduleIndex) {
    if (moduleIndex < 0 || moduleIndex >= numModules) return;
    
    // Note: magnetDetected() doesn't need I2C, but we protect it for consistency
    // since it's typically called after a sensor read in the same critical section
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        modules[moduleIndex].magnetDetected();
        xSemaphoreGive(i2cMutex);
    }
}

// TCA9548A Multiplexer Channel Selection
void SplitFlapDisplay::selectMuxChannel(uint8_t muxIndex, uint8_t channel) {
    if (channel > 7) return;  // TCA9548A has 8 channels (0-7)
    if (muxIndex >= numMuxes) {
        Serial.printf("[ERROR] Invalid mux index %d (max %d)\n", muxIndex, numMuxes - 1);
        return;
    }
    
    uint8_t muxAddr = muxAddrs[muxIndex];
    TwoWire& wire = getWireForMux(muxIndex);
    
    // DISABLED: Mux caching optimization disabled for parallel execution reliability
    // Always switch to ensure correct channel is selected in multi-threaded context
    
    // Enable the target channel on the target mux
    wire.beginTransmission(muxAddr);
    wire.write(1 << channel);  // Set bit for desired channel
    byte error = wire.endTransmission();
    if (error != 0) {
        Serial.printf("[ERROR] MUX 0x%02X channel select failed: error %d\n", muxAddr, error);
    }
    
    // Small delay to let the mux channel activate
    delayMicroseconds(10);
}

TwoWire& SplitFlapDisplay::getWireForMux(uint8_t muxIndex) {
    if (muxIndex >= numMuxes) return Wire;
    return (muxBus[muxIndex] == 1 && useDualBus) ? Wire1 : Wire;
}

void SplitFlapDisplay::configureI2cModules() {
    numMuxes = 0;
    memset(muxAddrs, 0, sizeof(muxAddrs));
    memset(muxBus, 0, sizeof(muxBus));
    
    // Parse Wire0 (primary bus) multiplexers
    String wire0MuxAddrsStr = settings.getString("wire0MuxAddrs");
    int start = 0;
    for (int i = 0; wire0MuxAddrsStr.length() > 0 && i <= wire0MuxAddrsStr.length() && numMuxes < 8; i++) {
        if (i == wire0MuxAddrsStr.length() || wire0MuxAddrsStr[i] == ',') {
            String addrStr = wire0MuxAddrsStr.substring(start, i);
            addrStr.trim();
            if (addrStr.length() > 0 && numMuxes < 8) {
                muxAddrs[numMuxes] = (uint8_t)addrStr.toInt();
                muxBus[numMuxes] = 0;  // Wire0
                numMuxes++;
            }
            start = i + 1;
        }
    }
    
    // Parse Wire1 (secondary bus) multiplexers if dual bus enabled
    if (useDualBus) {
        String wire1MuxAddrsStr = settings.getString("wire1MuxAddrs");
        start = 0;
        for (int i = 0; wire1MuxAddrsStr.length() > 0 && i <= wire1MuxAddrsStr.length() && numMuxes < 8; i++) {
            if (i == wire1MuxAddrsStr.length() || wire1MuxAddrsStr[i] == ',') {
                String addrStr = wire1MuxAddrsStr.substring(start, i);
                addrStr.trim();
                if (addrStr.length() > 0 && numMuxes < 8) {
                    muxAddrs[numMuxes] = (uint8_t)addrStr.toInt();
                    muxBus[numMuxes] = 1;  // Wire1
                    numMuxes++;
                }
                start = i + 1;
            }
        }
    }
    
    // Default to single mux on Wire0 if not configured
    if (numMuxes == 0) {
        muxAddrs[0] = 0x70;
        muxBus[0] = 0;
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
    memset(displayModuleStart, 0, sizeof(displayModuleStart));
    
    int flatIdx = 0;
    
    // Process each configured multiplexer
    for (int muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        // Build key name based on which bus this mux is on
        String busPrefix = (muxBus[muxIdx] == 0) ? "wire0" : "wire1";
        String addrsKey = busPrefix + "ChModAddrs" + String(muxAddrs[muxIdx]);
        String moduleAddrStr = settings.getString(addrsKey.c_str());
        
        if (moduleAddrStr.length() == 0) {
            Serial.printf("[WARN] Missing %s config\n", addrsKey.c_str());
            continue;
        }
        
        // Parse channel addresses and derive counts: "32;;;;;;;;" or "32;33,34;;;;;;;"
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
                        displayModuleStart[numDisplays] = flatIdx - addrCount;  // First module index for this display
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
    Serial.printf("Dual bus mode: %s\n", useDualBus ? "ENABLED" : "DISABLED");
    for (int muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        bool muxHasModules = false;
        for (int i = 0; i < numModules; i++) {
            if (moduleMuxes[i] == muxIdx) {
                muxHasModules = true;
                break;
            }
        }
        
        if (muxHasModules) {
            Serial.printf("Mux%d (0x%02X) on %s: ", muxIdx, muxAddrs[muxIdx],
                          muxBus[muxIdx] == 1 ? "Wire1" : "Wire");
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
    
    // Scan Wire bus
    Serial.println("Wire (primary bus):");
    scanBus(Wire, 0);
    
    // Scan Wire1 if enabled
    if (useDualBus) {
        Serial.println("\nWire1 (secondary bus):");
        scanBus(Wire1, 1);
    }
    
    Serial.println("===================\n");
}

void SplitFlapDisplay::scanBus(TwoWire& wire, uint8_t busNum) {
    // Scan all 8 possible TCA9548A addresses
    for (uint8_t addr = 0x70; addr <= 0x77; addr++) {
        wire.beginTransmission(addr);
        if (wire.endTransmission() == 0) {
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
                    wire.beginTransmission(a);
                    wire.write(0x00);
                    wire.endTransmission();
                }
                delay(10);
                
                // Enable only the target channel on the target mux
                wire.beginTransmission(addr);
                wire.write(1 << channel);
                wire.endTransmission();
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
                    
                    wire.beginTransmission(devAddr);
                    if (wire.endTransmission() == 0) {
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
                wire.beginTransmission(a);
                wire.write(0x00);
                wire.endTransmission();
            }
            
            Serial.println();
        }
    }
}

// Home all active displays in parallel
void SplitFlapDisplay::homeAllChannels(float speed) {
    // Phase 1: Trigger homing for all modules
    int targetPositions[numModules];
    for (int i = 0; i < numModules; i++) {
        targetPositions[i] = (modules[i].getPosition() - 1 + stepsPerRot) % stepsPerRot;
    }
    startMotors();
    moveTo(targetPositions, speed, false, true);
    delay(1000);
    
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

// Get the starting module index for a given display
int SplitFlapDisplay::getModuleStartIndex(int displayIndex) const {
    if (displayIndex < 0 || displayIndex >= numDisplays) return 0;
    return displayModuleStart[displayIndex];
}

// Static task entry point for FreeRTOS
void SplitFlapDisplay::displayTaskFunction(void* params) {
    DisplayTaskParams* taskParams = (DisplayTaskParams*)params;
    
    // Call the actual processing function on the display object
    taskParams->display->processDisplayRange(taskParams);
    
    // Signal completion
    *(taskParams->taskComplete) = true;
    
    // Delete this task when done
    vTaskDelete(NULL);
}

// Static task entry point for dual-bus FreeRTOS tasks
void SplitFlapDisplay::dualBusTaskFunction(void* params) {
    DualBusTaskParams* taskParams = (DualBusTaskParams*)params;
    
    // Call the actual processing function on the display object
    taskParams->display->processBusRange(taskParams);
    
    // Signal completion
    *(taskParams->taskComplete) = true;
    
    // Delete this task when done
    vTaskDelete(NULL);
}

// Process all modules on a single I2C bus (called from task context)
// NO MUTEX NEEDED - each task has exclusive access to its own I2C bus hardware
void SplitFlapDisplay::processBusRange(DualBusTaskParams* params) {
    uint8_t busIndex = params->busIndex;
    int* targetPositions = params->targetPositions;
    unsigned long stepIntervalUs = params->stepIntervalUs;
    int checkIntervalSteps = params->checkIntervalSteps;
    bool releaseMotors = params->releaseMotors;
    bool enableHallSensors = params->enableHallSensors;
    
    int startStopDelay = 200;  // Motor start/stop delay in ms
    
    // Get the wire object for this bus
    TwoWire& wire = (busIndex == 1 && useDualBus) ? Wire1 : Wire;
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Bus%d] Starting parallel execution\n", busIndex);
#endif
    
    unsigned long moveStartTime = micros();
    unsigned long totalI2cTimeUs = 0;
    unsigned long totalSteps = 0;
    unsigned long burstCount = 0;
    
    // Track which modules on THIS BUS need to move
    int stepsRemaining[numModules];
    bool isActive[numModules];
    bool resetLatches[numModules] = {};
    int numActive = 0;
    
    // Filter for modules on this bus only
    for (int i = 0; i < numModules; i++) {
        // Skip modules not on our bus
        if (muxBus[moduleMuxes[i]] != busIndex) {
            stepsRemaining[i] = 0;
            isActive[i] = false;
            continue;
        }
        
        // Acquire mutex to safely read module state
        xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
        int current = modules[i].getPosition();
        xSemaphoreGive(params->display->getModuleStateMutex());
        
        int target = targetPositions[i];
        
        if (current != target) {
            stepsRemaining[i] = (target - current + stepsPerRot) % stepsPerRot;
            if (stepsRemaining[i] == 0) stepsRemaining[i] = stepsPerRot;
            isActive[i] = true;
            resetLatches[i] = true;
            numActive++;
        } else {
            stepsRemaining[i] = 0;
            isActive[i] = false;
        }
    }
    
    if (numActive == 0) {
#if SPLITFLAP_DEBUG
        Serial.printf("[Bus%d] No modules to move\n", busIndex);
#endif
        return;
    }
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Bus%d] Moving %d modules: ", busIndex, numActive);
    for (int i = 0; i < numModules; i++) {
        if (isActive[i]) {
            // Acquire mutex to safely read module position for debug output
            xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
            int curPos = modules[i].getPosition();
            xSemaphoreGive(params->display->getModuleStateMutex());
            
            Serial.printf("M%d(mux%d,ch%d,cur%d->tgt%d) ", i, moduleMuxes[i], moduleChannels[i], 
                         curPos, targetPositions[i]);
        }
    }
    Serial.println();
#endif
    
    // Start all active motors on this bus
    for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
        // Skip muxes not on our bus
        if (muxBus[muxIdx] != busIndex) continue;
        
        for (uint8_t channel = 0; channel < 8; channel++) {
            bool selected = false;
            for (int i = 0; i < numModules; i++) {
                if (!isActive[i]) continue;
                if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                
                if (!selected) {
                    selectMuxChannel(muxIdx, channel);
                    selected = true;
                }
                // Acquire mutex for module state modification
                xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
                modules[i].start(wire);
                xSemaphoreGive(params->display->getModuleStateMutex());
            }
        }
    }
    delay(startStopDelay);
    
    // Main stepping loop - burst stepping for this bus
    unsigned long lastBurstTime = micros();
    int stepsSinceLastSensorCheck = 0;
    
    unsigned long estimatedBurstTimeUs = numActive * 150;
    bool i2cLimited = (estimatedBurstTimeUs > stepIntervalUs);
    
    while (numActive > 0) {
        unsigned long now = micros();
        
        if (!i2cLimited && (now - lastBurstTime) < stepIntervalUs) {
            continue;
        }
        lastBurstTime = now;
        burstCount++;
        
        // Step all active modules on this bus
        for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
            if (muxBus[muxIdx] != busIndex) continue;
            
            for (uint8_t channel = 0; channel < 8; channel++) {
                bool selected = false;
                
                for (int i = 0; i < numModules; i++) {
                    if (!isActive[i]) continue;
                    if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                    
                    if (!selected) {
                        selectMuxChannel(muxIdx, channel);
                        selected = true;
                    }
                    
                    unsigned long i2cStart = micros();
                    // Acquire mutex for module state modification
                    xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
                    modules[i].step(true, wire);
                    xSemaphoreGive(params->display->getModuleStateMutex());
                    totalI2cTimeUs += micros() - i2cStart;
                    totalSteps++;
                    
                    stepsRemaining[i]--;
                    if (stepsRemaining[i] <= 0) {
                        isActive[i] = false;
                        numActive--;
                        i2cLimited = (numActive * 150 > stepIntervalUs);
                    }
                }
            }
        }
        
        // Check hall sensors periodically
        if (enableHallSensors && checkIntervalSteps > 0) {
            stepsSinceLastSensorCheck++;
            if (stepsSinceLastSensorCheck >= checkIntervalSteps) {
                stepsSinceLastSensorCheck = 0;
                
                for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
                    if (muxBus[muxIdx] != busIndex) continue;
                    
                    for (uint8_t channel = 0; channel < 8; channel++) {
                        bool selected = false;
                        
                        for (int i = 0; i < numModules; i++) {
                            if (!isActive[i]) continue;
                            if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                            
                            if (!selected) {
                                selectMuxChannel(muxIdx, channel);
                                selected = true;
                            }
                            
                            unsigned long i2cStart = micros();
                            // Acquire mutex for module state access
                            xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
                            bool sensorTriggered = modules[i].readHallEffectSensor(wire);
                            
                            if (sensorTriggered) {
                                if (!resetLatches[i]) {
                                    modules[i].magnetDetected();
                                    resetLatches[i] = true;
                                }
                            } else if (resetLatches[i]) {
                                resetLatches[i] = false;
                            }
                            xSemaphoreGive(params->display->getModuleStateMutex());
                            totalI2cTimeUs += micros() - i2cStart;
                        }
                    }
                }
            }
        }
    }
    
    // Stop motors if requested
    if (releaseMotors) {
        delay(startStopDelay);
        for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
            if (muxBus[muxIdx] != busIndex) continue;
            
            for (uint8_t channel = 0; channel < 8; channel++) {
                bool selected = false;
                for (int i = 0; i < numModules; i++) {
                    // Acquire mutex to check position
                    xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
                    bool atTarget = (modules[i].getPosition() == targetPositions[i]);
                    xSemaphoreGive(params->display->getModuleStateMutex());
                    
                    if (!atTarget) continue;
                    if (moduleMuxes[i] != muxIdx || moduleChannels[i] != channel) continue;
                    
                    if (!selected) {
                        selectMuxChannel(muxIdx, channel);
                        selected = true;
                    }
                    // Acquire mutex for module state modification
                    xSemaphoreTake(params->display->getModuleStateMutex(), portMAX_DELAY);
                    modules[i].stop(wire);
                    xSemaphoreGive(params->display->getModuleStateMutex());
                }
            }
        }
    }
    
    unsigned long totalTimeMs = (micros() - moveStartTime) / 1000;
    float stepsPerSec = totalSteps * 1000000.0f / (micros() - moveStartTime);
    float i2cUtilization = 100.0f * totalI2cTimeUs / (micros() - moveStartTime);
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Bus%d] DONE: %lu steps in %lums (%.0f steps/sec)\n", 
                  busIndex, totalSteps, totalTimeMs, stepsPerSec);
    Serial.printf("[Bus%d] I2C: %lums (%.1f%% util), Bursts: %lu\n",
                  busIndex, totalI2cTimeUs / 1000, i2cUtilization, burstCount);
#endif
}

// Process a range of displays (called from task context)
// Uses time-sliced batching: acquire mutex once, process multiple steps, release, yield
// This reduces mutex contention compared to per-operation locking
void SplitFlapDisplay::processDisplayRange(DisplayTaskParams* params) {
    int startDisp = params->startDisplayIndex;
    int endDisp = params->endDisplayIndex;
    float timePerStep = params->timePerStep;
    int checkIntervalUs = params->checkIntervalUs;
    int* targetPositions = params->targetPositions;
    int taskId = params->taskId;
    bool enableHallSensors = params->enableHallSensors;
    
    // Time slice duration in microseconds - how long each task holds the mutex
    const unsigned long TIME_SLICE_US = 10000;  // 10ms per batch
    
    // Debug instrumentation
    unsigned long totalMutexWaitUs = 0;
    unsigned long totalMutexHoldUs = 0;
    unsigned long totalI2cTimeUs = 0;
    unsigned long totalSteps = 0;
    unsigned long loopIterations = 0;
    unsigned long moveStartTime = micros();
    
    // Build list of modules in our display range that need to move
    int activeModules[MAX_MODULES];
    int numActive = 0;
    bool resetLatches[MAX_MODULES] = {};
    unsigned long lastStepTimes[MAX_MODULES] = {};
    
    unsigned long currentTime = micros();
    unsigned long lastSensorCheckTime = currentTime;
    
    // Identify active modules in our assigned display range
    for (int dispIdx = startDisp; dispIdx < endDisp; dispIdx++) {
        int moduleStart = displayModuleStart[dispIdx];
        int moduleCount = displayModuleCount[dispIdx];
        
        for (int m = 0; m < moduleCount; m++) {
            int moduleIdx = moduleStart + m;
            if (moduleIdx >= numModules) continue;
            
            lastStepTimes[moduleIdx] = currentTime;
            
            if (modules[moduleIdx].getPosition() != targetPositions[moduleIdx]) {
                activeModules[numActive++] = moduleIdx;
                resetLatches[moduleIdx] = true;
            }
        }
    }
    
    if (numActive == 0) {
        return;  // Nothing to do for this task
    }
    
    int initialActive = numActive;
#if SPLITFLAP_DEBUG
    Serial.printf("[Task%d] Starting: %d modules, displays %d-%d\n", 
                  taskId, numActive, startDisp, endDisp - 1);
#endif
    
    // Start motors for active modules - batch under single mutex lock
    unsigned long waitStart = micros();
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        totalMutexWaitUs += micros() - waitStart;
        unsigned long holdStart = micros();
        
        for (int j = 0; j < numActive; j++) {
            int moduleIdx = activeModules[j];
            unsigned long i2cStart = micros();
            selectMuxChannel(moduleMuxes[moduleIdx], moduleChannels[moduleIdx]);
            TwoWire& wire = getWireForMux(moduleMuxes[moduleIdx]);
            modules[moduleIdx].start(wire);
            totalI2cTimeUs += micros() - i2cStart;
        }
        
        totalMutexHoldUs += micros() - holdStart;
        xSemaphoreGive(i2cMutex);
    }
    
    // Small delay to let motors align to magnetic field
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Main stepping loop with time-sliced batching
    while (numActive > 0) {
        loopIterations++;
        
        // Acquire mutex for a time slice
        unsigned long waitStart = micros();
        if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
            totalMutexWaitUs += micros() - waitStart;
            unsigned long holdStart = micros();
            unsigned long sliceStart = micros();
            
            // Process within this time slice
            while ((micros() - sliceStart) < TIME_SLICE_US && numActive > 0) {
                currentTime = micros();
                
                // Step all motors grouped by mux/channel to minimize mux switching
                for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
                    for (uint8_t channel = 0; channel < 8; channel++) {
                        bool selected = false;
                        for (int j = 0; j < numActive; j++) {
                            int moduleIdx = activeModules[j];
                            if (moduleMuxes[moduleIdx] != muxIdx || moduleChannels[moduleIdx] != channel) {
                                continue;
                            }
                            
                            if ((currentTime - lastStepTimes[moduleIdx]) > timePerStep) {
                                if (!selected) {
                                    selectMuxChannel(muxIdx, channel);
                                    selected = true;
                                }
                                // Direct I2C access (we already hold the mutex)
                                unsigned long i2cStart = micros();
                                TwoWire& wire = getWireForMux(muxIdx);
                                modules[moduleIdx].step(true, wire);
                                totalI2cTimeUs += micros() - i2cStart;
                                totalSteps++;
                                
                                lastStepTimes[moduleIdx] = micros();
                                
                                // Check if module reached target
                                if (modules[moduleIdx].getPosition() == targetPositions[moduleIdx]) {
                                    // Remove from active list by swapping with last
                                    activeModules[j] = activeModules[numActive - 1];
                                    numActive--;
                                    j--;  // Re-check this position since we swapped
                                }
                            }
                        }
                    }
                }
                
                // Check hall effect sensors periodically
                if (enableHallSensors && (currentTime - lastSensorCheckTime) > checkIntervalUs) {
                    // Read sensors grouped by mux/channel to minimize mux switching
                    for (uint8_t muxIdx = 0; muxIdx < numMuxes; muxIdx++) {
                        for (uint8_t channel = 0; channel < 8; channel++) {
                            bool selected = false;
                            for (int j = 0; j < numActive; j++) {
                                int moduleIdx = activeModules[j];
                                if (moduleMuxes[moduleIdx] != muxIdx || moduleChannels[moduleIdx] != channel) {
                                    continue;
                                }
                                if (!selected) {
                                    selectMuxChannel(muxIdx, channel);
                                    selected = true;
                                }
                                
                                // Direct sensor read (we already hold the mutex)
                                unsigned long i2cStart = micros();
                                TwoWire& wire = getWireForMux(muxIdx);
                                bool sensorTriggered = modules[moduleIdx].readHallEffectSensor(wire);
                                totalI2cTimeUs += micros() - i2cStart;
                                
                                if (sensorTriggered) {
                                    if (!resetLatches[moduleIdx]) {
                                        // Magnet detected - update position
                                        modules[moduleIdx].magnetDetected();
                                        resetLatches[moduleIdx] = true;
                                    }
                                } else if (resetLatches[moduleIdx]) {
                                    resetLatches[moduleIdx] = false;
                                }
                            }
                        }
                    }
                    lastSensorCheckTime = micros();
                }
            }
            
            totalMutexHoldUs += micros() - holdStart;
            // Release mutex after time slice
            xSemaphoreGive(i2cMutex);
        }
        
        // Yield to other tasks - this is where other tasks get their time slice
        taskYIELD();
    }
    
    // Stop motors that were moving - batch under single mutex lock
    unsigned long stopWaitStart = micros();
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY) == pdTRUE) {
        totalMutexWaitUs += micros() - stopWaitStart;
        unsigned long holdStart = micros();
        
        for (int dispIdx = startDisp; dispIdx < endDisp; dispIdx++) {
            int moduleStart = displayModuleStart[dispIdx];
            int moduleCount = displayModuleCount[dispIdx];
            
            for (int m = 0; m < moduleCount; m++) {
                int moduleIdx = moduleStart + m;
                if (moduleIdx >= numModules) continue;
                
                if (modules[moduleIdx].getPosition() == targetPositions[moduleIdx]) {
                    unsigned long i2cStart = micros();
                    selectMuxChannel(moduleMuxes[moduleIdx], moduleChannels[moduleIdx]);
                    TwoWire& wire = getWireForMux(moduleMuxes[moduleIdx]);
                    modules[moduleIdx].stop(wire);
                    totalI2cTimeUs += micros() - i2cStart;
                }
            }
        }
        
        totalMutexHoldUs += micros() - holdStart;
        xSemaphoreGive(i2cMutex);
    }
    
    // Print debug summary
    unsigned long totalTimeMs = (micros() - moveStartTime) / 1000;
    float stepsPerSec = totalSteps * 1000000.0f / (micros() - moveStartTime);
    float i2cUtilization = 100.0f * totalI2cTimeUs / (micros() - moveStartTime);
    float mutexWaitPct = 100.0f * totalMutexWaitUs / (micros() - moveStartTime);
    
#if SPLITFLAP_DEBUG
    Serial.printf("[Task%d] DONE: %lu steps in %lums (%.0f steps/sec)\n", 
                  taskId, totalSteps, totalTimeMs, stepsPerSec);
    Serial.printf("[Task%d] I2C: %lums (%.1f%% util), Mutex wait: %lums (%.1f%%), Loop iters: %lu\n",
                  taskId, totalI2cTimeUs / 1000, i2cUtilization, 
                  totalMutexWaitUs / 1000, mutexWaitPct, loopIterations);
#endif
}
