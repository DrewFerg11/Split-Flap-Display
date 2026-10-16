#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 16384
#endif

// Byte cursors let HTTP snapshots and SSE consumers detect overwritten data.
class DeviceLog : public Print {
  public:
    void begin();
    using Print::write;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t *data, size_t length) override;
    size_t snapshot(char *output, size_t capacity, uint64_t &end);
    size_t readSince(uint64_t &cursor, char *output, size_t capacity, bool &dropped);

  private:
    void append(const char *data, size_t length);
    char buffer[LOG_BUFFER_SIZE] = {};
    uint64_t next = 0;
    size_t used = 0;
    bool lineStart = true;
    portMUX_TYPE bufferMux = portMUX_INITIALIZER_UNLOCKED;
    SemaphoreHandle_t writerMutex = nullptr;
};

extern DeviceLog Log;
