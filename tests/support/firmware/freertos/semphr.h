#pragma once
#include <cstddef>
#include <deque>
#include <set>
using SemaphoreHandle_t = int *;
// Mutexes are tracked so tests can assert work happens while one is held.
inline std::set<SemaphoreHandle_t> fakeMutexes;
inline int fakeMutexesHeld = 0;
inline SemaphoreHandle_t xSemaphoreCreateCounting(int, int count) {
    return new int(count);
}
inline SemaphoreHandle_t xSemaphoreCreateBinary() {
    return new int(0);
}
// Firmware objects own their mutex for the program's lifetime, so the fake keeps them in
// static storage (stable addresses) rather than leaking one per test object.
inline std::deque<int> fakeMutexStorage;
inline SemaphoreHandle_t xSemaphoreCreateMutex() {
    SemaphoreHandle_t mutex = &fakeMutexStorage.emplace_back(1);
    fakeMutexes.insert(mutex);
    return mutex;
}
inline void xSemaphoreGive(SemaphoreHandle_t s) {
    ++*s;
    if (fakeMutexes.count(s)) --fakeMutexesHeld;
}
inline int xSemaphoreTake(SemaphoreHandle_t s, int) {
    if (*s > 0) {
        --*s;
        if (fakeMutexes.count(s)) ++fakeMutexesHeld;
        return 1;
    }
    return 0;
}
inline void vSemaphoreDelete(SemaphoreHandle_t s) {
    if (fakeMutexes.erase(s)) return;
    delete s;
}
inline void vTaskDelete(void *) {}
inline int xTaskCreatePinnedToCore(void (*fn)(void *), const char *, int, void *arg, int, void *, int) {
    fn(arg);
    return 1;
}
#define pdMS_TO_TICKS(value) (value)
#define pdTRUE 1
#define portMAX_DELAY 0xffffffff
