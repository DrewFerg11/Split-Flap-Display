#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
class Print { public: virtual ~Print() {} virtual size_t write(uint8_t)=0; virtual size_t write(const uint8_t*d,size_t n){for(size_t i=0;i<n;i++)write(d[i]);return n;} };
struct FakeSerial { size_t write(const uint8_t*,size_t n){return n;} };
extern FakeSerial Serial;
inline unsigned long millis(){return 42;}
