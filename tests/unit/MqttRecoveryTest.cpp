#include <SplitFlapMqtt.h>
#include <WiFi.h>
#include <gtest/gtest.h>
#include <tuple>

namespace {
constexpr const char *commandTopic = "splitflap/clock/set";
constexpr const char *availabilityTopic = "splitflap/clock/availability";

// Authenticated/anonymous sessions, with either a definite socket loss or a
// half-open connection detected by the real library's keepalive timeout.
class MqttRecoveryTest : public testing::TestWithParam<std::tuple<bool, bool>> {
  protected:
    JsonSettings settings;
    WiFiClient transport;
    SplitFlapDisplay display;
    SplitFlapMqtt mqtt{settings, transport};

    void SetUp() override {
        fakeMillis = 0;
        WiFi.online = true;
        if (std::get<0>(GetParam())) {
            settings.strings["mqtt_user"] = "ha";
            settings.strings["mqtt_pass"] = "secret";
        }
        mqtt.setDisplay(&display);
    }
    void tick(unsigned int milliseconds) {
        fakeMillis += milliseconds;
        mqtt.loop();
    }
};

TEST_P(MqttRecoveryTest, LostSessionLeavesClockFrozenUntilSetupRunsAgain) {
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_EQ(transport.connections.size(), 1u);
    EXPECT_EQ(transport.connections.front().user, std::get<0>(GetParam()) ? "ha" : "");
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
    transport.networkAvailable = true;
    for (int second = 0; second < 60; ++second) tick(1000);

    // Characterize #70: the broker is reachable again, but no new connection
    // or subscription is made and fresh clock commands cannot be delivered.
    EXPECT_FALSE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 1);
    EXPECT_FALSE(transport.deliver(commandTopic, "MON12:36"));
    EXPECT_EQ(display.lastMessage, "MON12:34");
    EXPECT_EQ(display.writeCalls, 1);
    EXPECT_TRUE(transport.connections.front().willTopic.empty());
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");

    // Positive control: the same peer and callback work if setup is explicitly
    // rerun. The failure above is application recovery, not the test transport.
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    ASSERT_TRUE(transport.deliver(commandTopic, "MON12:37"));
    mqtt.loop();
    EXPECT_EQ(display.lastMessage, "MON12:37");
    EXPECT_EQ(display.writeCalls, 2);
}

INSTANTIATE_TEST_SUITE_P(
    AnonymousAndAuthenticated, MqttRecoveryTest, testing::Combine(testing::Bool(), testing::Bool())
);
} // namespace
