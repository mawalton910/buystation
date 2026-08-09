#ifndef SECURE_BUYSTATION_CLIENT_H
#define SECURE_BUYSTATION_CLIENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include "config.h"
#include "BuyStationDeviceAuth.h"

class SecureBuyStationClient {
public:
    void begin();
    void loop();
    bool ready() const { return helloOk_; }
    String gameId() const { return serverGameId_; }
    String widgetId() const { return serverWidgetId_; }
    String iotId() const { return serverIotId_; }
    bool capabilitiesLoaded() const { return capabilitiesLoaded_; }
    bool supportsLootStation() const { return !capabilitiesLoaded_ || allowLootStation_; }
    bool supportsLootLock() const { return !capabilitiesLoaded_ || allowLootLock_; }
    bool supportsLootUnlock() const { return !capabilitiesLoaded_ || allowLootUnlock_; }
    bool supportsBadgeProgramming() const { return !capabilitiesLoaded_ || allowBadgeProgramming_; }
    String capabilitySummary() const;
    bool request(const char* action, JsonVariantConst body = JsonVariantConst());
    bool requestBlocking(const char* action, JsonVariantConst body, DynamicJsonDocument& response, uint32_t timeoutMs = SECURE_BUYSTATION_REQUEST_TIMEOUT_MS);
    void endLootSession(const String& sessionId);

private:
    static SecureBuyStationClient* active_;
    static void wsThunk(WStype_t type, uint8_t* payload, size_t length);
    void onWs(WStype_t type, uint8_t* payload, size_t length);
    void onText(const char* payload, size_t length);
    void sendHello();
    void resetServerState();
    void applyCapabilities(JsonVariantConst body);
    String requestId(const char* prefix);

    WebSocketsClient ws_;
    bool started_ = false;
    bool open_ = false;
    bool helloOk_ = false;
    String serverGameId_;
    String serverWidgetId_;
    String serverIotId_;
    bool capabilitiesLoaded_ = false;
    bool allowLootStation_ = false;
    bool allowLootLock_ = false;
    bool allowLootUnlock_ = false;
    bool allowBadgeProgramming_ = false;
    uint32_t sequence_ = 0;

    bool waiting_ = false;
    String waitingAction_;
    bool lastOk_ = false;
    int lastStatus_ = 0;
    DynamicJsonDocument* waitingResponse_ = nullptr;
};

SecureBuyStationClient* SecureBuyStationClient::active_ = nullptr;

void SecureBuyStationClient::begin() {
    if (started_) return;
    active_ = this;
    ws_.onEvent(wsThunk);
    ws_.setReconnectInterval(SECURE_BUYSTATION_RECONNECT_MS);
    ws_.beginSslWithCA(SECURE_BUYSTATION_WS_HOST, SECURE_BUYSTATION_WS_PORT, SECURE_BUYSTATION_WS_PATH, ROOT_CA_PEM);
    started_ = true;
}

void SecureBuyStationClient::loop() {
    if (started_) ws_.loop();
}

void SecureBuyStationClient::wsThunk(WStype_t type, uint8_t* payload, size_t length) {
    if (active_) active_->onWs(type, payload, length);
}

void SecureBuyStationClient::onWs(WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        open_ = true;
        resetServerState();
        BuyStationDeviceAuth::clearChallenge();
        Serial.println("[secure-ws] connected; waiting for auth challenge");
    } else if (type == WStype_DISCONNECTED) {
        open_ = false;
        resetServerState();
        BuyStationDeviceAuth::clearChallenge();
        Serial.println("[secure-ws] disconnected; retrying");
    } else if (type == WStype_ERROR) {
        Serial.println("[secure-ws] transport error");
    } else if (type == WStype_TEXT) {
        onText(reinterpret_cast<const char*>(payload), length);
    }
}

String SecureBuyStationClient::requestId(const char* prefix) {
    return String(prefix) + "-" + String(esp_random(), HEX) + "-" + String(++sequence_);
}

