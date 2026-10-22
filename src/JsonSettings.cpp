#include "JsonSettings.h"

#include <ArduinoJson.h>
#include <sstream>

// Opens the shared handle. The first open creates the namespace (read-write), so read-only
// opens on a factory-fresh device don't log "nvs_open failed: NOT_FOUND" (#54 item 4).
void JsonSettings::openLocked(bool readOnly) {
    if (! namespaceReady) {
        preferences.begin(name, false);
        preferences.end();
        namespaceReady = true;
    }
    preferences.begin(name, readOnly);
}

// Unset keys use their defaults without asking Preferences, which logs an error-level
// "nvs_get_* len fail: <key> NOT_FOUND" for every absent key (#84). Real failures on a
// key that exists (invalid handle, corruption) still surface from the get call.
String JsonSettings::readStringLocked(const char *key, const String &fallback) {
    return preferences.isKey(key) ? preferences.getString(key, fallback) : fallback;
}

int JsonSettings::readIntLocked(const char *key, int fallback) {
    return preferences.isKey(key) ? preferences.getInt(key, fallback) : fallback;
}

float JsonSettings::readFloatLocked(const char *key, float fallback) {
    return preferences.isKey(key) ? preferences.getFloat(key, fallback) : fallback;
}

String JsonSettings::getString(const char *key) {
    Guard guard(lock);
    JsonSetting *setting = lookup(key);
    if (! setting) return "";
    openLocked(true);
    String value = readStringLocked(key, setting->strDefault);
    preferences.end();
    return value;
}

int JsonSettings::getInt(const char *key) {
    Guard guard(lock);
    JsonSetting *setting = lookup(key);
    if (! setting) return 0;
    openLocked(true);
    int value = readIntLocked(key, setting->intDefault);
    preferences.end();
    return value;
}

float JsonSettings::getFloat(const char *key) {
    Guard guard(lock);
    JsonSetting *setting = lookup(key);
    if (! setting) return 0;
    openLocked(true);
    float value = readFloatLocked(key, setting->floatDefault);
    preferences.end();
    return value;
}

std::vector<int> JsonSettings::getIntVector(const char *key) {
    String value = getString(key);

    std::vector<int> intVector;
    std::istringstream stream(value.c_str());
    std::string token;
    while (std::getline(stream, token, ',')) {
        try {
            intVector.push_back(std::stoi(token));
        } catch (const std::invalid_argument &) {
            throw std::runtime_error("Invalid CSV: Non-integer value found");
        } catch (const std::out_of_range &) {
            throw std::runtime_error("Invalid CSV: Integer value out of range");
        }
    }
    return intVector;
}

void JsonSettings::putString(const char *key, String value) {
    Guard guard(lock);
    openLocked(false);
    preferences.putString(key, value);
    preferences.end();
}

void JsonSettings::putInt(const char *key, int value) {
    Guard guard(lock);
    openLocked(false);
    preferences.putInt(key, value);
    preferences.end();
}

void JsonSettings::putFloat(const char *key, float value) {
    Guard guard(lock);
    openLocked(false);
    preferences.putFloat(key, value);
    preferences.end();
}

void JsonSettings::putIntVector(const char *key, std::vector<int> value) {
    std::ostringstream stream;
    for (size_t i = 0; i < value.size(); ++i) {
        stream << value[i];
        if (i < value.size() - 1) {
            stream << ",";
        }
    }
    putString(key, stream.str().c_str());
}

JsonDocument JsonSettings::toJson() {
    Guard guard(lock);
    return toJsonLocked();
}

JsonDocument JsonSettings::toJsonLocked() {
    JsonDocument settings;

    openLocked(true);

    for (const auto &pair : map) {
        const String &key = pair.first;
        const JsonSetting &setting = pair.second;

        switch (setting.type) {
            case JsonSettingType::JST_STR:
            case JsonSettingType::JST_INT_VECTOR:
                settings[key] = readStringLocked(key.c_str(), setting.strDefault);
                break;
            case JsonSettingType::JST_INT: settings[key] = readIntLocked(key.c_str(), setting.intDefault); break;
            case JsonSettingType::JST_FLOAT: settings[key] = readFloatLocked(key.c_str(), setting.floatDefault); break;
        }
    }

    preferences.end();
    return settings;
}

bool JsonSettings::fromJson(JsonDocument settings) {
    Guard guard(lock);
    return fromJsonLocked(settings);
}

// All-or-nothing: every known key is validated before anything is written, and keys this
// build doesn't have (another firmware version's settings page, the other bus layout) are
// skipped instead of crashing the web server task (#54 item 2).
bool JsonSettings::fromJsonLocked(JsonDocument settings) {
    if (! validateCrossFields(settings)) {
        return false;
    }

    for (JsonPair kv : settings.as<JsonObject>()) {
        const char *key = kv.key().c_str();
        JsonSetting *setting = lookup(key);
        if (setting && ! setting->validate(kv.value().as<String>())) {
            lastValidationError = setting->getLastValidationError();
            lastValidationKey = String(key);
            return false;
        }
    }

    openLocked(false);

    for (JsonPair kv : settings.as<JsonObject>()) {
        const char *key = kv.key().c_str();
        JsonSetting *setting = lookup(key);
        if (! setting) continue;

        switch (setting->type) {
            case JsonSettingType::JST_INT_VECTOR:
            case JsonSettingType::JST_STR: preferences.putString(key, kv.value().as<String>()); break;
            case JsonSettingType::JST_INT: preferences.putInt(key, kv.value().as<int>()); break;
            case JsonSettingType::JST_FLOAT: preferences.putFloat(key, kv.value().as<float>()); break;
        }
    }

    preferences.end();

    return true;
}

