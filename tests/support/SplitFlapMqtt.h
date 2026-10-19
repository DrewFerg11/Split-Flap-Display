#pragma once

#include "JsonSettings.h"
#include "WiFiClient.h"

class SplitFlapDisplay;

// Only the MQTT surface used by the sketch. No broker or socket is involved.
class SplitFlapMqtt {
  public:
    SplitFlapMqtt(JsonSettings &, WiFiClient &) {}
    int loopCalls = 0, setupCalls = 0;

    void loop() { ++loopCalls; }
    void setup() { ++setupCalls; }
    void setDisplay(SplitFlapDisplay *) {}
};
