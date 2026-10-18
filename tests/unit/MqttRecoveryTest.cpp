#include <SplitFlapMqtt.h>
#include <WiFi.h>
#include <gtest/gtest.h>
#include <limits>
#include <tuple>

namespace {
constexpr const char *commandTopic = "splitflap/clock/set";
constexpr const char *availabilityTopic = "splitflap/clock/availability";

// Authenticated/anonymous sessions, with either a definite socket loss or a
// half-open connection detected by the real library's keepalive timeout.
class MqttSessionTest : public testing::Test {
  protected:
    JsonSettings settings;
    WiFiClient transport;
    SplitFlapDisplay display;
    SplitFlapMqtt mqtt{settings, transport};

    void SetUp() override {
        fakeMillis = 0;
        WiFi.online = true;
        mqtt.setDisplay(&display);
    }
    void tick(unsigned int milliseconds) {
        fakeMillis += milliseconds;
        mqtt.loop();
    }
};

class MqttRecoveryTest : public MqttSessionTest, public testing::WithParamInterface<std::tuple<bool, bool>> {
  protected:
    void SetUp() override {
        MqttSessionTest::SetUp();
        if (std::get<0>(GetParam())) {
            settings.strings["mqtt_user"] = "ha";
            settings.strings["mqtt_pass"] = "secret";
        }
    }
};

TEST_P(MqttRecoveryTest, LostSessionAutomaticallyResumesClockUpdates) {
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_EQ(transport.connections.size(), 1u);
    EXPECT_EQ(transport.connections.front().user, std::get<0>(GetParam()) ? "ha" : "");
    EXPECT_EQ(transport.connections.front().password, std::get<0>(GetParam()) ? "secret" : "");
    EXPECT_EQ(transport.connections.front().willTopic, availabilityTopic);
    EXPECT_EQ(transport.connections.front().willPayload, "offline");
    EXPECT_TRUE(transport.connections.front().willRetained);
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");
    ASSERT_TRUE(transport.deliver(commandTopic, "MON12:34"));
    mqtt.loop();
    ASSERT_EQ(display.lastMessage, "MON12:34");
    ASSERT_EQ(display.writeCalls, 1);

    transport.networkAvailable = false;
    if (std::get<1>(GetParam())) {
        transport.blackhole = true;
    } else {
        transport.stop();
    }
    // Long enough for two default keepalive periods, plus ample retry time.
    for (int second = 0; second < 60; ++second) tick(1000);
    ASSERT_FALSE(mqtt.isConnected());
    if (std::get<1>(GetParam())) EXPECT_GT(transport.pingRequests, 0);
    EXPECT_EQ(transport.retained.at(availabilityTopic), "offline");
    EXPECT_EQ(display.lastMessage, "MON12:34");
    EXPECT_EQ(display.writeCalls, 1);
    transport.networkAvailable = true;
    for (int second = 0; second < 60; ++second) tick(1000);

    // Same failure scenario as the characterization commit. No setup(), reset,
    // settings change or explicit reconnect is allowed to rescue the session.
    ASSERT_TRUE(mqtt.isConnected());
    EXPECT_GT(transport.connectAttempts, 1);
    ASSERT_EQ(transport.connections.size(), 2u);
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");
    EXPECT_EQ(transport.connections.back().user, std::get<0>(GetParam()) ? "ha" : "");
    EXPECT_EQ(transport.connections.back().password, std::get<0>(GetParam()) ? "secret" : "");
    EXPECT_EQ(transport.connections.back().willTopic, availabilityTopic);
    ASSERT_TRUE(transport.subscriptions.count(commandTopic));
    ASSERT_TRUE(transport.deliver(commandTopic, "MON12:36"));
    mqtt.loop();
    EXPECT_EQ(display.lastMessage, "MON12:36");
    EXPECT_EQ(display.writeCalls, 2);

    // A second outage must also recover; this is not a one-shot rescue.
    transport.stop();
    tick(5000);
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_TRUE(transport.deliver(commandTopic, "MON12:37"));
    mqtt.loop();
    EXPECT_EQ(display.lastMessage, "MON12:37");
    EXPECT_EQ(display.writeCalls, 3);
}

TEST_F(MqttSessionTest, BrokerUnavailableAtBootRetriesAndEventuallyReceivesCommands) {
    transport.networkAvailable = false;
    mqtt.setup();
    ASSERT_FALSE(mqtt.isConnected());
    ASSERT_EQ(transport.connectAttempts, 1);
    for (int second = 0; second < 60; ++second) tick(1000);
    EXPECT_EQ(transport.connectAttempts, 13);
    transport.networkAvailable = true;
    tick(5000);
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_TRUE(transport.deliver(commandTopic, "MON12:34"));
    mqtt.loop();
    EXPECT_EQ(display.lastMessage, "MON12:34");
}

TEST_F(MqttSessionTest, FailedAttemptsWaitFiveSecondsAfterBlockingConnectReturns) {
    transport.networkAvailable = false;
    transport.connectDelayMs = 20000;
    mqtt.setup();
    ASSERT_EQ(transport.connectAttempts, 1);
    tick(0);
    tick(4999);
    EXPECT_EQ(transport.connectAttempts, 1);
    tick(1);
    ASSERT_EQ(transport.connectAttempts, 2);
    for (int iteration = 0; iteration < 100; ++iteration) tick(0);
    EXPECT_EQ(transport.connectAttempts, 2);
    tick(4999);
    EXPECT_EQ(transport.connectAttempts, 2);
    transport.networkAvailable = true;
    transport.connectDelayMs = 0;
    tick(1);
    EXPECT_TRUE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 3);
}

