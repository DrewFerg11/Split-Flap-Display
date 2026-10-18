#pragma once

#include <Arduino.h>

namespace ImprovTypes {
enum Error {
    ERROR_NONE
};
enum ChipFamily {
    CF_ESP32,
    CF_ESP32_C3,
    CF_ESP32_S3
};
} // namespace ImprovTypes

class ImprovWiFi {
  public:
    explicit ImprovWiFi(SerialFake *) {}
    void setDeviceInfo(ImprovTypes::ChipFamily, const char *, const char *, const char *) {}
    void onImprovError(void (*)(ImprovTypes::Error)) {}
    void onImprovConnected(void (*)(const char *, const char *)) {}
    void setCustomConnectWiFi(bool (*)(const char *, const char *)) {}
    void handleSerial() {}
};
