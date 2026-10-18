#pragma once

#include "../Arduino.h"

#include <cstdlib>
#include <cstring>

using byte = uint8_t;
class __FlashStringHelper;
#define PROGMEM
#define PGM_P const char *
#define memcpy_P memcpy
#define strlen_P strlen
#define strnlen_P strnlen
#define pgm_read_byte_near(address) (*(address))
