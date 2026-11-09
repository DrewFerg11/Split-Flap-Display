#pragma once
#include <mutex>
#include <cassert>
struct portMUX_TYPE { std::mutex mutex; };
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(x) (x)->mutex.lock()
#define portEXIT_CRITICAL(x) (x)->mutex.unlock()
#define configASSERT(x) assert(x)
#define portMAX_DELAY 0
