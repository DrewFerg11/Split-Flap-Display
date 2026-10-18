#pragma once

#include "JsonSettings.h"

constexpr float MAX_RPM = 15.0f;
class SplitFlapMqtt;

struct SplitFlapModule
{
    inline static const char StandardChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    inline static const char ExtendedChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?+-*/:;=@";
};

class SplitFlapDisplay {
  public:
    explicit SplitFlapDisplay(JsonSettings &) {}
    int writeCalls = 0;
    void init() {}
    void homeToString(String) {}
    void setMqtt(SplitFlapMqtt *) {}
    int getNumModules() const { return 8; }
    int getCharsetSize() const { return 37; }
    void writeString(String, float = MAX_RPM, bool = true) { ++writeCalls; }
    void writeChar(char) { ++writeCalls; }
    void testRandom() { ++writeCalls; }
};
