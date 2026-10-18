#pragma once

#include "JsonSettings.h"

// Only the web-server surface referenced by the sketch; no HTTP server runs.
class SplitFlapWebServer {
  public:
    explicit SplitFlapWebServer(JsonSettings &) {}
    int mode = 4;
    int otaCalls = 0;
    int rebootChecks = 0;

    int getMode() const { return mode; }
    void handleOta() { ++otaCalls; }
    void checkRebootRequired() { ++rebootChecks; }
    void init() {}
    bool connectToWifi() { return true; }
    void startAccessPoint() {}
    void enableOta() {}
    void startMDNS() {}
    void endMDNS() {}
    void startWebServer() {}
    void checkWiFi() {}
    unsigned long getLastCheckWifiTime() const { return 0; }
    void setLastCheckWifiTime(unsigned long) {}
    int getWifiCheckInterval() const { return 1000; }
    bool getAttemptReconnect() const { return false; }
    void setAttemptReconnect(bool) {}
    String getInputString() const { return ""; }
    String getWrittenString() const { return ""; }
    void setWrittenString(String) {}
    int getCentering() const { return true; }
    String getMultiInputString() const { return ""; }
    unsigned long getLastSwitchMultiTime() const { return 0; }
    void setLastSwitchMultiTime(unsigned long) {}
    int getMultiWordDelay() const { return 1000; }
    int getMultiWordCurrentIndex() const { return 0; }
    void setMultiWordCurrentIndex(int) {}
    int getNumMultiWords() const { return 1; }
    unsigned long getLastCheckDateTime() const { return 0; }
    void setLastCheckDateTime(unsigned long) {}
    int getDateCheckInterval() const { return 1000; }
    unsigned long getLastAccuracyStepTime() const { return 0; }
    void setLastAccuracyStepTime(unsigned long) {}
    int getAccuracyDelay() const { return 1000; }
    int getAccuracyCharIndex() const { return 0; }
    void setAccuracyCharIndex(int) {}
    int getAccuracyStepSize() const { return 1; }
};
