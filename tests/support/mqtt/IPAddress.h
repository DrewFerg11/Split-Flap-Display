#pragma once

#include <cstdint>

class IPAddress {
  public:
    IPAddress() = default;
    IPAddress(uint8_t, uint8_t, uint8_t, uint8_t) {}
};
