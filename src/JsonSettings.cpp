#include "JsonSettings.h"

#include <ArduinoJson.h>
#include <sstream>

String JsonSettings::getString(const char *key) {
    preferences.begin(name, true);
    String value = preferences.getString(key, this->find(key).strDefault);
    preferences.end();
    return value;
}

int JsonSettings::getInt(const char *key) {
    preferences.begin(name, true);
    int value = preferences.getInt(key, this->find(key).intDefault);
    preferences.end();
    return value;
}

float JsonSettings::getFloat(const char *key) {
    preferences.begin(name, true);
    float value = preferences.getFloat(key, this->find(key).floatDefault);
    preferences.end();
    return value;
}

std::vector<int> JsonSettings::getIntVector(const char *key) {
    preferences.begin(name, true);
    String value = preferences.getString(key, this->find(key).strDefault);
    preferences.end();

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
    preferences.begin(name, false);
    preferences.putString(key, value);
    preferences.end();
}

void JsonSettings::putInt(const char *key, int value) {
    preferences.begin(name, false);
    preferences.putInt(key, value);
    preferences.end();
}

void JsonSettings::putFloat(const char *key, float value) {
    preferences.begin(name, false);
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
    JsonDocument settings;

    preferences.begin(name, true);

    for (const auto &pair : map) {
        const String &key = pair.first;
        const JsonSetting &setting = pair.second;

        switch (setting.type) {
            case JsonSettingType::JST_STR:
            case JsonSettingType::JST_INT_VECTOR:
                settings[key] = preferences.getString(key.c_str(), setting.strDefault);
                break;
            case JsonSettingType::JST_INT: settings[key] = preferences.getInt(key.c_str(), setting.intDefault); break;
            case JsonSettingType::JST_FLOAT:
                settings[key] = preferences.getFloat(key.c_str(), setting.floatDefault);
                break;
        }
    }

    preferences.end();
    return settings;
}

bool JsonSettings::fromJson(JsonDocument settings) {
    if (! validateCrossFields(settings)) {
        return false;
    }

    preferences.begin(name, false);

    for (JsonPair kv : settings.as<JsonObject>()) {
        const char *key = kv.key().c_str();
        JsonSetting setting = this->find(key);

        if (! setting.validate(kv.value().as<String>())) {
            lastValidationError = setting.getLastValidationError();
            lastValidationKey = String(key);
            return false;
        }

        switch (setting.type) {
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
    preferences.begin("config", false);
    preferences.clear();
    preferences.end();

    return fromJson(toJson());
}

JsonSetting JsonSettings::find(const char *key) {
    auto it = this->map.find(key);
    if (it == this->map.end()) {
        throw std::runtime_error("Key not found in settings map");
    }
    return it->second;
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
