#include "AppLoop.h"

#include "BackgroundTick.h"

// Angle includes let the host test supply small substitutes. Firmware builds
// resolve these to the real headers in src/ and the Arduino framework.
#include <Arduino.h>
#include <SplitFlapMqtt.h>
#include <SplitFlapWebServer.h>

extern SplitFlapMqtt splitflapMqtt;
extern SplitFlapWebServer webServer;

void loop() {
    backgroundTick(); // Improv Wi-Fi (first 5 min after boot only)

    splitflapMqtt.loop();

    // check what mode the display is in, this value is updated by the web server
    switch (webServer.getMode()) {
        case 0: singleInputMode(); break;
        case 1: multiInputMode(); break;
        case 2: dateMode(); break;
        case 3: timeMode(); break;
        case 4: break;
        case 5: randomTest(); break;
        case 9: accuracyTestMode(); break;
#ifdef ENABLE_DUAL_I2C
        case 7: dualSingleInputMode(); break;
        case 8: dualMultiInputMode(); break;
        case 10: dateTimeMode(); break;
#endif
        default: break;
    }

    webServer.handleOta();
    checkConnection();

    reconnectIfNeeded();

    webServer.checkRebootRequired();
    yield();
}
