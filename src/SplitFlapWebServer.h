#pragma once

#include "JsonSettings.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

class SplitFlapDisplay;
class SplitFlapCluster;

class SplitFlapWebServer {
  public:
    SplitFlapWebServer(JsonSettings &settings);
    void init();
    void setDisplay(SplitFlapDisplay *displayPtr);
    void setCluster(SplitFlapCluster *clusterPtr);
    void setTimezone();
    void checkRebootRequired();

    // Wifi Connectivity
    bool loadWiFiCredentials();
    bool connectToWifi();
    bool getAttemptReconnect() const { return attemptReconnect; }
    void setAttemptReconnect(bool input) { attemptReconnect = input; }
    void startWebServer();
    void endMDNS();
    void startMDNS();
    void enableOta();
    void handleOta();
    void startAccessPoint();
    void checkWiFi();
    unsigned long getLastCheckWifiTime() { return lastCheckWifiTime; }
    void setLastCheckWifiTime(unsigned long input) { lastCheckWifiTime = input; }
    int getWifiCheckInterval() { return wifiCheckInterval; }

    // Mode
    int getMode();

    // Mode 0 - Single String
    String getInputString() const { return inputString; }
    String getWrittenString() const { return writtenString; }
    void setWrittenString(String input) { writtenString = input; }

    // Mode 1, Multi Input
    String getMultiInputString() const { return multiInputString; }
    int getMultiWordDelay() const { return multiWordDelay; }
    unsigned long getLastSwitchMultiTime() { return lastSwitchMultiTime; }
    void setLastSwitchMultiTime(unsigned long input) { lastSwitchMultiTime = input; }
    int getMultiWordCurrentIndex() { return multiWordCurrentIndex; }
    void setMultiWordCurrentIndex(int input) { multiWordCurrentIndex = input; }
    int getNumMultiWords() const { return numMultiWords; }

    // Mode 2, Date
    // Function to get current minute as a string
    String getCurrentMinute();
    String getCurrentHour();
    String getDayPrefix(int n);
    String getMonthPrefix(int n);
    String getCurrentDay();
    unsigned long getLastCheckDateTime() { return lastCheckDateTime; }
    void setLastCheckDateTime(unsigned long input) { lastCheckDateTime = input; }
    int getDateCheckInterval() { return checkDateInterval; }

    // Mode 7, Per-Display
    static const int MAX_DISPLAY_TEXTS = 32;
    String getDisplayText(int index) const { return (index >= 0 && index < MAX_DISPLAY_TEXTS) ? displayTexts[index] : ""; }
    void setDisplayTexts(const String* texts, int count) {
        int n = min(count, MAX_DISPLAY_TEXTS);
        for (int i = 0; i < n; i++) {
            displayTexts[i] = texts[i];
        }
        for (int i = n; i < MAX_DISPLAY_TEXTS; i++) {
            displayTexts[i] = "";
        }
        displayTextsUpdated = true;
    }
    bool hasDisplayTextsUpdated() const { return displayTextsUpdated; }
    void clearDisplayTextsUpdated() { displayTextsUpdated = false; }
    bool getDisplayCentering() const { return displayCentering; }

    // Mode 8, All Display Test
    unsigned long getTestModeDelay() const { return testModeDelay; }
    void setTestModeDelay(unsigned long delay) { testModeDelay = delay; }
    int getTestModeSkip() const { return testModeSkip; }
    void setTestModeSkip(int skip) { testModeSkip = skip; }
    unsigned long getLastTestModeTime() const { return lastTestModeTime; }
    void setLastTestModeTime(unsigned long time) { lastTestModeTime = time; }
    int getTestModeCharIndex() const { return testModeCharIndex; }
    void setTestModeCharIndex(int index) { testModeCharIndex = index; }
    char getTestModeCurrentChar() const { return testModeCurrentChar; }
    void setTestModeCurrentChar(char input) { testModeCurrentChar = input; }
    unsigned long getTestModeCycleCount() const { return testModeCycleCount; }
    void setTestModeCycleCount(unsigned long count) { testModeCycleCount = count; }

    int getCentering() { return centering; }

  private:
    JsonSettings &settings;
    SplitFlapDisplay *display;
    SplitFlapCluster *cluster;

    String decodeURIComponent(String encodedString);
    void setInputString(String input) { inputString = input; }
    void setMultiInputString(String input) { multiInputString = input; }

    void setMode(int targetMode);
    void setMultiDelay(int input) { multiWordDelay = input; }

    unsigned long lastCheckDateTime;
    int checkDateInterval;

    int connectionMode; // 0 is AP mode, 1 is Internet Mode
    int centering;      // whether to center text from custom imput

    int numMultiWords;
    unsigned long lastSwitchMultiTime;
    int multiWordDelay;
    int multiWordCurrentIndex;
    String multiInputString; // latest multi input from user

    String inputString;      // latest single input from user
    String writtenString;    // string for whatever is currently written to the display

    String displayTexts[MAX_DISPLAY_TEXTS];  // Mode 7: per-display texts (dis1-dis32)
    bool displayTextsUpdated; // Flag to indicate new display texts
    bool displayCentering;    // Whether to center text in per-display mode
    
    // Mode 8: All Display Test
    unsigned long testModeDelay = 5000;   // Delay between characters (ms)
    int testModeSkip = 1;                 // Characters to skip each cycle
    unsigned long lastTestModeTime = 0;   // Last character change time
    int testModeCharIndex = 0;            // Current character index
    char testModeCurrentChar = ' ';       // Current character being displayed
    unsigned long testModeCycleCount = 0; // Number of full character cycles

    int currentMode;         // Cached current mode (avoids constant Preferences reads)

    bool rebootRequired;
    bool attemptReconnect;
    unsigned long lastCheckWifiTime;
    int wifiCheckInterval;
    AsyncWebServer server; // Declare server as a class member
};
