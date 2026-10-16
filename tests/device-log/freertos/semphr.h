#pragma once
#include "FreeRTOS.h"
using SemaphoreHandle_t = std::mutex*;
inline SemaphoreHandle_t xSemaphoreCreateMutex(){return new std::mutex;}
inline void xSemaphoreTake(SemaphoreHandle_t m,int){m->lock();}
inline void xSemaphoreGive(SemaphoreHandle_t m){m->unlock();}
