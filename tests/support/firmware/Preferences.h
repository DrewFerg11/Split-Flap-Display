#pragma once
#include <Arduino.h>
#include <freertos/semphr.h>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>
class Preferences {
  public:
    using Value = std::variant<std::string, int, float>;
    inline static std::map<std::string, std::map<std::string, Value>> storage;
    // absentReads counts gets of keys that don't exist: the real library logs an error for those.
    inline static int writes = 0, openSessions = 0, unlockedOpens = 0, absentReads = 0;
    inline static std::vector<std::pair<std::string, bool>> opens; // namespace, readOnly
    bool begin(const char *name, bool readOnly = false) {
        ns = name;
        ++openSessions;
        if (fakeMutexesHeld == 0) ++unlockedOpens;
        opens.emplace_back(name, readOnly);
        return true;
    }
    void end() { --openSessions; }
    bool isKey(const char *key) { return storage[ns].count(key) > 0; }
    String getString(const char *key, String fallback = "") {
        if (! isKey(key)) ++absentReads;
        auto &m = storage[ns];
        auto i = m.find(key);
        return i != m.end() && std::holds_alternative<std::string>(i->second) ? String(std::get<std::string>(i->second))
                                                                              : fallback;
    }
    int getInt(const char *key, int fallback = 0) {
        if (! isKey(key)) ++absentReads;
        auto &m = storage[ns];
        auto i = m.find(key);
        return i != m.end() && std::holds_alternative<int>(i->second) ? std::get<int>(i->second) : fallback;
    }
    float getFloat(const char *key, float fallback = 0) {
        if (! isKey(key)) ++absentReads;
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
