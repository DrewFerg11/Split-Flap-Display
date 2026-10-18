#pragma once

#include <Arduino.h>

struct WiFiFake
{
    String macAddress() const { return "00:00:00:00:00:00"; }
};
inline WiFiFake WiFi;
