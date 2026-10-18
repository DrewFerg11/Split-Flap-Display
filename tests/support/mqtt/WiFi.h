#pragma once

constexpr int WL_CONNECTED = 3;
struct WiFiFake
{
    bool online = true;
    int status() const { return online ? WL_CONNECTED : 0; }
};
inline WiFiFake WiFi;
