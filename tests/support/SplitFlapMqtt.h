#pragma once

// Only the MQTT surface used by AppLoop.cpp. No broker or socket is involved.
class SplitFlapMqtt {
  public:
    int loopCalls = 0;

    void loop() { ++loopCalls; }
};
