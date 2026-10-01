#include "Log.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

static_assert(LOG_BUFFER_SIZE >= 256, "LOG_BUFFER_SIZE must be at least 256 bytes");
DeviceLog Log;

void DeviceLog::begin() {
    // Called once in setup(), before any task can log. No allocations in write().
    writerMutex = xSemaphoreCreateMutex();
    configASSERT(writerMutex != nullptr);
}

size_t DeviceLog::write(uint8_t byte) {
    return write(&byte, 1);
}

void DeviceLog::append(const char *data, size_t length) {
    // Bound lock duration even for a large settings request. Serial, formatting,
    // allocation and network operations must never run under this spinlock.
    while (length > 0) {
        size_t count = std::min(length, size_t(256));
        portENTER_CRITICAL(&bufferMux);
        size_t offset = next % LOG_BUFFER_SIZE;
        size_t first = std::min(count, size_t(LOG_BUFFER_SIZE) - offset);
        memcpy(buffer + offset, data, first);
        memcpy(buffer, data + first, count - first);
        next += count;
        used = std::min(size_t(LOG_BUFFER_SIZE), used + count);
        portEXIT_CRITICAL(&bufferMux);
        data += count;
        length -= count;
    }
}

size_t DeviceLog::write(const uint8_t *data, size_t length) {
    if (length == 0) return 0;
    if (writerMutex == nullptr) return Serial.write(data, length);
    xSemaphoreTake(writerMutex, portMAX_DELAY);
    // Preserve the existing Serial byte stream, including Improv's shared port.
    size_t written = Serial.write(data, length);
    const char *text = reinterpret_cast<const char *>(data);
    size_t remaining = length;
    while (remaining > 0) {
        if (lineStart) {
            char prefix[24];
            int count = snprintf(prefix, sizeof(prefix), "[%lu] ", (unsigned long) millis());
            append(prefix, count);
            lineStart = false;
        }
        const char *newline = static_cast<const char *>(memchr(text, '\n', remaining));
        size_t count = newline == nullptr ? remaining : size_t(newline - text) + 1;
        append(text, count);
        lineStart = newline != nullptr;
        text += count;
        remaining -= count;
    }
    xSemaphoreGive(writerMutex);
    return written;
}

size_t DeviceLog::snapshot(char *output, size_t capacity, uint64_t &end) {
    portENTER_CRITICAL(&bufferMux);
    size_t count = std::min(used, capacity);
    end = next;
    size_t offset = (next - count) % LOG_BUFFER_SIZE;
    size_t first = std::min(count, size_t(LOG_BUFFER_SIZE) - offset);
    memcpy(output, buffer + offset, first);
    memcpy(output + first, buffer, count - first);
    portEXIT_CRITICAL(&bufferMux);
    return count;
}

size_t DeviceLog::readSince(uint64_t &cursor, char *output, size_t capacity, bool &dropped) {
    portENTER_CRITICAL(&bufferMux);
    uint64_t oldest = next - used;
    dropped = cursor < oldest || cursor > next;
    if (dropped) cursor = oldest;
    size_t count = std::min(uint64_t(capacity), next - cursor);
    size_t offset = cursor % LOG_BUFFER_SIZE;
    size_t first = std::min(count, size_t(LOG_BUFFER_SIZE) - offset);
    memcpy(output, buffer + offset, first);
    memcpy(output + first, buffer, count - first);
    cursor += count;
    portEXIT_CRITICAL(&bufferMux);
    return count;
}
