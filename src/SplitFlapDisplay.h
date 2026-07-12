#pragma once

#include "JsonSettings.h"
#include "SplitFlapModule.h"

#include <Arduino.h>

#ifdef ENABLE_DUAL_I2C
#define MAX_MODULES 16
#else
#define MAX_MODULES 8
#endif
#define MAX_RPM 15.0f

class SplitFlapMqtt;

class SplitFlapDisplay {
  public:
    SplitFlapDisplay(JsonSettings &settings);

    void init();
    void writeString(
        String inputString, float speed = MAX_RPM,
        bool centering = true
    );                                     // Move all modules at once to show a specific string
    void writeChar(char inputChar,
                   float speed = MAX_RPM); // sets all modules to a single char
    void moveTo(int targetPositions[], float speed = MAX_RPM, bool releaseMotors = true);
    void home(float speed = MAX_RPM);      // move home
    void homeToString(
        String homeString, float speed = MAX_RPM,
        bool centering = true
    );                                      // moves home and then writes a string
    void homeToChar(char homeChar,
                    float speed = MAX_RPM); // moves home and then sets all modules to a char
    void testAll();
    void testCount();
    void testRandom(float speed = MAX_RPM);
    int getNumModules() { return numModules; }
    int getWire1Count() const { return wire1Count; }
    int getCharsetSize() const { return charSetSize; }
    void setMqtt(SplitFlapMqtt *mqttHandler);

    // Static movement engine — operates on a subset of modules.
    // `idleCallback` runs every ~20ms during the move (used for Improv Wi-Fi).
    // Must be nullptr when called from a background task — not thread-safe.
    static void moveModules(
        SplitFlapModule *mods, int count, int *targets, float speed, bool releaseMotors, int stepsPerRot, float maxVel,
        void (*idleCallback)() = nullptr
    );

#ifdef ENABLE_DUAL_I2C
    void homeToStringDual(String row1, String row2, float speed = MAX_RPM, bool centering = true);
    void writeStringDual(String row1, String row2, float speed = MAX_RPM, bool centering = true);
#endif

  private:
    JsonSettings &settings;

    static bool checkAllFalse(bool array[], int size);
    static void stopMotors(SplitFlapModule *mods, int count);
    static void startMotors(SplitFlapModule *mods, int count);
    void configI2cModules();
    void scanI2cModules();

#ifdef ENABLE_DUAL_I2C
    void moveToDual(int *targetPositions, float speed, bool releaseMotors);
#endif

    int numModules;
    SplitFlapModule modules[MAX_MODULES];
    int displayOffset;

    // Wire (Bus 1)
    uint8_t wireAddresses[MAX_MODULES];
    int wireOffsets[MAX_MODULES];
    int wireCount;
    int SDAPin;
    int SCLPin;

    int wire1Count = 0; // Always declared; 0 when dual I2C disabled

#ifdef ENABLE_DUAL_I2C
    // Wire1 (Bus 2)
    uint8_t wire1Addresses[MAX_MODULES];
    int wire1Offsets[MAX_MODULES];
    int SDA2Pin;
    int SCL2Pin;
#endif

    float maxVel;       // Max Velocity In RPM
    int charSetSize;    // 37 for standard, 48 for extended
    int stepsPerRot;    // number of motor steps per full rotation of character drum
    int magnetPosition; // position of drum wheel when magnet is detected

    SplitFlapMqtt *mqtt = nullptr;
};
