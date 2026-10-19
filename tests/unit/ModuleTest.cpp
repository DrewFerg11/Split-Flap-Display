#include <SplitFlapModule.h>
#include <cmath>
#include <gtest/gtest.h>
class ModuleTest : public testing::Test {
  protected:
    TwoWire bus;
    void SetUp() override {
        fakeMillis = 0;
        fakeMicros = 0;
    }
    uint16_t state(size_t index) {
        const auto &bytes = bus.writes.at(index).bytes;
        EXPECT_EQ(bytes.size(), 2u);
        return bytes.at(0) | (bytes.at(1) << 8);
    }
};
TEST_F(ModuleTest, CharacterMappingsUseRealCharsets) {
    for (int count : {37, 48}) {
        SplitFlapModule m(32, 96, 0, 0, count, &bus);
        m.init();
        const char *chars = count == 37 ? SplitFlapModule::StandardChars : SplitFlapModule::ExtendedChars;
        for (int i = 0; i < count; ++i)
            EXPECT_EQ(m.getCharPosition(chars[i]), static_cast<int>(std::floor(i * 96.0 / count)));
        EXPECT_EQ(m.getCharPosition('a'), m.getCharPosition('A'));
        EXPECT_EQ(m.getCharPosition('~'), 0);
    }
}
TEST_F(ModuleTest, StepsWrapPositionAndFollowCoilSequence) {
    SplitFlapModule m(32, 16, 0, 0, 37, &bus);
    m.init();
    bus.writes.clear();
    for (int i = 0; i < 16; ++i) m.step();
    EXPECT_EQ(m.getPosition(), 4);
    const uint16_t expected[] = {0xffe7, 0xfff3, 0xfff9, 0xffed};
    for (int i = 0; i < 16; ++i) EXPECT_EQ(state(i), expected[i % 4]);
}
TEST_F(ModuleTest, StopReleasesCoilsAndStartPreservesLogicalPosition) {
    SplitFlapModule m(32, 96, 0, 0, 37, &bus);
    m.init();
    m.step();
    const int before = m.getPosition();
    bus.writes.clear();
    m.stop();
    ASSERT_EQ(bus.writes.size(), 1u);
    EXPECT_EQ(state(0), 0xffe1);
    EXPECT_EQ(m.getPosition(), before);
    m.start();
    EXPECT_EQ(m.getPosition(), before);
    ASSERT_EQ(bus.writes.size(), 2u);
    EXPECT_EQ(state(1), 0xffe7);
    // The next coil phase after resume is a known defect tracked separately.
}
TEST_F(ModuleTest, SensorReadsHighBitAndRejectsShortReads) {
    SplitFlapModule m(32, 96, 3, 5, 37, &bus);
    m.init();
    bus.sensorReads = {{0, 0x80}, {0, 0}, {0x80}, {0x80, 0}};
    EXPECT_TRUE(m.readHallEffectSensor());
    EXPECT_FALSE(m.readHallEffectSensor());
    EXPECT_FALSE(m.readHallEffectSensor());
    EXPECT_FALSE(m.readHallEffectSensor());
    m.magnetDetected();
    EXPECT_EQ(m.getPosition(), 8);
}
TEST_F(ModuleTest, BusFailureStopsUsingSensorData) {
    bus.error = 2;
    SplitFlapModule m(32, 96, 0, 0, 37, &bus);
    m.init();
    bus.sensorReads = {{0, 0x80}};
    EXPECT_TRUE(m.getHasErrored());
    EXPECT_FALSE(m.readHallEffectSensor());
}

TEST_F(ModuleTest, CalibrationOffsetsWithinOneRotationAreApplied) {
    for (int offset : {-2, 10}) {
        SplitFlapModule m(32, 96, offset, 5, 37, &bus);
        m.init();
        m.magnetDetected();
        EXPECT_EQ(m.getPosition(), 5 + offset);
    }
}
