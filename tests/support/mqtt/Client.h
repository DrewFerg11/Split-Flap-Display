#pragma once

#include "IPAddress.h"
#include "Stream.h"

class Client : public Stream {
  public:
    virtual int connect(const char *, uint16_t) = 0;
    virtual int connect(IPAddress, uint16_t) = 0;
    virtual uint8_t connected() = 0;
    virtual void stop() = 0;
};
