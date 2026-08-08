#include "SplitFlapMqtt.h"

#include "Version.h"

#include <WiFi.h>

// Discovery payloads are ~400-600 bytes; PubSubClient's default buffer (256)
// silently drops anything larger.
#define MQTT_BUFFER_SIZE 1024

// Retry/telemetry cadence
static const unsigned long MQTT_RETRY_MS = 5000;
static const unsigned long TELEMETRY_MS = 60000;

// HA publishes "online" here on startup; discovery must be re-sent in response.
static const char *HA_STATUS_TOPIC = "homeassistant/status";

// Modes exposed to HA's select entity. Only self-contained modes are listed;
// the others (multi-word, tests) need extra input that MQTT doesn't carry.
struct ModeOption
{
    const char *name;
    int mode;
};
static const ModeOption MODE_OPTIONS[] = {
    {"Off", 4},
    {"Date", 2},
    {"Time", 3},
#ifdef ENABLE_DUAL_I2C
    {"Date & Time", 10},
#endif
};

static const char *modeToName(int mode) {
    for (auto &m : MODE_OPTIONS) {
        if (m.mode == mode) return m.name;
    }
    return nullptr;
}

SplitFlapMqtt::SplitFlapMqtt(JsonSettings &settings, WiFiClient &wifiClient)
    : settings(settings), wifiClient(wifiClient), mqttClient(wifiClient), display(nullptr) {}

void SplitFlapMqtt::setup() {
    mqttServer = settings.getString("mqtt_server");
    mqttPort = settings.getInt("mqtt_port");
    mqttUser = settings.getString("mqtt_user");
    mqttPass = settings.getString("mqtt_pass");
    mdns = settings.getString("mdns");

    String base = "splitflap/" + mdns;
    topic_command = base + "/set";
    topic_state = base + "/state";
    topic_avail = base + "/availability";
    topic_cmd_restart = base + "/restart/set";
    topic_cmd_home = base + "/home/set";
    topic_cmd_mode = base + "/mode/set";
    topic_state_mode = base + "/mode/state";
    topic_state_rssi = base + "/rssi/state";
    topic_state_ip = base + "/ip/state";

    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback([this](char *topic, byte *payload, unsigned int length) {
        String message;
        message.reserve(length);
        for (unsigned int i = 0; i < length; i++) {
            message += (char) payload[i];
        }
        handleMessage(String(topic), message);
    });

    if (mqttServer.length() > 0) {
        connectToMqtt();
    }
}

void SplitFlapMqtt::handleMessage(const String &topic, const String &message) {
    Serial.printf("[MQTT] Message received on %s: %s\n", topic.c_str(), message.c_str());

    if (topic == topic_command) {
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
    } else if (topic == topic_cmd_restart) {
        restartPending = true; // acted on from loop() so availability can flush first
    } else if (topic == topic_cmd_home) {
        if (display) {
            display->homeToString("", settings.getFloat("maxVel"));
        }
    } else if (topic == topic_cmd_mode) {
        bool found = false;
        for (auto &m : MODE_OPTIONS) {
            if (message == m.name) {
                settings.putInt("mode", m.mode);
                found = true;
                break;
            }
        }
        if (! found) {
            Serial.printf("[MQTT] Unknown mode: %s\n", message.c_str());
        }
    } else if (topic == HA_STATUS_TOPIC) {
        if (message == "online") {
            // HA restarted: re-announce everything
            publishDiscovery();
            mqttClient.publish(topic_avail.c_str(), "online", true);
            lastPublishedMode = -1; // force mode state republish from loop()
            publishTelemetry();
        }
    }
}

void SplitFlapMqtt::connectToMqtt() {
    if (! mqttClient.connected()) {
        Serial.println("[MQTT] Attempting to connect...");

        // Last will marks the device unavailable if it drops off ungracefully
        bool connected;
        if (mqttUser.length() > 0) {
            connected = mqttClient.connect(
                mdns.c_str(), mqttUser.c_str(), mqttPass.c_str(), topic_avail.c_str(), 0, true, "offline"
            );
        } else {
            connected = mqttClient.connect(mdns.c_str(), topic_avail.c_str(), 0, true, "offline");
        }

        if (connected) {
            Serial.println("[MQTT] Connected to broker");

            mqttClient.subscribe(topic_command.c_str());
            mqttClient.subscribe(topic_cmd_restart.c_str());
            mqttClient.subscribe(topic_cmd_home.c_str());
            mqttClient.subscribe(topic_cmd_mode.c_str());
            mqttClient.subscribe(HA_STATUS_TOPIC);

            mqttClient.publish(topic_avail.c_str(), "online", true);
            lastPublishedMode = -1; // force mode state republish from loop()

            publishDiscovery();
            publishTelemetry();
        } else {
            Serial.println("[MQTT] Failed to connect");
        }
    }
}

// Shared "device" block that groups all entities under one HA device
String SplitFlapMqtt::deviceJson() {
    String name = settings.getString("name");
    // clang-format off
    return String("\"device\":{"
        "\"identifiers\":[\"splitflap_" + mdns + "\"],"
        "\"name\":\"" + name + "\","
        "\"manufacturer\":\"SplitFlap\","
        "\"model\":\"SplitFlap Display\","
        "\"sw_version\":\"" FIRMWARE_VERSION "\""
    "}");
    // clang-format on
}

