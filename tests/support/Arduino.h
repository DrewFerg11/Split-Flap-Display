#pragma once

// Minimal host substitutes for the sketch, not an Arduino compatibility layer.
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <deque>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class String {
  public:
    String(const char *value = "") : value(value ? value : "") {}
    String(std::string value) : value(std::move(value)) {}
    String(float value) : value(std::to_string(value)) {}
    String(int value) : value(std::to_string(value)) {}
    unsigned int length() const { return static_cast<unsigned int>(value.length()); }
    bool isEmpty() const { return value.empty(); }
    const char *c_str() const { return value.c_str(); }
    char operator[](int index) const { return value[index]; }
    String substring(unsigned int start, unsigned int end) const {
        if (start > end) std::swap(start, end);
        start = std::min(start, length());
        end = std::min(end, length());
        return value.substr(start, end - start);
    }
    void replace(const char *from, const char *to) {
        const std::string token(from);
        size_t pos = 0;
        while ((pos = value.find(token, pos)) != std::string::npos) {
            value.replace(pos, token.length(), to);
            pos += std::string(to).length();
        }
    }
    bool operator!=(const String &other) const { return value != other.value; }
    String operator+(const String &other) const { return value + other.value; }
    String &operator+=(char ch) {
        value += ch;
        return *this;
    }
    int indexOf(char ch) const {
        const auto pos = value.find(ch);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    String substring(unsigned int start) const { return substring(start, length()); }
    String &operator+=(const String &other) {
        value += other.value;
        return *this;
    }
    bool operator==(const String &other) const { return value == other.value; }
    bool operator<(const String &other) const { return value < other.value; }
    bool concat(const char *text) {
        if (text) value += text;
        return true;
    }
    void remove(unsigned int index) {
        if (index < length()) value.erase(index);
    }
    bool endsWith(const char *suffix) const {
        std::string s(suffix);
        return value.size() >= s.size() && value.compare(value.size() - s.size(), s.size(), s) == 0;
    }
    void trim() {
        auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) value.clear();
        else value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    }
    void toUpperCase() {
        for (char &c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

  private:
    std::string value;
};

inline String operator+(const char *left, const String &right) {
    return String(left) + right;
}

inline uint32_t fakeMillis = 0;
inline int yieldCalls = 0;
inline uint64_t fakeMicros = 0;
inline unsigned long micros() {
    fakeMicros += 1000;
    return static_cast<uint32_t>(fakeMicros);
}
inline time_t fakeEpoch = 1792326840;
inline time_t sketchTime(time_t *result) {
    if (result) *result = fakeEpoch;
    return fakeEpoch;
}
inline bool getLocalTime(tm *result) {
    *result = *localtime(&fakeEpoch);
    return true;
}
inline std::vector<std::string> runtimeEvents;
template <typename T> T constrain(T value, T low, T high) {
    return std::min(std::max(value, low), high);
}
inline long random(long high) {
    return high > 0 ? high / 2 : 0;
}
inline long random(long low, long high) {
    return low + random(high - low);
}
using byte = uint8_t;
inline std::string configuredTimezone;
inline void configTzTime(const char *tz, const char *) {
    configuredTimezone = tz;
}
inline unsigned long millis() {
    return fakeMillis;
}
inline void delay(unsigned long ms) {
    fakeMillis += ms;
    fakeMicros += ms * 1000;
}
inline void yield() {
    ++yieldCalls;
}

struct SerialFake
{
    void begin(int) {}
    std::deque<uint8_t> input;
    int available() const { return static_cast<int>(input.size()); }
    int read() {
        if (input.empty()) return -1;
        auto b = input.front();
        input.pop_front();
        return b;
    }
    void flush() { runtimeEvents.push_back("flush"); }
    void println(const char *) {}
    void println(const String &) {}
    template <typename T> void println(T) {}
    void println() {}
    template <typename T> void print(T) {}
    template <typename... Args> void printf(const char *, Args...) {}
};
inline SerialFake Serial;

struct EspFake
{
    const char *getChipModel() const { return "host-test"; }
    int getChipRevision() const { return 0; }
    int restarts = 0;
    void restart() {
        ++restarts;
        runtimeEvents.push_back("restart");
    }
};
inline EspFake ESP;
