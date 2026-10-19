#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <memory>
constexpr int HTTP_GET = 1, HTTP_POST = 2;
class AsyncWebServerRequest {
  public:
    int verb = HTTP_POST, status = 0;
    String path, body;
    int method() const { return verb; }
    const char *methodToString() const { return verb == HTTP_POST ? "POST" : "GET"; }
    String url() const { return path; }
    void send(int code, const char * = "", String text = "") {
        status = code;
        body = text;
    }
    void redirect(const char *) { status = 302; }
};
struct AsyncCallbackJsonWebHandler
{
    String path;
    std::function<void(AsyncWebServerRequest *, JsonVariant &)> callback;
    AsyncCallbackJsonWebHandler(const char *p, std::function<void(AsyncWebServerRequest *, JsonVariant &)> fn)
        : path(p), callback(fn) {}
};
class AsyncWebServer {
  public:
    inline static AsyncWebServer *active = nullptr;
    explicit AsyncWebServer(int) { active = this; }
    std::vector<std::unique_ptr<AsyncCallbackJsonWebHandler>> jsonHandlers;
    std::map<std::pair<std::string, int>, std::function<void(AsyncWebServerRequest *)>> routes;
    void on(const char *path, int method, std::function<void(AsyncWebServerRequest *)> fn) {
        routes[{path, method}] = fn;
    }
    void onNotFound(std::function<void(AsyncWebServerRequest *)>) {}
    void addHandler(AsyncCallbackJsonWebHandler *handler) { jsonHandlers.emplace_back(handler); }
    template <typename... Args> void serveStatic(Args...) {}
    void begin() {}
    AsyncWebServerRequest request(const char *path, const char *payload, int method = HTTP_POST) {
        AsyncWebServerRequest req;
        req.path = path;
        req.verb = method;
        if (routes.count({path, method})) {
            routes.at({path, method})(&req);
            return req;
        }
        JsonDocument doc;
        deserializeJson(doc, payload);
        JsonVariant json = doc.as<JsonVariant>();
        for (auto &h : jsonHandlers)
            if (h->path == String(path)) {
                h->callback(&req, json);
                return req;
            }
        req.status = 404;
        return req;
    }
};
