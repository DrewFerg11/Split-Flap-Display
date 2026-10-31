#pragma once
#include <Arduino.h>
#include <map>
struct I2CWrite
{
    uint8_t address;
    std::vector<uint8_t> bytes;
};
class TwoWire {
  public:
    std::vector<I2CWrite> writes;
    std::deque<std::vector<uint8_t>> sensorReads;
    uint8_t error = 0;
    int begins = 0;
    bool begin(int, int) {
        ++begins;
        return true;
    }
    void setClock(int) {}
    void beginTransmission(uint8_t address) { pending = {address, {}}; }
    size_t write(uint8_t b) {
        pending.bytes.push_back(b);
        return 1;
    }
    uint8_t endTransmission() {
        writes.push_back(pending);
        return error;
    }
    uint8_t requestFrom(uint8_t, uint8_t) {
        input.clear();
        if (! sensorReads.empty()) {
            for (auto b : sensorReads.front()) input.push_back(b);
            sensorReads.pop_front();
        } else {
            input = {0, 0};
        }
        return static_cast<uint8_t>(input.size());
    }
    int available() { return static_cast<int>(input.size()); }
    int read() {
        if (input.empty()) return -1;
        auto b = input.front();
        input.pop_front();
        return b;
    }

  private:
    I2CWrite pending{};
    std::deque<uint8_t> input;
};
inline TwoWire Wire, Wire1;
