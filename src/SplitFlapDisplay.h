#pragma once

#include "JsonSettings.h"
#include "SplitFlapModule.h"

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define MAX_MODULES 64 // Realistic limit: 8 muxes × 8 channels × 1 address typical
#define MAX_DISPLAYS 64
#define MAX_RPM 15.0f
#define SPLITFLAP_DEBUG 1
#define DISPLAY_TASK_STACK_SIZE 4096
#define DISPLAY_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define MAX_DISPLAY_TASKS 8  // Maximum concurrent display tasks

class SplitFlapMqtt;
class SplitFlapDisplay;

// Parameters passed to each display task
struct DisplayTaskParams {
    SplitFlapDisplay* display;   // Pointer to parent display object
    int* targetPositions;        // Target positions array (shared)
    int startDisplayIndex;       // First display this task handles
    int endDisplayIndex;         // Last display (exclusive) this task handles
    float timePerStep;           // Microseconds per step
    int checkIntervalUs;         // Hall sensor check interval
    TaskHandle_t taskHandle;     // Handle for this task
    volatile bool* taskComplete; // Flag to signal completion
    int taskId;                  // Task identifier for debug output
    bool enableHallSensors;      // Enable hall sensor polling during move
};

// Parameters passed to each dual-bus task (one task per I2C bus)
struct DualBusTaskParams {
    SplitFlapDisplay* display;     // Pointer to parent display object
    int* targetPositions;          // Target positions array (shared)
    uint8_t busIndex;              // Which I2C bus (0=Wire, 1=Wire1)
    unsigned long stepIntervalUs;  // Microseconds per step
    int checkIntervalSteps;        // Hall sensor check interval (in steps)
    bool releaseMotors;            // Release motors when done
    bool enableHallSensors;        // Enable hall sensor polling during move
    TaskHandle_t taskHandle;       // Handle for this task
    volatile bool* taskComplete;   // Flag to signal completion
};

class SplitFlapDisplay {
  public:
    SplitFlapDisplay(JsonSettings &settings);

    void init();
    
    // Multi-mux display methods (preferred)
    void homeAllChannels(float speed = MAX_RPM);  // Home all active channels in parallel
    void writeDisplays(
        String displayTexts[8],
        float speed = MAX_RPM,
        bool centering = true
    );                                            // Write text to displays connected across multiple muxes/channels
    
    // Legacy single-mux methods
    void writeString(
        String inputString, float speed = MAX_RPM,
        bool centering = true
    );                                     // Move all modules at once to show a specific string
    void writeStringPerChannel(
        String channelStrings[], 
        float speed = MAX_RPM,
        bool centering = true
    );                                     // Write different strings to each channel with optional centering
    void writeChar(char inputChar,
                   float speed = MAX_RPM); // sets all modules to a single char
    void home(float speed = MAX_RPM);      // move home
    void homeToString(
        String homeString, float speed = MAX_RPM,
        bool centering = true
    );                                      // moves home and then writes a string
    void homeToChar(char homeChar,
                    float speed = MAX_RPM); // moves home and then sets all modules to a char
    
    // Low-level methods
    void moveTo(int targetPositions[], float speed = MAX_RPM, bool releaseMotors = true, bool enableHallSensors = false);
    void testAll();
    void testCount();
    void testRandom(float speed = MAX_RPM);
    int getNumModules() { return numModules; }
    int getCharsetSize() const { return charSetSize; }
    void setMqtt(SplitFlapMqtt *mqttHandler);
    
    // Threading control
    void setNumDisplayTasks(int numTasks) { numDisplayTasks = constrain(numTasks, 1, MAX_DISPLAY_TASKS); }
    int getNumDisplayTasks() const { return numDisplayTasks; }
    
    // Display configuration access for web UI
    int getNumDisplays() const { return numDisplays; }
    uint8_t getDisplayMux(int displayIndex) const { return displayIndex < numDisplays ? displayMux[displayIndex] : 0; }
    uint8_t getDisplayChannel(int displayIndex) const { return displayIndex < numDisplays ? displayChannel[displayIndex] : 0; }
    uint8_t getDisplayModuleCount(int displayIndex) const { return displayIndex < numDisplays ? displayModuleCount[displayIndex] : 0; }
    
