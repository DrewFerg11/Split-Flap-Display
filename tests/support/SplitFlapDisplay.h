#pragma once

#include "JsonSettings.h"

constexpr float MAX_RPM = 15.0f;
class SplitFlapMqtt;

struct SplitFlapModule
{
    inline static const char StandardChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    inline static const char ExtendedChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\':?!.-/$@#%";
};

class SplitFlapDisplay {
  public:
    explicit SplitFlapDisplay(JsonSettings &) {}
    int writeCalls = 0, initCalls = 0, homeCalls = 0, numModules = 8, wire1Count = 0;
    std::string lastMessage, row1, row2;
    void init() { ++initCalls; }
    void homeToString(String) { ++homeCalls; }
    void setMqtt(SplitFlapMqtt *) {}
    int getNumModules() const { return numModules; }
    int getCharsetSize() const { return 37; }
    void writeString(String value, float = MAX_RPM, bool = true) {
        ++writeCalls;
        lastMessage = value.c_str();
    }
    void writeChar(char value) {
        ++writeCalls;
        lastMessage = std::string(1, value);
    }
    int getWire1Count() const { return wire1Count; }
    void homeToStringDual(String, String, float = MAX_RPM, bool = true) { ++homeCalls; }
    void writeStringDual(String first, String second, float = MAX_RPM, bool = true) {
        ++writeCalls;
        row1 = first.c_str();
        row2 = second.c_str();
    }
    void testRandom() { ++writeCalls; }
};
