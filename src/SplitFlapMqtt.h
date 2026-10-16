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
    bool isConnected();

  private:
    PubSubClient mqttClient; // PubSubClient instead of AsyncMqttClient
    WiFiClient &wifiClient;  // store reference to WiFiClient

    JsonSettings &settings;
    SplitFlapDisplay *display;

    void connectToMqtt();
    void handleMessage(const String &topic, const String &message);
    void publishDiscovery();
    void publishTelemetry();
    String deviceJson();

    // MQTT config
    String mqttServer;
    int mqttPort = 1883;
    String mqttUser;
    String mqttPass;
    String mdns;

    String topic_command;
    String topic_state;
    String topic_avail;
    String topic_cmd_restart;
    String topic_cmd_home;
    String topic_cmd_mode;
    String topic_state_mode;
    String topic_state_rssi;
    String topic_state_ip;

    // Set by the restart button handler; acted on from loop() so the
    // availability update can go out before the reboot.
    bool restartPending = false;

    int lastPublishedMode = -1;
    unsigned long lastAttempt = 0;
    unsigned long lastTelemetry = 0;
};