    // FreeRTOS threading - expose for task callback
    SemaphoreHandle_t getI2cMutex() { return i2cMutex; }
    SemaphoreHandle_t getModuleStateMutex() { return moduleStateMutex; }
    SplitFlapModule* getModules() { return modules; }
    uint8_t* getModuleMuxes() { return moduleMuxes; }
    uint8_t* getModuleChannels() { return moduleChannels; }
    int getModuleStartIndex(int displayIndex) const;  // Get first module index for a display
    int getMagnetPosition() const { return magnetPosition; }

  private:
    JsonSettings &settings;

    bool checkAllFalse(bool array[], int size);
    void stopMotors();
    void startMotors();
    
    // TCA9548A I2C Multiplexer helpers
    void selectMuxChannel(uint8_t muxAddr, uint8_t channel);
    void configureI2cModules();
    void scanI2cModules();
    void scanBus(TwoWire& wire, uint8_t busNum);
    
    // Thread-safe I2C operations (acquire mutex before I2C, release after)
    void i2cStepModule(int moduleIndex);           // Thread-safe: select mux + step motor
    bool i2cReadSensor(int moduleIndex);           // Thread-safe: select mux + read hall sensor
    void i2cStartModule(int moduleIndex);          // Thread-safe: select mux + start motor
    void i2cStopModule(int moduleIndex);           // Thread-safe: select mux + stop motor
    void i2cMagnetDetected(int moduleIndex);       // Thread-safe: select mux + update position
    
    // Multi-threaded movement
    void moveToThreaded(int targetPositions[], float timePerStep, int checkIntervalUs, bool releaseMotors, bool enableHallSensors);
    void moveToDualBus(int targetPositions[], unsigned long stepIntervalUs, int checkIntervalSteps, bool releaseMotors, bool enableHallSensors);
    void processBusRange(DualBusTaskParams* params);  // Process modules on single I2C bus

    int numModules;
    int moduleCountPerChannel[8];  // Per-channel module counts
    SplitFlapModule modules[MAX_MODULES];
    int moduleOffsets[MAX_MODULES];
    uint8_t moduleMuxes[MAX_MODULES];     // Stores which mux index (0-7) each individual module is connected to
    uint8_t moduleChannels[MAX_MODULES];  // Stores which channel (0-7) each individual module is on
    uint8_t moduleAddresses[MAX_MODULES]; // Stores which I2C address (0-7) of each individual module
    int displayOffset;

    float maxVel;       // Max Velocity In RPM
    int charSetSize;    // 37 for standard, 48 for extended
    int stepsPerRot;    // number of motor steps per full rotation of character
                        // drum
    int magnetPosition; // position of drum wheel when magnet is detected
    int SDAPin;         // SDA pin
    int SCLPin;         // SCL pin
    int SDA1Pin;        // Secondary I2C bus SDA pin
    int SCL1Pin;        // Secondary I2C bus SCL pin
    bool useDualBus;    // Whether Wire1 is enabled
    
    uint8_t muxAddrs[8];  // TCA9548A I2C multiplexer addresses (up to 8)
    uint8_t muxBus[8];    // Which I2C bus each mux is on (0=Wire, 1=Wire1)
    uint8_t numMuxes;     // Number of configured multiplexers
    
    // Helper to get the Wire object for a given mux
    TwoWire& getWireForMux(uint8_t muxIndex);
    
    // Display tracking (ordered by mux address, then channel)
    int numDisplays;           // Number of configured displays
    uint8_t displayMux[64];    // Mux index for each display
    uint8_t displayChannel[64]; // Channel for each display
    uint8_t displayModuleCount[64]; // Number of modules per display
    int displayModuleStart[64];     // Starting module index for each display

    SplitFlapMqtt *mqtt = nullptr;
    
    // FreeRTOS threading infrastructure
    SemaphoreHandle_t i2cMutex;           // Mutex for I2C bus access
    SemaphoreHandle_t moduleStateMutex;   // Mutex for module state access (getPosition, etc.)
    int numDisplayTasks;                   // Number of parallel tasks to use
    volatile bool taskCompleteFlags[MAX_DISPLAY_TASKS];  // Completion flags for tasks
    
    // Threading helper methods
    void initThreading();
    static void displayTaskFunction(void* params);  // Static task entry point
    void processDisplayRange(DisplayTaskParams* params);  // Actual work function
    static void dualBusTaskFunction(void* params);  // Static task entry point for dual bus
};