void SecureBuyStationClient::sendHello() {
    if (!open_ || !BuyStationDeviceAuth::ready()) return;

    StaticJsonDocument<1536> doc;
    doc["type"] = "hello";
    doc["reqId"] = requestId("hello");

    JsonObject body = doc["payload"].to<JsonObject>();
    String mac = configuredMacAddressString();
    mac.trim();
    if (!mac.length()) mac = WiFi.macAddress();
    const String serial = configuredSerialNumberString();
    const String configuredGame = configuredGameIdString();

    body["mac_address"] = mac;
    body["serial_number"] = serial;
    if (configuredGame.length()) body["game_id"] = configuredGame;
    body["ip_address"] = WiFi.localIP().toString();
    body["rssi"] = WiFi.RSSI();
    body["firmware_version"] = FIRMWARE_VERSION;
    body["role"] = "buy-station";
    body["game_updates"] = false;

    JsonArray capabilities = body["capabilities"].to<JsonArray>();
    capabilities.add("loot-station");
    capabilities.add("loot-lock");
    capabilities.add("loot-unlock");

    if (!BuyStationDeviceAuth::appendProof(body, mac, serial, FIRMWARE_VERSION)) {
        Serial.println("[secure-auth] failed to append device proof");
        return;
    }

    String output;
    serializeJson(doc, output);
    ws_.sendTXT(output);
}

void SecureBuyStationClient::onText(const char* payload, size_t length) {
    DynamicJsonDocument doc(16384);
    if (deserializeJson(doc, payload, length)) return;

    const String type = doc["type"] | "";
    if (type == "authChallenge") {
        BuyStationDeviceAuth::acceptChallenge(doc.as<JsonVariantConst>());
        if (!BuyStationDeviceAuth::ready()) {
            Serial.println("[secure-auth] invalid challenge or device credential shape");
            return;
        }
        Serial.println("[secure-auth] challenge accepted; sending signed hello");
        sendHello();
    } else if (type == "hello") {
        helloOk_ = doc["ok"] | false;
        if (helloOk_) {
            serverIotId_ = String(doc["iot"]["_id"] | "");
            serverWidgetId_ = String(doc["widget"]["_id"] | "");
            serverGameId_ = String(doc["widget"]["game"] | "");
            if (serverGameId_.length()) {
                ::gameId = serverGameId_;
            }
            Serial.printf("[secure-auth] device authenticated iot=%s widget=%s game=%s\n",
                          serverIotId_.length() ? serverIotId_.c_str() : "<none>",
                          serverWidgetId_.length() ? serverWidgetId_.c_str() : "<none>",
                          serverGameId_.length() ? serverGameId_.c_str() : "<none>");
        } else {
            const String error = doc["error"] | "";
            const char* reason = "server rejected hello";
            if (error.startsWith("IoT device not found")) reason = "device record not found";
            else if (error == "Unknown device authentication key") reason = "key ID does not match device record";
            else if (error == "Device authentication key is not provisioned") reason = "device record is not provisioned";
            else if (error == "Device authentication failed") reason = "device secret does not match device record";
            else if (error.startsWith("Widget not found")) reason = "credential accepted; attach device to a widget";
            else if (error.startsWith("Widget disabled")) reason = "credential accepted; assigned widget is disabled";
            else if (error.startsWith("Widget has no game assigned")) reason = "credential accepted; widget has no game";
            else if (error.startsWith("IoT assigned to multiple widgets")) reason = "credential accepted; multiple widget assignments";
            Serial.printf("[secure-auth] hello rejected: %s\n", reason);
        }
        if (helloOk_) request("capabilities");
    } else if (type == "universalDialResult") {
        const String action = doc["action"] | "";
        if (action == "capabilities") {
            applyCapabilities(doc["body"]);
        }
        if (waiting_ && waitingResponse_ && action == waitingAction_) {
            waitingResponse_->clear();
            (*waitingResponse_)["ok"] = doc["ok"] | false;
            (*waitingResponse_)["status"] = doc["status"] | 500;
            (*waitingResponse_)["body"].set(doc["body"]);
            lastOk_ = (*waitingResponse_)["ok"] | false;
            lastStatus_ = (*waitingResponse_)["status"] | 500;
            waiting_ = false;
        }
    }
}

