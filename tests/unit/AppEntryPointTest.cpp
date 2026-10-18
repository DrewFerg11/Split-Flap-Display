#include "AppLoop.h"
#include "BackgroundTick.h"

#include <SplitFlapMqtt.h>
#include <SplitFlapWebServer.h>
#include <gtest/gtest.h>

SplitFlapMqtt splitflapMqtt;
SplitFlapWebServer webServer;

// These handlers belong to the sketch. Their internals are outside this test.
void backgroundTick() {}
void checkConnection() {}
void reconnectIfNeeded() {}
void yield() {}

void singleInputMode() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}
void multiInputMode() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}
void dateMode() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}
void timeMode() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}
void randomTest() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}
void accuracyTestMode() {
    ADD_FAILURE() << "Idle mode must not dispatch a display handler";
}

TEST(AppEntryPointTest, IdleLoopStillServicesMqttAndWebMaintenance) {
    splitflapMqtt = SplitFlapMqtt{};
    webServer = SplitFlapWebServer{};
    webServer.mode = 4;

    // Invoke the actual firmware entry point, compiled from src/AppLoop.cpp.
    loop();

    EXPECT_EQ(splitflapMqtt.loopCalls, 1);
    EXPECT_EQ(webServer.otaCalls, 1);
    EXPECT_EQ(webServer.rebootChecks, 1);
}
