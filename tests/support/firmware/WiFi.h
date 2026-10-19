#pragma once
#include <Arduino.h>
constexpr int WL_CONNECTED = 3, WIFI_STA = 1;
struct HostIP
{
    String toString() const { return "127.0.0.1"; }
};
struct WiFiFake
{
    bool online = true, ap = false, autoReconnect = false;
    int reconnects = 0, beginCalls = 0;
    unsigned long statusTick = 0;
    int status() {
        fakeMillis += statusTick;
        return online ? WL_CONNECTED : 0;
    }
    void mode(int) {}
    void begin(const char *, const char *) { ++beginCalls; }
    void disconnect() {}
    void reconnect() { ++reconnects; }
    void softAPdisconnect() { ap = false; }
    void setAutoReconnect(bool on) { autoReconnect = on; }
    void persistent(bool) {}
    void setSleep(bool) {}
    void softAP(const char *) { ap = true; }
    HostIP softAPIP() { return {}; }
    HostIP localIP() { return {}; }
};
inline WiFiFake WiFi;