bool SecureBuyStationClient::request(const char* action, JsonVariantConst body) {
    if (!helloOk_) return false;

    DynamicJsonDocument doc(4096);
    doc["type"] = "universalDial";
    doc["reqId"] = requestId(action);
    JsonObject payload = doc["payload"].to<JsonObject>();
    payload["action"] = action;
    if (!body.isNull()) payload["body"].set(body);

    String output;
    serializeJson(doc, output);
    return ws_.sendTXT(output);
}

void SecureBuyStationClient::resetServerState() {
    helloOk_ = false;
    serverGameId_ = "";
    serverWidgetId_ = "";
    serverIotId_ = "";
    capabilitiesLoaded_ = false;
    allowLootStation_ = false;
    allowLootLock_ = false;
    allowLootUnlock_ = false;
    allowBadgeProgramming_ = false;
}

void SecureBuyStationClient::applyCapabilities(JsonVariantConst body) {
    capabilitiesLoaded_ = true;
    allowLootStation_ = false;
    allowLootLock_ = false;
    allowLootUnlock_ = false;
    allowBadgeProgramming_ = false;

    JsonArrayConst modes = body["allowedModes"].as<JsonArrayConst>();
    for (JsonVariantConst value : modes) {
        String mode = String(value | "");
        mode.trim();
        mode.toLowerCase();
        if (mode == "loot-station") allowLootStation_ = true;
        else if (mode == "loot-lock") allowLootLock_ = true;
        else if (mode == "loot-unlock") allowLootUnlock_ = true;
        else if (mode == "badge-programmer") allowBadgeProgramming_ = true;
    }

    Serial.println("[secure-auth] capabilities: " + capabilitySummary());
}

String SecureBuyStationClient::capabilitySummary() const {
    if (!capabilitiesLoaded_) return "pending";
    String output = "";
    if (allowLootStation_) output += "loot";
    if (allowLootLock_) output += output.length() ? ",lock" : "lock";
    if (allowLootUnlock_) output += output.length() ? ",unlock" : "unlock";
    if (allowBadgeProgramming_) output += output.length() ? ",program" : "program";
    return output.length() ? output : "none";
}

bool SecureBuyStationClient::requestBlocking(const char* action, JsonVariantConst body, DynamicJsonDocument& response, uint32_t timeoutMs) {
    if (!helloOk_) {
        Serial.println("[secure-ws] request blocked: not authenticated");
        return false;
    }

    waiting_ = true;
    waitingAction_ = action;
    waitingResponse_ = &response;
    response.clear();

    if (!request(action, body)) {
        waiting_ = false;
        waitingResponse_ = nullptr;
        Serial.println("[secure-ws] send failed");
        return false;
    }

    const uint32_t startedAt = millis();
    while (waiting_ && static_cast<uint32_t>(millis() - startedAt) < timeoutMs) {
        loop();
        delay(2);
    }

    const bool completed = !waiting_;
    waiting_ = false;
    waitingResponse_ = nullptr;

    if (!completed) {
        Serial.printf("[secure-ws] timeout waiting for %s\n", action);
        return false;
    }

    return lastOk_ && lastStatus_ >= 200 && lastStatus_ < 300;
}

void SecureBuyStationClient::endLootSession(const String& sessionId) {
    if (!helloOk_ || !sessionId.length()) return;

    StaticJsonDocument<128> body;
    body["sessionId"] = sessionId;
    request("endLootSession", body.as<JsonVariantConst>());
}

#endif
