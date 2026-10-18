#include "SketchDeclarations.h"

#include <SplitFlapDisplay.h>
#include <SplitFlapMqtt.h>
#include <SplitFlapWebServer.h>
#include <gtest/gtest.h>

// These globals and loop() are defined by the unmodified production sketch.
extern SplitFlapMqtt splitflapMqtt;
extern SplitFlapWebServer webServer;
extern SplitFlapDisplay display;

TEST(AppEntryPointTest, IdleLoopStillServicesMqttAndWebMaintenance) {
    fakeMillis = 0;
    yieldCalls = 0;
    splitflapMqtt.loopCalls = 0;
    webServer.otaCalls = 0;
    webServer.rebootChecks = 0;
    display.writeCalls = 0;
    webServer.mode = 4;

    // Invoke loop() compiled from the complete SplitFlapDisplay.ino sketch.
    loop();

    EXPECT_EQ(splitflapMqtt.loopCalls, 1);
    EXPECT_EQ(webServer.otaCalls, 1);
    EXPECT_EQ(webServer.rebootChecks, 1);
    EXPECT_EQ(display.writeCalls, 0);
    EXPECT_EQ(yieldCalls, 1);
}
