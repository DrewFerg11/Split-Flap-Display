#pragma once

#include "Print.h"

class Stream : public Print {
  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual void flush() = 0;
};
