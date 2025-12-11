#pragma once

#include "JsonSettings.h"
#include "SplitFlapDisplay.h"

#include <PubSubClient.h>
#include <WiFiClient.h>

class SplitFlapMqtt {
  public:
    SplitFlapMqtt(JsonSettings &settings, WiFiClient &client); // updated constructor

    void setup();
    void loop();                                               // needed for PubSubClient3
    void publishState(const String &message);
    void setDisplay(SplitFlapDisplay *display);
    void setDisplayQueues(QueueHandle_t *q1, QueueHandle_t *q2);
    bool isConnected();

  private:
    PubSubClient mqttClient; // PubSubClient instead of AsyncMqttClient
    WiFiClient &wifiClient;  // store reference to WiFiClient

    JsonSettings &settings;
    SplitFlapDisplay *display;
    QueueHandle_t *display1Queue_ptr = nullptr;
    QueueHandle_t *display2Queue_ptr = nullptr;

    void connectToMqtt();

    // MQTT config
    String mqttServer;
    int mqttPort = 1883;
    String mqttUser;
    String mqttPass;
    String topic_command;
    String topic_state;
    String topic_avail;
    String topic_config_text;
    String topic_config_sensor;
    String topic_displays_update;  // New topic for dual-display control

    unsigned long lastAttempt = 0;
    int retryCount = 0;
};
