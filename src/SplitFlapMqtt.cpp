#include "SplitFlapMqtt.h"
#include "DisplayCommand.h"
#include <ArduinoJson.h>

SplitFlapMqtt::SplitFlapMqtt(JsonSettings &settings, WiFiClient &wifiClient)
    : settings(settings), wifiClient(wifiClient), mqttClient(wifiClient), display(nullptr) {}

void SplitFlapMqtt::setup() {
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
    topic_displays_update = "splitflap/" + mdns + "/displays/set";

    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length) {
        String message;
        for (unsigned int i = 0; i < length; i++) {
            message += (char) payload[i];
        }
        Serial.printf("[MQTT] Message received on topic %s: %s\n", topic, message.c_str());
        
        String topicStr = String(topic);
        
        // Handle dual-display updates
        if (topicStr == topic_displays_update) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, message);
            
            if (error) {
                Serial.printf("[MQTT] JSON parse error: %s\n", error.c_str());
                return;
            }
            
            // Process each display update in the array
            JsonArray displays = doc["displays"].as<JsonArray>();
            for (JsonObject displayUpdate : displays) {
                int displayNum = displayUpdate["num"].as<int>();
                int mode = displayUpdate["mode"].as<int>();
                
                Serial.printf("[MQTT] Display %d: mode=%d\n", displayNum, mode);
                
                // Update the mode setting
                String modeKey = (displayNum == 1) ? "d1_mode" : "d2_mode";
                settings.putInt(modeKey, mode);
                
                // If mode 6 (custom text) and text provided, queue the command
                if (mode == 6 && displayUpdate.containsKey("text")) {
                    String text = displayUpdate["text"].as<String>();
                    QueueHandle_t *targetQueue = (displayNum == 1) ? display1Queue_ptr : display2Queue_ptr;
                    
                    if (targetQueue && *targetQueue) {
                        DisplayCommand cmd;
                        cmd.text = text;
                        cmd.centerText = false;  // Can add this to payload if needed
                        
                        if (xQueueSend(*targetQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
                            Serial.printf("[MQTT] Queued text '%s' for display %d\n", text.c_str(), displayNum);
                        } else {
                            Serial.printf("[MQTT] Failed to queue command for display %d\n", displayNum);
                        }
                    } else {
                        Serial.printf("[MQTT] Queue not available for display %d\n", displayNum);
                    }
                }
            }
        }
        // Handle legacy single-display command
        else if (topicStr == topic_command) {
            if (display) {
                float maxVel = settings.getFloat("maxVel");
                display->writeString(message, maxVel, false);
            }
        }
    });

    connectToMqtt();
}

void SplitFlapMqtt::connectToMqtt() {
    if (! mqttClient.connected()) {
        Serial.println("[MQTT] Attempting to connect...");
        String mdns = settings.getString("mdns");
        String name = settings.getString("name");

        if (mqttUser.length() > 0) {
            mqttClient.connect(mdns.c_str(), mqttUser.c_str(), mqttPass.c_str());
        } else {
            mqttClient.connect(mdns.c_str());
        }

        if (mqttClient.connected()) {
            Serial.println("[MQTT] Connected to broker");

            // clang-format off
            String payload_text = "{"
                "\"name\":\"Display\","
                "\"unique_id\":\"text_" + mdns + "\","
                "\"command_topic\":\"" + topic_command + "\","
                "\"availability_topic\":\"" + topic_avail + "\","
                "\"device\":{"
                    "\"identifiers\":[\"splitflap_" + mdns + "\"],"
                    "\"name\":\"" + name + "\","
                    "\"manufacturer\":\"SplitFlap\","
                    "\"model\":\"SplitFlap Display\","
                    "\"sw_version\":\"1.0.0\""
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
                    "\"sw_version\":\"1.0.0\""
                "}"
            "}";
            // clang-format on

            mqttClient.subscribe(topic_command.c_str());
            mqttClient.subscribe(topic_displays_update.c_str());
            mqttClient.publish(topic_avail.c_str(), "online", true);
            mqttClient.publish(topic_state.c_str(), "", true);

            mqttClient.publish(topic_config_text.c_str(), payload_text.c_str(), true);
            mqttClient.publish(topic_config_sensor.c_str(), payload_sensor.c_str(), true);
        } else {
            Serial.println("[MQTT] Failed to connect");
        }
    }
}

void SplitFlapMqtt::setDisplay(SplitFlapDisplay *d) {
    display = d;
}

void SplitFlapMqtt::setDisplayQueues(QueueHandle_t *q1, QueueHandle_t *q2) {
    display1Queue_ptr = q1;
    display2Queue_ptr = q2;
}

void SplitFlapMqtt::publishState(const String &message) {
    Serial.println("[MQTT] Publishing state: " + message);
    mqttClient.publish(topic_state.c_str(), message.c_str(), true);
}

void SplitFlapMqtt::loop() {
    mqttClient.loop();
}

bool SplitFlapMqtt::isConnected() {
    return mqttClient.connected();
}
