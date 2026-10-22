#pragma once

#include "JsonSetting.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <freertos/semphr.h>
#include <map>

class JsonSettings {
  public:
    JsonSettings(const char *name, std::map<String, JsonSetting> map)
        : name(name), map(map), lock(xSemaphoreCreateMutex()) {}

    String getString(const char *key);
    int getInt(const char *key);
    float getFloat(const char *key);
    std::vector<int> getIntVector(const char *key);

    void putString(const char *key, String value);
    void putInt(const char *key, int value);
    void putFloat(const char *key, float value);
    void putIntVector(const char *key, std::vector<int> value);

    JsonDocument toJson();
    bool fromJson(JsonDocument settings);
    bool reset();

    String getLastValidationError() { return lastValidationError; }
    String getLastValidationKey() { return lastValidationKey; }

  private:
    const char *name;
    std::map<String, JsonSetting> map;

    String lastValidationError;
    String lastValidationKey;

    // The loop task and the web server's task both use these settings, and every access
    // opens and closes the one shared Preferences handle. Each public method holds `lock`
    // for its whole open/use/close, so one task can never close the handle under the
    // other (#54). Internal helpers ending in `Locked` expect the caller to hold it.
    class Guard {
      public:
        explicit Guard(SemaphoreHandle_t lock) : lock(lock) { xSemaphoreTake(lock, portMAX_DELAY); }
        ~Guard() { xSemaphoreGive(lock); }
        Guard(const Guard &) = delete;
        Guard &operator=(const Guard &) = delete;

      private:
        SemaphoreHandle_t lock;
    };

    JsonSetting *lookup(const char *key);
    void openLocked(bool readOnly);
    String readStringLocked(const char *key, const String &fallback);
    int readIntLocked(const char *key, int fallback);
    float readFloatLocked(const char *key, float fallback);
    JsonDocument toJsonLocked();
    bool fromJsonLocked(JsonDocument settings);
    bool validateCrossFields(const JsonDocument &doc);

    Preferences preferences;
    SemaphoreHandle_t lock;
    bool namespaceReady = false;
};
