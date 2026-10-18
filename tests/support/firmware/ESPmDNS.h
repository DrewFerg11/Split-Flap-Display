#pragma once
struct MDNSFake
{
    bool begin(const char *) { return true; }
    void end() {}
};
inline MDNSFake MDNS;
