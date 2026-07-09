#pragma once

// Defined in SplitFlapDisplay.ino - services Improv Wi-Fi during long
// blocking operations (motor moves, Wi-Fi connect retry). Only ever call this
// from the main thread: ImprovWiFi's internal frame-parse state isn't
// thread-safe, so the dual-I2C busMovementTask threads must NOT call it.
void backgroundTick();