bool JsonSettings::reset() {
    Guard guard(lock);
    openLocked(false);
    preferences.clear();
    preferences.end();

    return fromJsonLocked(toJsonLocked());
}

// The setting for `key`, or nullptr (logged) when this build has no such key.
JsonSetting *JsonSettings::lookup(const char *key) {
    auto it = this->map.find(key);
    if (it == this->map.end()) {
        Serial.printf("[WARN] Ignoring unknown setting: %s\n", key);
        return nullptr;
    }
    return &it->second;
}

bool JsonSettings::validateCrossFields(const JsonDocument &doc) {
    auto csvLen = [](const String &s) -> int {
        if (s.isEmpty()) return 0;
        int count = 1;
        for (size_t i = 0; i < s.length(); i++) {
            if (s[i] == ',') count++;
        }
        return count;
    };

    auto parseAddresses = [](const String &csv, int out[], int &count) -> bool {
        count = 0;
        std::istringstream stream(csv.c_str());
        std::string token;
        while (std::getline(stream, token, ',')) {
            try {
                out[count++] = std::stoi(token);
            } catch (...) {
                return false;
            }
        }
        return true;
    };

    auto validateAddressList = [&](const String &csv, const char *key) -> bool {
        int addrs[16];
        int count = 0;
        if (! parseAddresses(csv, addrs, count)) {
            lastValidationError = "Invalid address value in " + String(key);
            lastValidationKey = key;
            return false;
        }
        for (int i = 0; i < count; i++) {
            if (addrs[i] < 0x20 || addrs[i] > 0x27) {
                lastValidationError = "Address " + String(addrs[i]) + " is not a valid module address (must be 32-39)";
                lastValidationKey = key;
                return false;
            }
            for (int j = i + 1; j < count; j++) {
                if (addrs[i] == addrs[j]) {
                    lastValidationError = "Duplicate address " + String(addrs[i]) + " on the same bus";
                    lastValidationKey = key;
                    return false;
                }
            }
        }
        return true;
    };

    bool hasWireAddrs = doc["wireAddresses"].is<String>();
    bool hasWireOffsets = doc["wireOffsets"].is<String>();
    bool hasWire1Addrs = doc["wire1Addresses"].is<String>();
    bool hasWire1Offsets = doc["wire1Offsets"].is<String>();

    // Bus 1: address and offset arrays must be the same length
    if (hasWireAddrs && hasWireOffsets) {
        int addrLen = csvLen(doc["wireAddresses"].as<String>());
        int offLen = csvLen(doc["wireOffsets"].as<String>());
        if (addrLen != offLen) {
            lastValidationError =
                "Bus 1 address count (" + String(addrLen) + ") must match offset count (" + String(offLen) + ")";
            lastValidationKey = "wireAddresses";
            return false;
        }
    }

    // Bus 2: address and offset arrays must be the same length
    if (hasWire1Addrs && hasWire1Offsets) {
        int addrLen = csvLen(doc["wire1Addresses"].as<String>());
        int offLen = csvLen(doc["wire1Offsets"].as<String>());
        if (addrLen != offLen) {
            lastValidationError =
                "Bus 2 address count (" + String(addrLen) + ") must match offset count (" + String(offLen) + ")";
            lastValidationKey = "wire1Addresses";
            return false;
        }
    }

    // Total module count must not exceed MAX_MODULES
    if (hasWireAddrs || hasWire1Addrs) {
        int wireCount = hasWireAddrs ? csvLen(doc["wireAddresses"].as<String>()) : 0;
        int wire1Count = hasWire1Addrs ? csvLen(doc["wire1Addresses"].as<String>()) : 0;
#ifdef ENABLE_DUAL_I2C
        const int maxModules = 16;
#else
        const int maxModules = 8;
#endif
        if (wireCount + wire1Count > maxModules) {
            lastValidationError = "Total module count (" + String(wireCount + wire1Count) + ") exceeds maximum (" +
                String(maxModules) + ")";
            lastValidationKey = "wireAddresses";
            return false;
        }
    }

    // Address range and duplicate check per bus
    if (hasWireAddrs && ! validateAddressList(doc["wireAddresses"].as<String>(), "wireAddresses")) {
        return false;
    }
    if (hasWire1Addrs && ! validateAddressList(doc["wire1Addresses"].as<String>(), "wire1Addresses")) {
        return false;
    }

    // SDA/SCL pin conflict checks
    int sda = doc["sdaPin"].isNull() ? -1 : doc["sdaPin"].as<int>();
    int scl = doc["sclPin"].isNull() ? -1 : doc["sclPin"].as<int>();
    int sda2 = doc["sda2Pin"].isNull() ? -1 : doc["sda2Pin"].as<int>();
    int scl2 = doc["scl2Pin"].isNull() ? -1 : doc["scl2Pin"].as<int>();

    if (sda != -1 && scl != -1 && sda == scl) {
        lastValidationError = "Bus 1 SDA and SCL must be different pins";
        lastValidationKey = "sdaPin";
        return false;
    }
    if (sda2 != -1 && scl2 != -1 && sda2 == scl2) {
        lastValidationError = "Bus 2 SDA2 and SCL2 must be different pins";
        lastValidationKey = "sda2Pin";
        return false;
    }
    if (sda != -1 && sda2 != -1) {
        int bus1[2] = {sda, scl};
        int bus2[2] = {sda2, scl2};
        for (int a : bus1) {
            for (int b : bus2) {
                if (a != -1 && b != -1 && a == b) {
                    lastValidationError = "GPIO " + String(a) + " is shared between Bus 1 and Bus 2";
                    lastValidationKey = "sda2Pin";
                    return false;
                }
            }
        }
    }

    return true;
}
