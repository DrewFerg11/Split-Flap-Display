#pragma once
#include <Arduino.h>
#include <cstring>
#include <map>
class File {
  public:
    bool valid = false, directory = false;
    std::string data;
    explicit operator bool() const { return valid; }
    bool isDirectory() const { return directory; }
    File openNextFile() { return {}; }
    const char *name() const { return ""; }
    size_t size() const { return data.size(); }
    size_t readBytes(char *buffer, size_t count) {
        count = std::min(count, data.size());
        std::memcpy(buffer, data.data(), count);
        return count;
    }
    void close() {}
};
struct LittleFSFake
{
    bool mounted = true;
    std::map<std::string, std::string> files;
    bool begin() { return mounted; }
    void end() {}
    File open(const char *path, const char * = "r") {
        File f;
        if (std::string(path) == "/") {
            f.valid = true;
            f.directory = true;
        } else if (files.count(path)) {
            f.valid = true;
            f.data = files[path];
        }
        return f;
    }
};
inline LittleFSFake LittleFS;
