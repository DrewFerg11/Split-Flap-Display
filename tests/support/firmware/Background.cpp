#include <Arduino.h>
int backgroundCalls = 0;
void backgroundTick() {
    ++backgroundCalls;
}