void SplitFlapMqtt::publishDiscovery() {
    String device = deviceJson();
    String avail = "\"availability_topic\":\"" + topic_avail + "\",";

    int numModules = display ? display->getNumModules() : 16;
    int wire1Count = display ? display->getWire1Count() : 0;
    int maxLen = wire1Count > 0 ? numModules + 1 : numModules;

    // clang-format off
    String payload_text = "{"
        "\"name\":\"Display\","
        "\"unique_id\":\"text_" + mdns + "\","
        "\"command_topic\":\"" + topic_command + "\","
        + avail +
        "\"max\":" + String(maxLen) + ","
        + device +
    "}";

    String payload_sensor = "{"
        "\"name\":\"Currently Displayed\","
        "\"unique_id\":\"sensor_" + mdns + "\","
        "\"state_topic\":\"" + topic_state + "\","
        + avail +
        "\"entity_category\":\"diagnostic\","
        + device +
    "}";

    String payload_restart = "{"
        "\"name\":\"Restart\","
        "\"unique_id\":\"restart_" + mdns + "\","
        "\"command_topic\":\"" + topic_cmd_restart + "\","
        + avail +
        "\"device_class\":\"restart\","
        "\"entity_category\":\"config\","
        + device +
    "}";

    String payload_home = "{"
        "\"name\":\"Re-Home\","
        "\"unique_id\":\"home_" + mdns + "\","
        "\"command_topic\":\"" + topic_cmd_home + "\","
        + avail +
        "\"icon\":\"mdi:home-search\","
        "\"entity_category\":\"config\","
        + device +
    "}";

    String modeOptions = "";
    for (auto &m : MODE_OPTIONS) {
        if (modeOptions.length() > 0) modeOptions += ",";
        modeOptions += "\"" + String(m.name) + "\"";
    }

    String payload_mode = "{"
        "\"name\":\"Mode\","
        "\"unique_id\":\"mode_" + mdns + "\","
        "\"command_topic\":\"" + topic_cmd_mode + "\","
        "\"state_topic\":\"" + topic_state_mode + "\","
        + avail +
        "\"options\":[" + modeOptions + "],"
        "\"icon\":\"mdi:animation-play-outline\","
        + device +
    "}";

    String payload_rssi = "{"
        "\"name\":\"WiFi Signal\","
        "\"unique_id\":\"rssi_" + mdns + "\","
        "\"state_topic\":\"" + topic_state_rssi + "\","
        + avail +
        "\"device_class\":\"signal_strength\","
        "\"unit_of_measurement\":\"dBm\","
        "\"state_class\":\"measurement\","
        "\"entity_category\":\"diagnostic\","
        + device +
    "}";

    String payload_ip = "{"
        "\"name\":\"IP Address\","
        "\"unique_id\":\"ip_" + mdns + "\","
        "\"state_topic\":\"" + topic_state_ip + "\","
        + avail +
        "\"icon\":\"mdi:ip-network\","
        "\"entity_category\":\"diagnostic\","
        + device +
    "}";
    // clang-format on

    struct
    {
        const char *component;
        const char *id;
        String *payload;
    } configs[] = {
        {"text", "text", &payload_text},
        {"sensor", "sensor", &payload_sensor},
        {"button", "restart", &payload_restart},
        {"button", "home", &payload_home},
        {"select", "mode", &payload_mode},
        {"sensor", "rssi", &payload_rssi},
        {"sensor", "ip", &payload_ip},
    };

    for (auto &c : configs) {
        String topic = "homeassistant/" + String(c.component) + "/splitflap_" + String(c.id) + "_" + mdns + "/config";
        if (! mqttClient.publish(topic.c_str(), c.payload->c_str(), true)) {
            Serial.printf("[MQTT] Failed to publish discovery config: %s\n", topic.c_str());
        }
    }
}

void SplitFlapMqtt::publishTelemetry() {
    mqttClient.publish(topic_state_rssi.c_str(), String(WiFi.RSSI()).c_str(), true);
    mqttClient.publish(topic_state_ip.c_str(), WiFi.localIP().toString().c_str(), true);
}

void SplitFlapMqtt::setDisplay(SplitFlapDisplay *d) {
    display = d;
}

void SplitFlapMqtt::publishState(const String &message) {
    Serial.println("[MQTT] Publishing state: " + message);
    mqttClient.publish(topic_state.c_str(), message.c_str(), true);
}

void SplitFlapMqtt::loop() {
    if (mqttServer.length() == 0) return;

    if (! mqttClient.connected()) {
        if (millis() - lastAttempt > MQTT_RETRY_MS) {
            lastAttempt = millis();
            connectToMqtt();
        }
        return;
    }

    mqttClient.loop();

    if (restartPending) {
        restartPending = false;
        Serial.println("[MQTT] Restart requested, rebooting...");
        mqttClient.publish(topic_avail.c_str(), "offline", true);
        mqttClient.disconnect();
        delay(100);
        ESP.restart();
    }

    // Reflect mode changes (from HA or the web UI) into the select's state
    int mode = settings.getInt("mode");
    if (mode != lastPublishedMode) {
        lastPublishedMode = mode;
        const char *name = modeToName(mode);
        // Modes not offered in the select (multi-word, tests) publish an empty
        // state, which HA shows as "unknown" rather than a wrong option.
        mqttClient.publish(topic_state_mode.c_str(), name ? name : "", true);
    }

    if (millis() - lastTelemetry > TELEMETRY_MS) {
        lastTelemetry = millis();
        publishTelemetry();
    }
}

bool SplitFlapMqtt::isConnected() {
    return mqttClient.connected();
}
