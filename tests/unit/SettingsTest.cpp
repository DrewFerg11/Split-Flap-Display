#include "Configuration.h"

#include <Preferences.h>
#include <gtest/gtest.h>
class SettingsTest : public testing::Test {
  protected:
    void SetUp() override {
        Preferences::storage.clear();
        Preferences::writes = 0;
        Preferences::openSessions = 0;
    }
    bool apply(JsonSettings &settings, const char *json) {
        JsonDocument doc;
        EXPECT_FALSE(deserializeJson(doc, json));
        return settings.fromJson(doc);
    }
};
TEST_F(SettingsTest, DefaultsAndTypedRoundTrips) {
    auto s = configuration();
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(s.getInt("mqtt_port"), 1883);
    s.putString("name", "Changed");
    s.putInt("mqtt_port", 1884);
    s.putFloat("maxVel", 12.5f);
    s.putIntVector("wireOffsets", {1, -2, 3, 0});
    auto reloaded = configuration();
    EXPECT_STREQ(reloaded.getString("name").c_str(), "Changed");
    EXPECT_EQ(reloaded.getInt("mqtt_port"), 1884);
    EXPECT_FLOAT_EQ(reloaded.getFloat("maxVel"), 12.5f);
    EXPECT_EQ(reloaded.getIntVector("wireOffsets"), (std::vector<int>{1, -2, 3, 0}));
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, ResetUsesConfiguredNamespace) {
    auto s = configuration();
    s.putString("name", "Changed");
    ASSERT_TRUE(s.reset());
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, RejectsMalformedIntegerLists) {
    for (const char *text : {"1-2", "--1", "32junk", "1,,2", ",1", "1,", "+1", " 1", "1, 2", "999999999999999999999"}) {
        JsonSetting field(std::vector<int>{});
        EXPECT_FALSE(field.validate(text)) << text;
    }
    JsonSetting valid(std::vector<int>{});
    EXPECT_TRUE(valid.validate("0,-2,3"));
    EXPECT_TRUE(valid.validate(""));
}
TEST_F(SettingsTest, RejectsUnknownKeysAndWrongTypesWithoutWriting) {
    auto s = configuration();
    for (const char *json :
         {R"({"unknown":1})", R"({"mqtt_port":"bad"})", R"({"name":9})", R"({"wireAddresses":[32,33]})"}) {
        int before = Preferences::writes;
        EXPECT_FALSE(apply(s, json)) << json;
        EXPECT_EQ(Preferences::writes, before);
        EXPECT_EQ(Preferences::openSessions, 0);
    }
}
TEST_F(SettingsTest, RejectedUpdateWritesNothing) {
    auto s = configuration();
    EXPECT_FALSE(apply(s, R"({"name":"Should not save","wireOffsets":"1-2,0,0,0"})"));
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(Preferences::writes, 0);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, PartialUpdatesValidateAgainstStoredConfiguration) {
    auto s = configuration();
    EXPECT_FALSE(apply(s, R"({"wireAddresses":"32,33"})"));
    EXPECT_FALSE(apply(s, R"({"sdaPin":9})"));
    ASSERT_TRUE(apply(s, R"({"wireAddresses":"32,33","wireOffsets":"0,-1"})"));
    EXPECT_EQ(s.getIntVector("wireAddresses"), (std::vector<int>{32, 33}));
}
TEST_F(SettingsTest, RejectsAddressRangeDuplicatesAndOversizedLists) {
    auto s = configuration();
    for (const char *json :
         {R"({"wireAddresses":"32,32,34,35"})",
          R"({"wireAddresses":"31,33,34,35"})",
          R"({"wireAddresses":"32,33,34,40"})",
          R"({"wireAddresses":"32,33,34,35,36,37,38,39,40","wireOffsets":"0,0,0,0,0,0,0,0,0"})"})
        EXPECT_FALSE(apply(s, json)) << json;
}
#ifdef ENABLE_DUAL_I2C
TEST_F(SettingsTest, SameAddressesOnSeparateBusesAreValidButSharedPinsAreNot) {
    auto s = configuration();
    EXPECT_TRUE(apply(s, R"({"wire1Addresses":"32,33,34"})"));
    EXPECT_FALSE(apply(s, R"({"sda2Pin":8})"));
}
#endif

TEST_F(SettingsTest, RejectsUnsafeMotionConfiguration) {
    auto s = configuration();
    for (const char *json : {R"({"stepsPerRot":0})", R"({"stepsPerRot":-1})", R"({"charset":99})", R"({"maxVel":0})"})
        EXPECT_FALSE(apply(s, json)) << json;
}
