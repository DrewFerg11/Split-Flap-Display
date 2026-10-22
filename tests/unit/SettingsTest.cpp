#include "Configuration.h"

#include <Preferences.h>
#include <gtest/gtest.h>
class SettingsTest : public testing::Test {
  protected:
    void SetUp() override {
        Preferences::storage.clear();
        Preferences::writes = 0;
        Preferences::openSessions = 0;
        Preferences::unlockedOpens = 0;
        Preferences::absentReads = 0;
        Preferences::opens.clear();
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
TEST_F(SettingsTest, IntegerListsRejectWhatWouldThrowWhenReadBackAtBoot) {
    JsonSetting field(std::vector<int>{});
    EXPECT_TRUE(field.validate("2147483647,-2147483648"));
    for (const char *text : {"999999999999", "2147483648", "-2147483649", "1,,2", ",1", "-", "--1", "1-2"}) {
        EXPECT_FALSE(field.validate(text)) << text;
    }
}
TEST_F(SettingsTest, AnOutOfRangeOffsetIsRejectedAndWritesNothing) {
    auto s = configuration();
    EXPECT_FALSE(apply(s, R"({"wireOffsets":"999999999999"})"));
    EXPECT_STREQ(s.getLastValidationKey().c_str(), "wireOffsets");
    EXPECT_EQ(Preferences::writes, 0);
    EXPECT_NO_THROW(s.getIntVector("wireOffsets"));
    EXPECT_EQ(Preferences::openSessions, 0);
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
TEST_F(SettingsTest, EveryStoreAccessHoldsTheSettingsLockAndReleasesIt) {
    auto s = configuration();
    s.getString("name");
    s.getInt("mqtt_port");
    s.getFloat("maxVel");
    s.getIntVector("wireOffsets");
    s.putString("name", "Changed");
    s.putInt("mqtt_port", 1884);
    s.putFloat("maxVel", 12.5f);
    s.putIntVector("wireOffsets", {1, 2, 3, 4});
    s.toJson();
    ASSERT_TRUE(apply(s, R"({"name":"Again"})"));
    ASSERT_TRUE(s.reset());
    EXPECT_FALSE(Preferences::opens.empty());
    EXPECT_EQ(Preferences::unlockedOpens, 0);
    EXPECT_EQ(fakeMutexesHeld, 0);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, UnknownKeysAreSkippedInsteadOfCrashing) {
    auto s = configuration();
    // A settings page from another firmware version or bus layout can send keys this build lacks.
    ASSERT_TRUE(apply(s, R"({"name":"Changed","wire9Addresses":"32","noSuchKey":7})"));
    EXPECT_STREQ(s.getString("name").c_str(), "Changed");
    EXPECT_EQ(Preferences::storage["test-config"].count("noSuchKey"), 0u);
    EXPECT_STREQ(s.getString("noSuchKey").c_str(), "");
    EXPECT_EQ(s.getInt("noSuchKey"), 0);
    EXPECT_FLOAT_EQ(s.getFloat("noSuchKey"), 0.0f);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, AnInvalidValueWritesNothingAndClosesTheStore) {
    auto s = configuration();
    EXPECT_FALSE(apply(s, R"({"name":"Should not save","wireOffsets":"1,x"})"));
    EXPECT_STREQ(s.getLastValidationKey().c_str(), "wireOffsets");
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(Preferences::writes, 0);
    EXPECT_EQ(Preferences::openSessions, 0);
}
TEST_F(SettingsTest, TheNamespaceIsCreatedBeforeTheFirstReadOnlyOpen) {
    auto s = configuration();
    s.getString("name");
    s.getInt("mode");
    ASSERT_EQ(Preferences::opens.size(), 3u);
    EXPECT_EQ(Preferences::opens[0], std::make_pair(std::string("test-config"), false));
    EXPECT_EQ(Preferences::opens[1], std::make_pair(std::string("test-config"), true));
    EXPECT_EQ(Preferences::opens[2], std::make_pair(std::string("test-config"), true));
}
TEST_F(SettingsTest, ResetClearsOnlyItsOwnNamespace) {
    Preferences::storage["config"]["name"] = std::string("Other store");
    auto s = configuration("test-config");
    s.putString("name", "Changed");
    ASSERT_TRUE(s.reset());
    EXPECT_STREQ(s.getString("name").c_str(), "Default");
    EXPECT_EQ(std::get<std::string>(Preferences::storage["config"]["name"]), "Other store");
}
TEST_F(SettingsTest, UnsetKeysUseDefaultsWithoutMissingKeyReads) {
    // Fresh flash, then a partial configuration: only some keys are ever written.
    auto s = configuration();
    ASSERT_TRUE(apply(s, R"({"wireAddresses":"32,33","wireOffsets":"0,1"})"));
    auto rebooted = configuration();
    EXPECT_STREQ(rebooted.getString("name").c_str(), "Default");
    EXPECT_EQ(rebooted.getInt("mqtt_port"), 1883);
    EXPECT_FLOAT_EQ(rebooted.getFloat("maxVel"), 15.0f);
    EXPECT_EQ(rebooted.getIntVector("wireAddresses"), (std::vector<int>{32, 33}));
    EXPECT_EQ(rebooted.getIntVector("wireOffsets"), (std::vector<int>{0, 1}));
    auto exported = rebooted.toJson();
    EXPECT_STREQ(exported["name"].as<const char *>(), "Default");
    EXPECT_STREQ(exported["wireOffsets"].as<const char *>(), "0,1");
    EXPECT_EQ(Preferences::absentReads, 0);
}
