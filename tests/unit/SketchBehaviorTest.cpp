#include "SketchDeclarations.h"

#include <gtest/gtest.h>
#define time sketchTime
#include <SplitFlapDisplay.ino>
#undef time

const auto initialStrings = settings.strings;
class SketchBehaviorTest : public testing::Test {
  protected:
    void SetUp() override {
        fakeMillis = 0;
        fakeMicros = 0;
        Serial.input.clear();
        ESP.restarts = 0;
        runtimeEvents.clear();
        settings.strings = initialStrings;
        webServer = SplitFlapWebServer(settings);
        display = SplitFlapDisplay(settings);
        splitflapMqtt = SplitFlapMqtt(settings, wifiClient);
        improv = ImprovWiFi(&Serial);
        improvActive = true;
        inImprovCallback = false;
        improvRebootPending = false;
    }
};
TEST_F(SketchBehaviorTest, TokenConversionDistinguishesMonthAndMinutes) {
    EXPECT_STREQ(convertToStrftime("{yyyy}-{mm}-{dd} {HH}:{MM} {AMPM}").c_str(), "%Y-%m-%d %H:%M %p");
    EXPECT_STREQ(convertToStrftime("literal {unknown}").c_str(), "literal {unknown}");
    EXPECT_STREQ(trimToModuleCount("ABCDEFGHI", 4).c_str(), "ABCD");
}
TEST_F(SketchBehaviorTest, ClockUsesConfiguredFormatAndDoesNotRewriteUnchangedOutput) {
    settings.strings["timeFormat"] = "{HH}:{MM}";
    fakeMillis = 1000;
    char expected[16];
    time_t epoch = fakeEpoch;
    strftime(expected, sizeof(expected), "%H:%M", localtime(&epoch));
    timeMode();
    EXPECT_EQ(display.lastMessage, expected);
    EXPECT_EQ(display.writeCalls, 1);
    fakeMillis += 1000;
    timeMode();
    EXPECT_EQ(display.writeCalls, 1);
}
TEST_F(SketchBehaviorTest, EmptyClockFormatUsesHoursAndMinutes) {
    settings.strings["timeFormat"] = "";
    fakeMillis = 1000;
    char expected[16];
    time_t epoch = fakeEpoch;
    strftime(expected, sizeof(expected), "%H:%M", localtime(&epoch));
    timeMode();
    EXPECT_EQ(display.lastMessage, expected);
}
TEST_F(SketchBehaviorTest, DefaultClockFormatDisplaysMinutesRatherThanMonth) {
    fakeMillis = 1000;
    char expected[16];
    time_t epoch = fakeEpoch;
    strftime(expected, sizeof(expected), "%H:%M", localtime(&epoch));
    timeMode();
    EXPECT_EQ(display.lastMessage, expected);
}

TEST_F(SketchBehaviorTest, SingleInputSuppressesRepeatedWritesButServicesMaintenance) {
    webServer.mode = 0;
    webServer.input = "HELLO";
    loop();
    loop();
    EXPECT_EQ(display.writeCalls, 1);
    EXPECT_EQ(display.lastMessage, "HELLO");
    EXPECT_EQ(splitflapMqtt.loopCalls, 2);
    EXPECT_EQ(webServer.otaCalls, 2);
}
TEST_F(SketchBehaviorTest, MultipleWordsAdvanceAtBoundaryAndWrap) {
    webServer.multiple = "ONE,TWO";
    webServer.words = 2;
    fakeMillis = 1000;
    multiInputMode();
    EXPECT_EQ(display.writeCalls, 0);
    ++fakeMillis;
    multiInputMode();
    EXPECT_EQ(display.lastMessage, "ONE");
    EXPECT_EQ(webServer.wordIndex, 1);
    fakeMillis += 1001;
    multiInputMode();
    EXPECT_EQ(display.lastMessage, "TWO");
    EXPECT_EQ(webServer.wordIndex, 0);
}
TEST_F(SketchBehaviorTest, EmptyWordListDoesNotDivideByZero) {
    webServer.words = 0;
    fakeMillis = 10000;
    multiInputMode();
    EXPECT_EQ(display.writeCalls, 0);
}
#ifdef ENABLE_DUAL_I2C
TEST_F(SketchBehaviorTest, OddDualWordListBlanksTheMissingSecondRowAndWraps) {
    webServer.multiple = "ONE,TWO,THREE";
    webServer.words = 3;
    fakeMillis = 1001;
    dualMultiInputMode();
    EXPECT_EQ(display.row1, "ONE");
    EXPECT_EQ(display.row2, "TWO");
    fakeMillis += 1001;
    dualMultiInputMode();
    EXPECT_EQ(display.row1, "THREE");
    EXPECT_EQ(display.row2, "");
    EXPECT_EQ(webServer.wordIndex, 0);
}
#endif
TEST_F(SketchBehaviorTest, ConnectedStartupInitializesAndHomesDisplayAndMqtt) {
    setup();
    EXPECT_EQ(webServer.apCalls, 0);
    EXPECT_EQ(display.initCalls, 1);
    EXPECT_EQ(display.homeCalls, 1);
    EXPECT_EQ(splitflapMqtt.setupCalls, 1);
}
TEST_F(SketchBehaviorTest, FailedWifiStartupFallsBackToAccessPointWithoutMqtt) {
    webServer.connectSucceeds = false;
    setup();
    EXPECT_EQ(webServer.apCalls, 1);
    EXPECT_EQ(display.initCalls, 1);
    EXPECT_EQ(display.lastMessage, "Wifi Err");
    EXPECT_EQ(splitflapMqtt.setupCalls, 0);
}
TEST_F(SketchBehaviorTest, SuccessfulProvisioningPersistsCredentialsAndRespondsBeforeRestart) {
    setup();
    display.writeCalls = 0;
    Serial.input.push_back(1);
    backgroundTick();
    EXPECT_EQ(settings.strings["ssid"], "new-ssid");
    EXPECT_EQ(settings.strings["password"], "new-password");
    EXPECT_EQ(ESP.restarts, 1);
    auto response = std::find(runtimeEvents.begin(), runtimeEvents.end(), "response");
    auto restart = std::find(runtimeEvents.begin(), runtimeEvents.end(), "restart");
    ASSERT_NE(response, runtimeEvents.end());
    ASSERT_NE(restart, runtimeEvents.end());
    EXPECT_LT(response, restart);
}
TEST_F(SketchBehaviorTest, FailedProvisioningDoesNotRestartAndConnectDoesNotReenterParser) {
    setup();
    webServer.connectSucceeds = false;
    webServer.duringConnect = [] { backgroundTick(); };
    Serial.input = {1, 2};
    backgroundTick();
    EXPECT_EQ(improv.handled, 2);
    EXPECT_EQ(ESP.restarts, 0);
}
TEST_F(SketchBehaviorTest, ProvisioningWindowExpiresAndStaysClosedAfterClockWrap) {
    fakeMillis = IMPROV_ACTIVE_MS;
    Serial.input.push_back(1);
    backgroundTick();
    EXPECT_EQ(improv.handled, 0);
    fakeMillis = 0;
    backgroundTick();
    EXPECT_EQ(improv.handled, 0);
}
TEST_F(SketchBehaviorTest, SettingsReconnectRequestRunsOnlyOnce) {
    webServer.reconnect = true;
    reconnectIfNeeded();
    reconnectIfNeeded();
    EXPECT_EQ(webServer.connectCalls, 1);
    EXPECT_EQ(splitflapMqtt.setupCalls, 1);
}
