#include <SplitFlapMqtt.h>
#include <WiFi.h>
#include <gtest/gtest.h>
#include <limits>
#include <string>
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
    // Loops in 100 ms steps until the session is usable; returns false on timeout.
    bool recoversWithin(unsigned int milliseconds) {
        for (unsigned int elapsed = 0; elapsed <= milliseconds; elapsed += 100) {
            if (mqtt.isConnected() && transport.subscriptions.count(commandTopic)) return true;
            tick(100);
        }
        return false;
    }
    bool commandReachesDisplay(const std::string &message) {
        if (! transport.deliver(commandTopic, message)) return false;
        mqtt.loop();
        return display.lastMessage == message;
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

TEST_F(MqttSessionTest, RefusedConnectIsRetriedEveryFiveSecondsThenRecovers) {
    transport.connackCode = 5; // Not authorized, e.g. a broker-side credential change.
    mqtt.setup();
    EXPECT_FALSE(mqtt.isConnected());
    for (int step = 0; step < 600; ++step) tick(100);
    EXPECT_FALSE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 13);
    transport.connackCode = 0;
    ASSERT_TRUE(recoversWithin(5000));
    EXPECT_TRUE(commandReachesDisplay("MON12:34"));
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");
}

TEST_F(MqttSessionTest, UnresponsiveBrokerTimesOutAndRetriesUntilItAnswers) {
    transport.silentBroker = true;
    mqtt.setup();
    EXPECT_FALSE(mqtt.isConnected());
    ASSERT_EQ(transport.connectAttempts, 1);
    for (int step = 0; step < 1200; ++step) tick(100);
    EXPECT_GT(transport.connectAttempts, 2);
    transport.silentBroker = false;
    ASSERT_TRUE(recoversWithin(25000));
    EXPECT_TRUE(commandReachesDisplay("MON12:34"));
}

TEST_F(MqttSessionTest, HalfOpenSessionIsReplacedWithinTwoKeepalivesWhilePublishingState) {
    mqtt.setup();
    ASSERT_TRUE(commandReachesDisplay("MON12:00"));
    transport.blackhole = true;
    // Outbound state publishes must not mask the missing inbound traffic.
    unsigned int elapsed = 0;
    while (transport.connections.size() == 1 && elapsed <= 40000) {
        if (elapsed % 2000 == 0) mqtt.publishState("MON12:00");
        tick(100);
        elapsed += 100;
    }
    ASSERT_EQ(transport.connections.size(), 2u);
    EXPECT_LE(elapsed, 31000u);
    ASSERT_TRUE(recoversWithin(0));
    EXPECT_TRUE(commandReachesDisplay("MON12:01"));
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");
}

TEST_F(MqttSessionTest, WifiLossWithStaleSocketRecoversWhenWifiReturns) {
    mqtt.setup();
    ASSERT_TRUE(commandReachesDisplay("MON12:00"));
    WiFi.online = false;
    transport.blackhole = true; // The old socket still looks open.
    for (int step = 0; step < 600; ++step) tick(100);
    EXPECT_FALSE(mqtt.isConnected());
    EXPECT_EQ(transport.connectAttempts, 1);
    EXPECT_EQ(transport.retained.at(availabilityTopic), "offline");
    WiFi.online = true;
    ASSERT_TRUE(recoversWithin(5000));
    EXPECT_TRUE(commandReachesDisplay("MON12:05"));
}

TEST_F(MqttSessionTest, PublishingStateWhileDisconnectedIsHarmless) {
    mqtt.setup();
    transport.networkAvailable = false;
    transport.stop();
    for (int step = 0; step < 20; ++step) {
        mqtt.publishState("X");
        tick(500);
    }
    EXPECT_FALSE(mqtt.isConnected());
    transport.networkAvailable = true;
    ASSERT_TRUE(recoversWithin(5000));
    EXPECT_TRUE(commandReachesDisplay("OK"));
}

// A week of minute updates with ~200 seeded outages of every modeled kind.
// Commands sent mid-outage are lost (QoS 0), but the clock must never stay frozen.
TEST_F(MqttSessionTest, WeekOfMinuteUpdatesSelfHealsAfterRepeatedMixedOutages) {
    mqtt.setup();
    ASSERT_TRUE(mqtt.isConnected());
    uint32_t seed = 12345;
    auto nextRandom = [&seed] {
        seed = seed * 1103515245u + 12345u;
        return (seed >> 16) & 0x7fff;
    };
    int outages = 0, outageMinutesLeft = 0, kind = 0, lastShownMinute = 0;
    for (int minute = 1; minute <= 7 * 24 * 60; ++minute) {
        if (outageMinutesLeft == 0 && nextRandom() % 50 == 0) {
            ++outages;
            kind = nextRandom() % 4;
            outageMinutesLeft = 1 + nextRandom() % 3;
            switch (kind) {
                case 0: transport.stop(); break;           // Broker restart or TCP reset.
                case 1: transport.blackhole = true; break; // Idle TCP dropped by NAT.
                case 2:                                    // Wi-Fi blip leaving a stale socket.
                    WiFi.online = false;
                    transport.blackhole = true;
                    break;
                case 3: // Broker down for a few minutes.
                    transport.networkAvailable = false;
                    transport.stop();
                    break;
            }
        }
        if (outageMinutesLeft > 0 && --outageMinutesLeft == 0) {
            WiFi.online = true;
            transport.networkAvailable = true;
        }
        if (commandReachesDisplay("T" + std::to_string(minute))) lastShownMinute = minute;
        for (int second = 0; second < 60; ++second) tick(1000);
        ASSERT_LE(minute - lastShownMinute, 6) << "Frozen at minute " << minute << " after outage kind " << kind;
    }
    EXPECT_GE(outages, 150);
    EXPECT_TRUE(mqtt.isConnected());
    EXPECT_EQ(transport.retained.at(availabilityTopic), "online");
}

INSTANTIATE_TEST_SUITE_P(
    AnonymousAndAuthenticated, MqttRecoveryTest, testing::Combine(testing::Bool(), testing::Bool())
);
} // namespace
