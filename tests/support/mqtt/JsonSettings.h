#pragma once

#include <Arduino.h>
#include <map>

class JsonSettings {
  public:
    std::map<std::string, std::string> strings = {
        {"mqtt_server", "broker.test"}, {"mdns", "clock"}, {"name", "Test clock"}
    };
    String getString(const char *key) { return strings[key]; }
    int getInt(const char *) const { return 1883; }
    float getFloat(const char *) const { return 15.0f; }
};
