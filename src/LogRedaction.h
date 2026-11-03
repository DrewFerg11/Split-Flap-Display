#pragma once

#include <ArduinoJson.h>

// Only log a redacted copy of settings; never modify the request being saved.
inline void redactSecrets(JsonDocument &document) {
    for (const char *key : {"password", "mqtt_pass", "otaPass"}) {
        if (document.as<JsonObject>().containsKey(key)) {
            document[key] = "***";
        }
    }
}
