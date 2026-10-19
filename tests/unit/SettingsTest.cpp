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
TEST_F(SettingsTest, ResetRestoresDefaultApplicationNamespace) {
    JsonSettings s("config", {{"name", JsonSetting(String("Default"))}});
    s.putString("name", "Changed");
    ASSERT_TRUE(s.reset());
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, IntegerListsAcceptSignedValuesAndRejectNonIntegerCharacters) {
    JsonSetting field(std::vector<int>{});
    EXPECT_TRUE(field.validate("0,-2,3"));
    EXPECT_TRUE(field.validate(""));
    for (const char *text : {"32junk", "+1", " 1", "1, 2", "1.5"}) EXPECT_FALSE(field.validate(text)) << text;
}
TEST_F(SettingsTest, JsonUpdatesPersistValuesAndExportTheirTypes) {
    auto s = configuration();
    ASSERT_TRUE(apply(s, R"({"name":"Changed","mqtt_port":1884,"maxVel":12.5})"));
    auto reloaded = configuration();
    auto exported = reloaded.toJson();
    EXPECT_TRUE(exported["name"].is<String>());
    EXPECT_STREQ(exported["name"].as<const char *>(), "Changed");
    EXPECT_TRUE(exported["mqtt_port"].is<int>());
    EXPECT_EQ(exported["mqtt_port"].as<int>(), 1884);
    EXPECT_FLOAT_EQ(exported["maxVel"].as<float>(), 12.5f);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, NumericFormStringsAreConvertedForPersistence) {
    auto s = configuration();
    // Plain Alpine x-model bindings send these values as strings after editing.
    ASSERT_TRUE(apply(s, R"({"mqtt_port":"1884","maxVel":"12.5"})"));
    EXPECT_EQ(s.getInt("mqtt_port"), 1884);
    EXPECT_FLOAT_EQ(s.getFloat("maxVel"), 12.5f);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, CompleteInvalidConfigurationIsRejectedBeforeWriting) {
    auto s = configuration();
    for (const char *json :
         {R"({"name":"Should not save","wireAddresses":"32,33","wireOffsets":"0"})", R"({"sdaPin":9,"sclPin":9})"}) {
        EXPECT_FALSE(apply(s, json)) << json;
        EXPECT_FALSE(s.getLastValidationError().isEmpty());
        EXPECT_STREQ(s.getString("name").c_str(), "Default");
        EXPECT_EQ(Preferences::writes, 0);
        EXPECT_EQ(Preferences::openSessions, 0);
    }
}
TEST_F(SettingsTest, MatchedAddressAndOffsetUpdatesPersistTogether) {
    auto s = configuration();
    ASSERT_TRUE(apply(s, R"({"wireAddresses":"32,33","wireOffsets":"0,-1"})"));
    EXPECT_EQ(s.getIntVector("wireAddresses"), (std::vector<int>{32, 33}));
    EXPECT_EQ(s.getIntVector("wireOffsets"), (std::vector<int>{0, -1}));
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
TEST_F(SettingsTest, SameAddressesOnSeparateBusesAreValidButCompletePinConflictsAreNot) {
    auto s = configuration();
    EXPECT_TRUE(apply(s, R"({"wire1Addresses":"32,33,34"})"));
    EXPECT_FALSE(apply(s, R"({"sdaPin":8,"sclPin":9,"sda2Pin":8,"scl2Pin":11})"));
}
#endif
