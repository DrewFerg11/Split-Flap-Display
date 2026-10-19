#pragma once
#include <Arduino.h>
#include <map>
#include <variant>
class Preferences {
  public:
    using Value = std::variant<std::string, int, float>;
    inline static std::map<std::string, std::map<std::string, Value>> storage;
    inline static int writes = 0, openSessions = 0;
    bool begin(const char *name, bool = false) {
        ns = name;
        ++openSessions;
        return true;
    }
    void end() { --openSessions; }
    String getString(const char *key, String fallback = "") {
        auto &m = storage[ns];
        auto i = m.find(key);
        return i != m.end() && std::holds_alternative<std::string>(i->second) ? String(std::get<std::string>(i->second))
                                                                              : fallback;
    }
    int getInt(const char *key, int fallback = 0) {
        auto &m = storage[ns];
        auto i = m.find(key);
        return i != m.end() && std::holds_alternative<int>(i->second) ? std::get<int>(i->second) : fallback;
    }
    float getFloat(const char *key, float fallback = 0) {
        auto &m = storage[ns];
        auto i = m.find(key);
        return i != m.end() && std::holds_alternative<float>(i->second) ? std::get<float>(i->second) : fallback;
    }
    size_t putString(const char *key, String v) {
        storage[ns][key] = std::string(v.c_str());
        ++writes;
        return v.length();
    }
    size_t putInt(const char *key, int v) {
        storage[ns][key] = v;
        ++writes;
        return sizeof(v);
    }
    size_t putFloat(const char *key, float v) {
        storage[ns][key] = v;
        ++writes;
        return sizeof(v);
    }
    bool clear() {
        storage[ns].clear();
        ++writes;
        return true;
    }

  private:
    std::string ns;
};
