#include "Configuration.h"

#include <SplitFlapWebServer.h>
#include <gtest/gtest.h>
#include <memory>
class WebCommandTest : public testing::Test {
  protected:
    JsonSettings settings = configuration("config");
    std::unique_ptr<SplitFlapWebServer> web;
    void SetUp() override {
        Preferences::storage.clear();
        Preferences::openSessions = 0;
        fakeMillis = 0;
        WiFi = WiFiFake{};
        LittleFS = LittleFSFake{};
        ArduinoOTA = OTAFake{};
        ESP = EspFake{};
        configuredTimezone = "";
        web = std::make_unique<SplitFlapWebServer>(settings);
        web->startWebServer();
    }
    AsyncWebServerRequest request(const char *path, const char *json, int method = HTTP_POST) {
        return AsyncWebServer::active->request(path, json, method);
    }
};
TEST_F(WebCommandTest, SingleAndMultipleCommandsUpdateRealApplicationState) {
    auto one = request("/text", R"({"mode":"single","words":["HELLO%20WORLD"],"center":true})");
    ASSERT_EQ(one.status, 200);
    EXPECT_EQ(web->getMode(), 0);
    EXPECT_STREQ(web->getInputString().c_str(), "HELLO WORLD");
    EXPECT_TRUE(web->getCentering());
    auto many = request("/text", R"({"mode":"multiple","words":["ONE","TWO"],"delay":2})");
    ASSERT_EQ(many.status, 200);
    EXPECT_EQ(web->getMode(), 1);
    EXPECT_EQ(web->getNumMultiWords(), 2);
    EXPECT_EQ(web->getMultiWordDelay(), 2000);
    EXPECT_STREQ(web->getMultiInputString().c_str(), "ONE,TWO");
}
TEST_F(WebCommandTest, InvalidModesWordsAndShortDelaysAreRejected) {
    for (const char *json :
         {R"({})",
          R"({"mode":"bogus"})",
          R"({"mode":"single","words":"A"})",
          R"({"mode":"multiple","words":["A"],"delay":0})",
          R"({"mode":"dual-single","row1":"TOP"})"}) {
        auto response = request("/text", json);
        EXPECT_EQ(response.status, 400) << json;
    }
}
TEST_F(WebCommandTest, CommonUppercaseEscapesAreDecoded) {
    ASSERT_EQ(request("/text", R"({"mode":"single","words":["A%2FB%20%26%21"]})").status, 200);
    EXPECT_STREQ(web->getInputString().c_str(), "A/B &!");
}
TEST_F(WebCommandTest, SettingsOnlyTriggerRequiredLifecycleChanges) {
    EXPECT_EQ(request("/settings", R"({"name":"New"})").status, 200);
    EXPECT_FALSE(web->getAttemptReconnect());
    EXPECT_EQ(request("/settings", R"({"mqtt_server":"broker.test"})").status, 200);
    EXPECT_TRUE(web->getAttemptReconnect());
    web->setAttemptReconnect(false);
    EXPECT_EQ(request("/settings", R"({"sdaPin":9,"sclPin":9})").status, 400);
    EXPECT_FALSE(web->getAttemptReconnect());
    EXPECT_EQ(settings.getInt("sdaPin"), 8);
    EXPECT_EQ(request("/settings", R"({"otaPass":"secret"})").status, 200);
    web->checkRebootRequired();
    EXPECT_EQ(ESP.restarts, 1);
}
TEST_F(WebCommandTest, IncorrectHttpMethodIsRejected) {
    EXPECT_EQ(request("/text", R"({"mode":"single","words":["A"]})", HTTP_GET).status, 405);
}

TEST_F(WebCommandTest, ResetEndpointRestoresDefaultsAndRequestsReconnect) {
    settings.putString("name", "Changed");
    EXPECT_EQ(request("/settings/reset", "{}").status, 200);
    EXPECT_STREQ(settings.getString("name").c_str(), "Default");
    EXPECT_TRUE(web->getAttemptReconnect());
}

TEST_F(WebCommandTest, MissingCredentialsFallBackWithoutWaitingForTimeout) {
    EXPECT_FALSE(web->connectToWifi());
    EXPECT_EQ(WiFi.beginCalls, 0);
    EXPECT_EQ(fakeMillis, 0u);
    web->startAccessPoint();
    EXPECT_TRUE(WiFi.ap);
}

TEST_F(WebCommandTest, WifiTimeoutAndReconnectUseRealNetworkLogic) {
    settings.putString("ssid", "network");
    settings.putString("password", "pass");
    WiFi.online = false;
    WiFi.statusTick = 1000;
    EXPECT_FALSE(web->connectToWifi());
    EXPECT_GE(fakeMillis, 20000u);
    WiFi.online = true;
    EXPECT_TRUE(web->connectToWifi());
    EXPECT_TRUE(WiFi.autoReconnect);
    WiFi.online = false;
    web->checkWiFi();
    EXPECT_EQ(WiFi.reconnects, 1);
}
TEST_F(WebCommandTest, MissingTimezoneFileFallsBackToUtc) {
    settings.putString("timezone", "Local");
    web->setTimezone();
    EXPECT_EQ(configuredTimezone, "UTC0");
}
TEST_F(WebCommandTest, OtaRequiresAPasswordAndIsServicedWhenEnabled) {
    web->enableOta();
    EXPECT_FALSE(ArduinoOTA.begun);
    settings.putString("otaPass", "secret");
    web->enableOta();
    EXPECT_TRUE(ArduinoOTA.begun);
    web->handleOta();
    EXPECT_EQ(ArduinoOTA.handles, 1);
}
TEST_F(WebCommandTest, DualRowCommandPreservesEachRow) {
    ASSERT_EQ(request("/text", R"({"mode":"dual-single","row1":"TOP","row2":"BOTTOM"})").status, 200);
    EXPECT_EQ(web->getMode(), 7);
    EXPECT_STREQ(web->getDualRow1String().c_str(), "TOP");
    EXPECT_STREQ(web->getDualRow2String().c_str(), "BOTTOM");
}
