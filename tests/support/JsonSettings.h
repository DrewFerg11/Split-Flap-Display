#pragma once

#include <Arduino.h>
#include <initializer_list>
#include <utility>

class JsonSetting {
  public:
    template <typename T> JsonSetting(T) {}
    JsonSetting(std::initializer_list<int>) {}
};

class JsonSettings {
  public:
    JsonSettings(const char *, std::initializer_list<std::pair<const char *, JsonSetting>>) {}
    String getString(const char *) const { return ""; }
    float getFloat(const char *) const { return 15.0f; }
    void putString(const char *, String) {}
};
