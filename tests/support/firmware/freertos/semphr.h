#pragma once
#include <cstddef>
using SemaphoreHandle_t = int *;
inline SemaphoreHandle_t xSemaphoreCreateCounting(int, int count) {
    return new int(count);
}
inline SemaphoreHandle_t xSemaphoreCreateBinary() {
    return new int(0);
}
inline void xSemaphoreGive(SemaphoreHandle_t s) {
    ++*s;
}
inline int xSemaphoreTake(SemaphoreHandle_t s, int) {
    if (*s > 0) {
        --*s;
        return 1;
    }
    return 0;
}
inline void vSemaphoreDelete(SemaphoreHandle_t s) {
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
