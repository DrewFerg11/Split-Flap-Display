#pragma once

#include "JsonSettings.h"

// Only the web-server surface referenced by the sketch; no HTTP server runs.
class SplitFlapWebServer {
  public:
    explicit SplitFlapWebServer(JsonSettings &) {}
    int mode = 4;
    int otaCalls = 0;
    int rebootChecks = 0, connectCalls = 0, apCalls = 0;
    bool connectSucceeds = true, reconnect = false, center = true;
    std::function<void()> duringConnect;
    String input, written, multiple, dual1, dual2;
    int words = 1, wordIndex = 0, multiDelay = 1000, accuracyIndex = 0;
    unsigned long lastWifi = 0, lastDate = 0, lastMulti = 0, lastAccuracy = 0;

    String getDualRow1String() const { return dual1; }
    String getDualRow2String() const { return dual2; }
    int getMode() const { return mode; }
    void handleOta() { ++otaCalls; }
    void checkRebootRequired() { ++rebootChecks; }
    void init() {}
    bool connectToWifi() {
        ++connectCalls;
        if (duringConnect) duringConnect();
        return connectSucceeds;
    }
    void startAccessPoint() { ++apCalls; }
    void enableOta() {}
    void startMDNS() {}
    void endMDNS() {}
    void startWebServer() {}
    void checkWiFi() {}
    unsigned long getLastCheckWifiTime() const { return lastWifi; }
    void setLastCheckWifiTime(unsigned long value) { lastWifi = value; }
    int getWifiCheckInterval() const { return 1000; }
    bool getAttemptReconnect() const { return reconnect; }
    void setAttemptReconnect(bool value) { reconnect = value; }
    String getInputString() const { return input; }
    String getWrittenString() const { return written; }
    void setWrittenString(String value) { written = value; }
    int getCentering() const { return center; }
    String getMultiInputString() const { return multiple; }
    unsigned long getLastSwitchMultiTime() const { return lastMulti; }
    void setLastSwitchMultiTime(unsigned long value) { lastMulti = value; }
    int getMultiWordDelay() const { return 1000; }
    int getMultiWordCurrentIndex() const { return wordIndex; }
    void setMultiWordCurrentIndex(int value) { wordIndex = value; }
    int getNumMultiWords() const { return words; }
    unsigned long getLastCheckDateTime() const { return lastDate; }
    void setLastCheckDateTime(unsigned long value) { lastDate = value; }
    int getDateCheckInterval() const { return 250; }
    unsigned long getLastAccuracyStepTime() const { return lastAccuracy; }
    void setLastAccuracyStepTime(unsigned long value) { lastAccuracy = value; }
    int getAccuracyDelay() const { return 1000; }
    int getAccuracyCharIndex() const { return accuracyIndex; }
    void setAccuracyCharIndex(int value) { accuracyIndex = value; }
    int getAccuracyStepSize() const { return 1; }
};
