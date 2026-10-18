#pragma once

#include "Client.h"

#include <Arduino.h>
#include <deque>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// In-memory TCP transport plus a small MQTT broker peer. The real PubSubClient
// serializes/parses packets; this peer accepts CONNECT/SUBSCRIBE/PUBLISH and
// answers CONNACK/PINGRESP. It never reconnects or calls application callbacks.
class WiFiClient : public Client {
  public:
    struct Connection
    {
        std::string clientId, user, password, willTopic, willPayload;
        bool willRetained = false;
    };
    bool networkAvailable = true;
    bool blackhole = false;
    unsigned long connectDelayMs = 0;
    int connectAttempts = 0;
    int pingRequests = 0;
    std::vector<Connection> connections;
    std::map<std::string, std::string> retained;
    std::set<std::string> subscriptions;

    int connect(const char *, uint16_t) override {
        ++connectAttempts;
        fakeMillis += connectDelayMs;
        if (! networkAvailable) return 0;
        open = true;
        blackhole = false;
        input.clear();
        output.clear();
        subscriptions.clear();
        session = {};
        return 1;
    }
    int connect(IPAddress, uint16_t port) override { return connect("", port); }
    uint8_t connected() override { return open; }
    void stop() override {
        if (open && ! session.willTopic.empty()) {
            if (session.willRetained) retained[session.willTopic] = session.willPayload;
        }
        open = false;
        input.clear();
        output.clear();
        subscriptions.clear();
    }
    int available() override { return static_cast<int>(input.size()); }
    int read() override {
        if (input.empty()) return -1;
        const int value = input.front();
        input.pop_front();
        return value;
    }
    void flush() override {}
    size_t write(uint8_t value) override { return write(&value, 1); }
    size_t write(const uint8_t *data, size_t size) override {
        if (! open) return 0;
        if (blackhole) { // TCP appears alive but no broker replies.
            if (size == 2 && data[0] == 0xC0) ++pingRequests;
            return size;
        }
        output.insert(output.end(), data, data + size);
        processPackets();
        return size;
    }

    // Represents a broker publishing a fresh QoS-0 command to this session.
    bool deliver(const std::string &topic, const std::string &message) {
        if (! open || blackhole || ! subscriptions.count(topic)) return false;
        std::vector<uint8_t> body;
        appendString(body, topic);
        body.insert(body.end(), message.begin(), message.end());
        input.push_back(0x30);
        size_t remaining = body.size();
        do {
            uint8_t digit = remaining % 128;
            remaining /= 128;
            input.push_back(digit | (remaining ? 0x80 : 0));
        } while (remaining);
        input.insert(input.end(), body.begin(), body.end());
        return true;
    }

  private:
    bool open = false;
    Connection session;
    std::deque<uint8_t> input;
    std::vector<uint8_t> output;

    static void appendString(std::vector<uint8_t> &bytes, const std::string &text) {
        bytes.push_back(static_cast<uint8_t>(text.size() >> 8));
        bytes.push_back(static_cast<uint8_t>(text.size()));
        bytes.insert(bytes.end(), text.begin(), text.end());
    }
    static std::string takeString(const std::vector<uint8_t> &body, size_t &pos) {
        const size_t length = (body.at(pos) << 8) | body.at(pos + 1);
        pos += 2;
        if (pos + length > body.size()) throw std::runtime_error("Truncated MQTT string");
        const std::string value(body.begin() + pos, body.begin() + pos + length);
        pos += length;
        return value;
    }
    void processPackets() {
        while (output.size() >= 2) {
            size_t pos = 1, remaining = 0, multiplier = 1;
            uint8_t digit;
            do {
                if (pos >= output.size()) return;
                digit = output[pos++];
                remaining += (digit & 0x7F) * multiplier;
                multiplier *= 128;
            } while (digit & 0x80);
            if (output.size() < pos + remaining) return;
            const auto header = output.front();
            const std::vector<uint8_t> body(output.begin() + pos, output.begin() + pos + remaining);
            output.erase(output.begin(), output.begin() + pos + remaining);
            handlePacket(header, body);
        }
    }
    void handlePacket(uint8_t header, const std::vector<uint8_t> &body) {
        size_t pos = 0;
        switch (header & 0xF0) {
            case 0x10: {               // CONNECT: decode the real client's flags and payload.
                takeString(body, pos); // Protocol name
                ++pos;                 // Protocol level
                const auto flags = body.at(pos++);
                pos += 2;              // Keepalive
                session.clientId = takeString(body, pos);
                if (flags & 0x04) {
                    session.willTopic = takeString(body, pos);
                    session.willPayload = takeString(body, pos);
                    session.willRetained = flags & 0x20;
                }
                if (flags & 0x80) session.user = takeString(body, pos);
                if (flags & 0x40) session.password = takeString(body, pos);
                connections.push_back(session);
                input.insert(input.end(), {0x20, 0x02, 0x00, 0x00}); // CONNACK
                break;
            }
            case 0x80:   // SUBSCRIBE: new sessions start without subscriptions.
                pos = 2; // Packet ID
                subscriptions.insert(takeString(body, pos));
                break;
            case 0x30: { // PUBLISH: retained availability is visible to HA.
                const auto topic = takeString(body, pos);
                if (header & 1) retained[topic] = std::string(body.begin() + pos, body.end());
                break;
            }
            case 0xC0:
                ++pingRequests;
                input.insert(input.end(), {0xD0, 0x00});
                break;
            case 0xE0: session.willTopic.clear(); break; // Graceful DISCONNECT suppresses the will.
            default: throw std::runtime_error("Unexpected client MQTT packet");
        }
    }
};
