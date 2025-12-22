#pragma once

#include "JsonSettings.h"
#include "SplitFlapModule.h"

#include <Arduino.h>

#define MAX_MODULES 64 // Realistic limit: 8 muxes × 8 channels × 1 address typical
#define MAX_RPM 15.0f

class SplitFlapMqtt;

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
    void moveTo(int targetPositions[], float speed = MAX_RPM, bool releaseMotors = true);
    void testAll();
    void testCount();
    void testRandom(float speed = MAX_RPM);
    int getNumModules() { return numModules; }
    int getCharsetSize() const { return charSetSize; }
    void setMqtt(SplitFlapMqtt *mqttHandler);
    
    // Display configuration access for web UI
    int getNumDisplays() const { return numDisplays; }
    uint8_t getDisplayMux(int displayIndex) const { return displayIndex < numDisplays ? displayMux[displayIndex] : 0; }
    uint8_t getDisplayChannel(int displayIndex) const { return displayIndex < numDisplays ? displayChannel[displayIndex] : 0; }
    uint8_t getDisplayModuleCount(int displayIndex) const { return displayIndex < numDisplays ? displayModuleCount[displayIndex] : 0; }

  private:
    JsonSettings &settings;

    bool checkAllFalse(bool array[], int size);
    void stopMotors();
    void startMotors();
    
    // TCA9548A I2C Multiplexer helpers
    void selectMuxChannel(uint8_t muxAddr, uint8_t channel);
    void configureI2cModules();
    void scanI2cModules();

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
    
    uint8_t muxAddrs[8];  // TCA9548A I2C multiplexer addresses (up to 8)
    uint8_t numMuxes;     // Number of configured multiplexers
    
    // Display tracking (ordered by mux address, then channel)
    int numDisplays;           // Number of configured displays
    uint8_t displayMux[64];    // Mux index for each display
    uint8_t displayChannel[64]; // Channel for each display
    uint8_t displayModuleCount[64]; // Number of modules per display

    SplitFlapMqtt *mqtt = nullptr;
};
