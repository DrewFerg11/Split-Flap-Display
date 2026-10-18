#pragma once

// Minimal host substitutes for the sketch, not an Arduino compatibility layer.
#include <cstdint>
#include <ctime>
#include <string>
#include <utility>

class String {
  public:
    String(const char *value = "") : value(value) {}
    String(std::string value) : value(std::move(value)) {}
    unsigned int length() const { return static_cast<unsigned int>(value.length()); }
    bool isEmpty() const { return value.empty(); }
    const char *c_str() const { return value.c_str(); }
    char operator[](int index) const { return value[index]; }
    String substring(int start, int end) const { return value.substr(start, end - start); }
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

  private:
    std::string value;
};

inline uint32_t fakeMillis = 0;
inline int yieldCalls = 0;
inline unsigned long millis() {
    return fakeMillis;
}
inline void delay(unsigned long ms) {
    fakeMillis += ms;
}
inline void yield() {
    ++yieldCalls;
}

struct SerialFake
{
    void begin(int) {}
    int available() const { return 0; }
    void flush() {}
    void println(const char *) {}
    template <typename... Args> void printf(const char *, Args...) {}
};
inline SerialFake Serial;

struct EspFake
{
    const char *getChipModel() const { return "host-test"; }
    int getChipRevision() const { return 0; }
    void restart() {}
};
inline EspFake ESP;
