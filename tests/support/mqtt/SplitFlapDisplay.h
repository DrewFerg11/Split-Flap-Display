#pragma once

#include <Arduino.h>

// Records commands delivered by the real MQTT callback, without motor hardware.
class SplitFlapDisplay {
  public:
    std::string lastMessage;
    int writeCalls = 0;
    int getNumModules() const { return 8; }
    int getWire1Count() const { return 0; }
    void writeString(String message, float, bool) {
        lastMessage = message.c_str();
        ++writeCalls;
    }
};
