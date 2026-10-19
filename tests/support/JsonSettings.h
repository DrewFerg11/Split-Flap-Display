#pragma once
#include <Arduino.h>
#include <initializer_list>
#include <map>
#include <utility>
class JsonSetting {
  public:
    String text;
    JsonSetting(const char *v) : text(v) {}
    JsonSetting(String v) : text(v) {}
    template <typename T> JsonSetting(T) {}
    JsonSetting(std::initializer_list<int>) {}
};
class JsonSettings {
  public:
    JsonSettings(const char *, std::initializer_list<std::pair<const char *, JsonSetting>> defaults) {
        for (const auto &entry : defaults) strings[entry.first] = entry.second.text.c_str();
    }
    std::map<std::string, std::string> strings;
    String getString(const char *key) { return strings[key]; }
    float getFloat(const char *) const { return 15; }
    void putString(const char *key, String value) { strings[key] = value.c_str(); }
};
