#pragma once
#include <Arduino.h>
class SplitFlapMqtt {
  public:
    bool connected = true;
    std::string state;
    bool isConnected() const { return connected; }
    void publishState(const String &message) { state = message.c_str(); }
};
