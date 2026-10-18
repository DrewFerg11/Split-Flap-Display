#pragma once
#include <Arduino.h>
using ota_error_t = int;
constexpr int U_FLASH = 0, OTA_AUTH_ERROR = 1, OTA_BEGIN_ERROR = 2, OTA_CONNECT_ERROR = 3, OTA_RECEIVE_ERROR = 4,
              OTA_END_ERROR = 5;
struct OTAFake
{
    bool begun = false;
    int handles = 0;
    void setHostname(const char *) {}
    void setPassword(const char *) {}
    int getCommand() { return U_FLASH; }
    template <typename T> OTAFake &onStart(T) { return *this; }
    template <typename T> OTAFake &onEnd(T) { return *this; }
    template <typename T> OTAFake &onProgress(T) { return *this; }
    template <typename T> OTAFake &onError(T) { return *this; }
    void begin() { begun = true; }
    void handle() { ++handles; }
};
inline OTAFake ArduinoOTA;
