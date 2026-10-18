#pragma once

// Only the web-server surface used by AppLoop.cpp.
class SplitFlapWebServer {
  public:
    int mode = 4;
    int otaCalls = 0;
    int rebootChecks = 0;

    int getMode() const { return mode; }
    void handleOta() { ++otaCalls; }
    void checkRebootRequired() { ++rebootChecks; }
};
