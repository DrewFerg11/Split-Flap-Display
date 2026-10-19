#pragma once
#include <JsonSettings.h>
inline JsonSettings configuration(const char *storageNamespace = "test-config") {
    std::map<String, JsonSetting> defaults = {
        {"name", JsonSetting(String("Default"))},
        {"mdns", JsonSetting(String("clock"))},
        {"ssid", JsonSetting(String(""))},
        {"password", JsonSetting(String(""))},
        {"mqtt_server", JsonSetting(String(""))},
        {"mqtt_port", JsonSetting(1883)},
        {"mqtt_user", JsonSetting(String(""))},
        {"mqtt_pass", JsonSetting(String(""))},
        {"otaPass", JsonSetting(String(""))},
        {"timezone", JsonSetting(String("UTC0"))},
        {"mode", JsonSetting(0)},
        {"charset", JsonSetting(37)},
        {"stepsPerRot", JsonSetting(96)},
        {"magnetPosition", JsonSetting(0)},
        {"displayOffset", JsonSetting(0)},
        {"maxVel", JsonSetting(15.0f)},
        {"wireAddresses", JsonSetting(std::vector<int>{32, 33, 34, 35})},
        {"wireOffsets", JsonSetting(std::vector<int>{0, 0, 0, 0})},
        {"sdaPin", JsonSetting(8)},
        {"sclPin", JsonSetting(9)}
    };
#ifdef ENABLE_DUAL_I2C
    defaults.emplace("wire1Addresses", JsonSetting(std::vector<int>{32, 33, 34}));
    defaults.emplace("wire1Offsets", JsonSetting(std::vector<int>{0, 0, 0}));
    defaults.emplace("sda2Pin", JsonSetting(10));
    defaults.emplace("scl2Pin", JsonSetting(11));
#endif
    return JsonSettings(storageNamespace, defaults);
}
