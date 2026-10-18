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
TEST_F(ModuleTest, StopAndStartDoNotRewindTheNextStep) {
    SplitFlapModule m(32, 96, 0, 0, 37, &bus);
    m.init();
    m.step();
    int before = m.getPosition();
    m.stop();
    bus.writes.clear();
    m.start();
    EXPECT_EQ(m.getPosition(), before);
    m.step();
    EXPECT_EQ(m.getPosition(), before + 1);
    ASSERT_EQ(bus.writes.size(), 2u);
    EXPECT_EQ(state(0), 0xffe7);
    EXPECT_EQ(state(1), 0xfff3);
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

TEST_F(ModuleTest, CalibrationOffsetsWrapIntoOneRotation) {
    for (int offset : {-10, 100}) {
        SplitFlapModule m(32, 96, offset, 5, 37, &bus);
        m.init();
        m.magnetDetected();
        EXPECT_EQ(m.getPosition(), ((5 + offset) % 96 + 96) % 96);
    }
}
