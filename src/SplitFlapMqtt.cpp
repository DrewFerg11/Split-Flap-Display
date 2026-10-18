#include "SplitFlapMqtt.h"

#include "Version.h"

#include <WiFi.h>

SplitFlapMqtt::SplitFlapMqtt(JsonSettings &settings, WiFiClient &wifiClient)
    : settings(settings), wifiClient(wifiClient), mqttClient(wifiClient), display(nullptr) {}

void SplitFlapMqtt::setup() {
    // Settings can change the broker, credentials or topics. End the old
    // session first, including its retained availability, before replacing them.
    if (mqttClient.connected()) {
        mqttClient.publish(topic_avail.c_str(), "offline", true);
        mqttClient.disconnect();
    }

    mqttServer = settings.getString("mqtt_server");
    mqttPort = settings.getInt("mqtt_port");
    mqttUser = settings.getString("mqtt_user");
    mqttPass = settings.getString("mqtt_pass");

    String mdns = settings.getString("mdns");
    String name = settings.getString("name");

    topic_command = "splitflap/" + mdns + "/set";
    topic_state = "splitflap/" + mdns + "/state";
    topic_avail = "splitflap/" + mdns + "/availability";
    topic_config_text = "homeassistant/text/splitflap_text_" + mdns + "/config";
    topic_config_sensor = "homeassistant/sensor/splitflap_sensor_" + mdns + "/config";

    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length) {
        String message;
        for (unsigned int i = 0; i < length; i++) {
            message += (char) payload[i];
        }
        Serial.printf("[MQTT] Message received: %s\n", message.c_str());
        if (display) {
            float maxVel = settings.getFloat("maxVel");
#ifdef ENABLE_DUAL_I2C
            int sep = message.indexOf('|');
            if (sep >= 0) {
                display->writeStringDual(message.substring(0, sep), message.substring(sep + 1), maxVel, false);
            } else {
                display->writeString(message, maxVel, false);
            }
#else
            display->writeString(message, maxVel, false);
#endif
        }
    });

    connectToMqtt();
}

void SplitFlapMqtt::connectToMqtt() {
    if (mqttServer.isEmpty() || WiFi.status() != WL_CONNECTED) return;

    if (! mqttClient.connected()) {
        Serial.println("[MQTT] Attempting to connect...");
        String mdns = settings.getString("mdns");
        String name = settings.getString("name");

        if (mqttUser.length() > 0) {
            mqttClient.connect(
                mdns.c_str(), mqttUser.c_str(), mqttPass.c_str(), topic_avail.c_str(), 0, true, "offline"
            );
        } else {
            mqttClient.connect(mdns.c_str(), topic_avail.c_str(), 0, true, "offline");
        }

        if (mqttClient.connected()) {
            Serial.println("[MQTT] Connected to broker");

            // clang-format off
            int numModules = display ? display->getNumModules() : 16;
            int wire1Count = display ? display->getWire1Count() : 0;
            int maxLen = wire1Count > 0 ? numModules + 1 : numModules;
            String payload_text = "{"
                "\"name\":\"Display\","
                "\"unique_id\":\"text_" + mdns + "\","
                "\"command_topic\":\"" + topic_command + "\","
                "\"availability_topic\":\"" + topic_avail + "\","
                "\"max\":" + String(maxLen) + ","
                "\"device\":{"
                    "\"identifiers\":[\"splitflap_" + mdns + "\"],"
                    "\"name\":\"" + name + "\","
                    "\"manufacturer\":\"SplitFlap\","
                    "\"model\":\"SplitFlap Display\","
                    "\"sw_version\":\"" FIRMWARE_VERSION "\""
                "}"
            "}";

            String payload_sensor = "{"
                "\"name\":\"Currently Displayed\","
                "\"unique_id\":\"sensor_" + mdns + "\","
                "\"state_topic\":\"" + topic_state + "\","
                "\"availability_topic\":\"" + topic_avail + "\","
                "\"entity_category\":\"diagnostic\","
                "\"device\":{"
                    "\"identifiers\":[\"splitflap_" + mdns + "\"],"
                    "\"name\":\"" + name + "\","
                    "\"manufacturer\":\"SplitFlap\","
                    "\"model\":\"SplitFlap Display\","
                    "\"sw_version\":\"" FIRMWARE_VERSION "\""
                "}"
            "}";
            // clang-format on

            mqttClient.subscribe(topic_command.c_str());
            mqttClient.publish(topic_avail.c_str(), "online", true);
            mqttClient.publish(topic_state.c_str(), "", true);

            mqttClient.publish(topic_config_text.c_str(), payload_text.c_str(), true);
            mqttClient.publish(topic_config_sensor.c_str(), payload_sensor.c_str(), true);
        } else {
            Serial.println("[MQTT] Failed to connect");
        }
        // An attempt may block on a socket timeout. Space retries from its end.
        lastAttempt = millis();
    }
}

void SplitFlapMqtt::setDisplay(SplitFlapDisplay *d) {
    display = d;
}

void SplitFlapMqtt::publishState(const String &message) {
    Serial.println("[MQTT] Publishing state: " + message);
    mqttClient.publish(topic_state.c_str(), message.c_str(), true);
}

void SplitFlapMqtt::loop() {
    mqttClient.loop();
    const uint32_t now = millis();
    if (! mqttClient.connected() && static_cast<uint32_t>(now - lastAttempt) >= reconnectInterval) {
        connectToMqtt();
    }
}

bool SplitFlapMqtt::isConnected() {
    return mqttClient.connected();
}