TEST_F(MqttSessionTest, RetryIntervalSurvivesMillisWrapping) {
    fakeMillis = std::numeric_limits<uint32_t>::max() - 2500;
    transport.networkAvailable = false;
    mqtt.setup();
    ASSERT_EQ(transport.connectAttempts, 1);
    tick(4999);
    EXPECT_EQ(transport.connectAttempts, 1);
    transport.networkAvailable = true;
    tick(1);
    EXPECT_EQ(transport.connectAttempts, 2);
    EXPECT_TRUE(mqtt.isConnected());
}

TEST_F(MqttSessionTest, WifiOutageSuppressesConnectAttemptsUntilWifiReturns) {
    WiFi.online = false;
    mqtt.setup();
    tick(60000);
    EXPECT_EQ(transport.connectAttempts, 0);
    WiFi.online = true;
    tick(0);
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_EQ(transport.connectAttempts, 1);
    transport.stop();
    WiFi.online = false;
    tick(60000);
    EXPECT_FALSE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 1);
    WiFi.online = true;
    tick(0);
    EXPECT_TRUE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 2);
}

TEST_F(MqttSessionTest, NoAttemptsBeforeSetupOrWithEmptyBrokerConfiguration) {
    tick(60000);
    EXPECT_EQ(transport.connectAttempts, 0);
    settings.strings["mqtt_server"] = "";
    mqtt.setup();
    tick(60000);
    EXPECT_EQ(transport.connectAttempts, 0);
    settings.strings["mqtt_server"] = "broker.test";
    mqtt.setup();
    EXPECT_EQ(transport.connectAttempts, 1);
    EXPECT_TRUE(mqtt.isConnected());
}

TEST_F(MqttSessionTest, ChangingSettingsEndsOldSessionAndUsesNewTopicsAndCredentials) {
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    settings.strings["mdns"] = "new-clock";
    settings.strings["mqtt_user"] = "new-user";
    settings.strings["mqtt_pass"] = "new-password";
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_EQ(transport.connections.size(), 2u);
    EXPECT_EQ(transport.connections.back().clientId, "new-clock");
    EXPECT_EQ(transport.connections.back().user, "new-user");
    EXPECT_EQ(transport.connections.back().password, "new-password");
    EXPECT_EQ(transport.connections.back().willTopic, "splitflap/new-clock/availability");
    EXPECT_EQ(transport.retained.at(availabilityTopic), "offline");
    EXPECT_EQ(transport.retained.at("splitflap/new-clock/availability"), "online");
    EXPECT_FALSE(transport.deliver(commandTopic, "OLD12:34"));
    ASSERT_TRUE(transport.deliver("splitflap/new-clock/set", "MON12:34"));
    mqtt.loop();
    EXPECT_EQ(display.lastMessage, "MON12:34");
}

TEST_F(MqttSessionTest, HealthyConnectionKeepsReceivingMinuteUpdatesForSimulatedDay) {
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    for (int minute = 0; minute < 24 * 60; ++minute) {
        const std::string message = "MIN" + std::to_string(minute);
        ASSERT_TRUE(transport.deliver(commandTopic, message));
        tick(60000);
        mqtt.loop(); // Drain any PINGRESP queued behind the command packet.
        ASSERT_TRUE(mqtt.isConnected());
        ASSERT_EQ(display.lastMessage, message);
    }
    EXPECT_EQ(display.writeCalls, 24 * 60);
    EXPECT_EQ(transport.connectAttempts, 1);
}

INSTANTIATE_TEST_SUITE_P(
    AnonymousAndAuthenticated, MqttRecoveryTest, testing::Combine(testing::Bool(), testing::Bool())
);
} // namespace
