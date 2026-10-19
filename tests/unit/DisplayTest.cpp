#include "Configuration.h"

#include <SplitFlapDisplay.h>
#include <SplitFlapMqtt.h>
#include <gtest/gtest.h>
extern int backgroundCalls;
class DisplayTest : public testing::Test {
  protected:
    JsonSettings settings = configuration();
    void SetUp() override {
        Preferences::storage.clear();
        Wire = TwoWire{};
        Wire1 = TwoWire{};
        fakeMillis = 0;
        fakeMicros = 0;
        backgroundCalls = 0;
    }
};
TEST_F(DisplayTest, CentersTruncatesAndClearsWithPublishedDisplayState) {
    SplitFlapDisplay display(settings);
    SplitFlapMqtt mqtt;
    display.init();
    display.setMqtt(&mqtt);
    const int width = display.getNumModules();
    display.writeString("ABC", 15, true);
    EXPECT_EQ(mqtt.state.size(), static_cast<size_t>(width));
    EXPECT_EQ(mqtt.state.find("ABC"), static_cast<size_t>((width - 3) / 2));
    display.writeString("ABCDEFGHIJKLMNO", 15, false);
    EXPECT_EQ(mqtt.state, std::string("ABCDEFGHIJKLMNO").substr(0, width));
    display.writeString("", 15, false);
    EXPECT_EQ(mqtt.state, std::string(width, ' '));
}

TEST_F(DisplayTest, BackgroundCallbackRunsDuringMotion) {
    TwoWire bus;
    SplitFlapModule module(32, 16, 0, 0, 37, &bus);
    module.init();
    backgroundCalls = 0;
    int target = 10;
    SplitFlapDisplay::moveModules(&module, 1, &target, 15, true, 16, 15, [] { ++backgroundCalls; });
    EXPECT_EQ(module.getPosition(), target);
    EXPECT_GT(backgroundCalls, 0);
}
TEST_F(DisplayTest, ZeroModuleMoveDoesNothing) {
    const auto before = Wire.writes.size();
    SplitFlapDisplay::moveModules(nullptr, 0, nullptr, 15, true, 96, 15);
    EXPECT_EQ(Wire.writes.size(), before);
}
TEST_F(DisplayTest, MotionClampsTargetsWrapsAndReleasesCoils) {
    TwoWire bus;
    SplitFlapModule module(32, 16, 0, 0, 37, &bus);
    module.init();
    int target = 99;
    SplitFlapDisplay::moveModules(&module, 1, &target, 15, true, 16, 15);
    EXPECT_EQ(module.getPosition(), 15);
    EXPECT_EQ(target, 15);
    target = -1;
    SplitFlapDisplay::moveModules(&module, 1, &target, 15, true, 16, 15);
    EXPECT_EQ(module.getPosition(), 0);
    EXPECT_EQ(target, 0);
    ASSERT_FALSE(bus.writes.empty());
    EXPECT_EQ(bus.writes.back().bytes, (std::vector<uint8_t>{0xe1, 0xff}));
}
TEST_F(DisplayTest, ReleaseFlagPreservesEnergizedCoils) {
    TwoWire bus;
    SplitFlapModule module(32, 16, 0, 0, 37, &bus);
    module.init();
    int target = 7;
    SplitFlapDisplay::moveModules(&module, 1, &target, 15, false, 16, 15);
    EXPECT_EQ(module.getPosition(), 7);
    EXPECT_NE(bus.writes.back().bytes, (std::vector<uint8_t>{0xe1, 0xff}));
}
#ifdef ENABLE_DUAL_I2C
TEST_F(DisplayTest, RowsUseIndependentWidthsAndBuses) {
    SplitFlapDisplay display(settings);
    SplitFlapMqtt mqtt;
    display.init();
    display.setMqtt(&mqtt);
    Wire.writes.clear();
    Wire1.writes.clear();
    display.writeStringDual("A", "BCDE", 15, true);
    EXPECT_EQ(mqtt.state, " A  |BCD");
    EXPECT_FALSE(Wire.writes.empty());
    EXPECT_FALSE(Wire1.writes.empty());
    for (const auto &w : Wire.writes) EXPECT_GE(w.address, 32);
    for (const auto &w : Wire1.writes) EXPECT_LE(w.address, 34);
}
#endif
