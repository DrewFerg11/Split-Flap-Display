#pragma once

// Arduino's loop entry point and the handlers implemented by the sketch.
void loop();
void singleInputMode();
void multiInputMode();
void dateMode();
void timeMode();
void randomTest();
void accuracyTestMode();
void checkConnection();
void reconnectIfNeeded();
#ifdef ENABLE_DUAL_I2C
void dualSingleInputMode();
void dualMultiInputMode();
void dateTimeMode();
#endif
