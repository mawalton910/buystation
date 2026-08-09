// ApiClient.h - API/HTTP Client Functions
#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "secrets.h"
#include "config.h"
#include "ErrorHandler.h"
#include "M5DialControl.h"
#include "HttpUtils.h"
#include "WiFiManager.h"
#include "FactionCache.h"

// External variables
extern String macAddress;
extern String serialNum;
extern String gameId;
extern String playerName;
extern String playerFaction;
extern String itemName;
extern String itemType;
extern unsigned long lastTelemetrySentMs;

// Fetch factions for the game attached to this IoT device's widget.
// The server resolves game scope from mac_address + serial_number.
inline bool fetchGameFactions() {
    if (!ensureWiFiConnected()) {
        clearGameFactionCache("wifi offline");
        return false;
    }

    if (!networkState.tlsReady) {
        clearGameFactionCache("tls pending");
        return false;
    }

    HTTPClient http;
    beginHttp(http, game_init_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);

    StaticJsonDocument<256> payload;
    payload["mac_address"] = macAddress;
    payload["serial_number"] = serialNum;

    String jsonPayload;
    serializeJson(payload, jsonPayload);

    Serial.println("=== Fetch Game Factions ===");
    Serial.println("Request: " + jsonPayload);

    startNetworkSpinner("FACTIONS");
    unsigned long startMs = markHttpStart();
    int httpCode = http.POST(jsonPayload);
    String response = http.getString();
    markHttpEnd(startMs);
    stopNetworkSpinner();

    Serial.println("HTTP: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("===========================");

    if (httpCode != 200 && httpCode != 201) {
        clearGameFactionCache("http " + String(httpCode));
        http.end();
        return false;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        clearGameFactionCache("json error");
        Serial.println("Faction JSON parse error: " + String(error.c_str()));
        http.end();
        return false;
    }

    JsonArray factions = doc["factions"].as<JsonArray>();
    if (factions.isNull() || factions.size() == 0) {
        clearGameFactionCache("empty");
        http.end();
        return false;
    }

    clearGameFactionCache("loading");
    gameFactionCacheWidgetId = doc["widget"]["_id"] | "";
    String backendGameId = doc["widget"]["game"] | "";
    if (backendGameId.length()) {
        gameId = backendGameId;
    }
    gameFactionCachePoiId = doc["poi"]["_id"] | "";
    gameFactionCacheUpdatedAt = doc["serverTime"] | "";

    for (JsonObject faction : factions) {
        String keywords = "";
        if (faction["keywords"].is<JsonArray>()) {
            JsonArray kwArray = faction["keywords"].as<JsonArray>();
            for (JsonVariant kw : kwArray) {
                if (keywords.length()) keywords += ",";
                keywords += kw.as<String>();
            }
        }

        addGameFactionToCache(
            faction["_id"] | "",
            faction["name"] | "",
            faction["colorCode"] | "",
            keywords
        );
    }

    gameFactionCacheStatus = "loaded";
    Serial.println("Faction cache loaded: " + String(gameFactionCount));
    for (int i = 0; i < gameFactionCount; i++) {
        Serial.println("Faction[" + String(i) + "]: " + gameFactionCache[i].name + " " + gameFactionCache[i].id + " " + gameFactionCache[i].colorCode);
    }

    http.end();
    return gameFactionCount > 0;
}

// Send device telemetry/heartbeat to active minigame endpoint
inline bool sendActiveMinigameTelemetry(bool force = false) {
    unsigned long now = millis();
    if (!force && (now - lastTelemetrySentMs) < TELEMETRY_INTERVAL_MS) {
        return true; // Not time yet
    }

    if (!ensureWiFiConnected()) {
        return false;
    }

    if (!networkState.tlsReady) {
        Serial.println("TLS not ready yet - delaying telemetry");
        lastTelemetrySentMs = now; // wait until next interval
        return false;
    }

    HTTPClient http;
    beginHttp(http, active_minigame_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);

    StaticJsonDocument<512> payload;
    payload["mac_address"] = macAddress;
    payload["serial_number"] = serialNum;
    payload["gameId"] = gameId;
    payload["rssi"] = WiFi.RSSI();
    payload["timestamp"] = now;

    String jsonPayload;
    serializeJson(payload, jsonPayload);

    startNetworkSpinner("NET");
    unsigned long startMs = markHttpStart();
    int httpCode = http.POST(jsonPayload);
    String response = http.getString();
    markHttpEnd(startMs);
    stopNetworkSpinner();

    networkState.lastHeartbeatMs = now;
    networkState.apiStatus = String(httpCode);
    networkState.rssi = payload["rssi"].as<long>();

    if (TELEMETRY_DEBUG) {
        Serial.println("=== Heartbeat /getActiveMinigame ===");
        Serial.println("Payload: " + jsonPayload);
        Serial.println("HTTP: " + String(httpCode));
        Serial.println("RTT: " + String(networkState.lastRttMs) + " ms");
        Serial.println("Response: " + response);
        Serial.println("====================================");
    }

    http.end();
    lastTelemetrySentMs = now;

    return httpCode == 200 || httpCode == 201;
}

inline void serviceNetworkHealth() {
    // Keep WiFi alive and send periodic telemetry
    ensureWiFiConnected();
    sendActiveMinigameTelemetry(false);
}

// Send UUID to server in relay mode (simple update, no player/loot logic)
int sendRelayUpdate(String uuid, String modeName = "Relay") {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return 0;
    }
    
    HTTPClient http;
    beginHttp(http, update_uuid_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    
    String queryData = "{\"mac_address\":\"" + macAddress + "\", \"last_ip\":\"" + WiFi.localIP().toString() + "\", \"serial_number\":\"" + serialNum + "\", \"last_uuid\":\"" + uuid + "\"}";
    
    Serial.println("=== " + modeName + " Mode Update ===");
    Serial.println("UUID: " + uuid);
    Serial.println("Request: " + queryData);
    
    int httpCode = http.POST(queryData);
    String payload = http.getString();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + payload);
    Serial.println("========================");
    
    if (httpCode == 200 || httpCode == 201) {
        playSoftAcceptTone();
    }

    if (httpCode != 200 && httpCode != 201) {
        playSoftErrorTone();
    }

    http.end();
    return httpCode;
}

// Flip loot lock status by UUID - returns status string
String flipLootByUUID(String uuid) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return "WiFi Failed";
    }
    
    HTTPClient http;
    beginHttp(http, flip_loot_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Build payload with required fields
    String ipAddress = WiFi.localIP().toString();
    String payload = "{";
    payload += "\"mac_address\":\"" + macAddress + "\",";
    payload += "\"serial_number\":\"" + serialNum + "\",";
    payload += "\"last_uuid\":\"" + uuid + "\",";
    payload += "\"last_ip\":\"" + ipAddress + "\",";
    payload += "\"game_Id\":\"" + gameId + "\"";
    payload += "}";
    
    Serial.println("=== Flip Loot Request ===");
    Serial.println("Payload: " + payload);
    
    int httpCode = http.POST(payload);
    String response = http.getString();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("========================");
    
    String status = "Unknown";
    
    if (httpCode == 200 || httpCode == 201) {
        playSoftAcceptTone();
        // Parse JSON response to find the specific UUID's lock status
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            // Check for the uuids array in result
            if (doc.containsKey("result") && doc["result"].containsKey("uuids")) {
                JsonArray uuids = doc["result"]["uuids"];
                
                // Find matching UUID in the array
                for (JsonObject uuidObj : uuids) {
                    String uuidStr = uuidObj["uuid"].as<String>();
                    // Remove spaces and compare uppercase
                    uuidStr.replace(" ", "");
                    uuidStr.toUpperCase();
                    String compareUuid = uuid;
                    compareUuid.replace(" ", "");
                    compareUuid.toUpperCase();
                    
                    if (uuidStr == compareUuid) {
                        bool locked = uuidObj["locked"].as<bool>();
                        status = locked ? "LOCKED" : "UNLOCKED";
                        Serial.println("Found UUID in response. Status: " + status);
                        break;
                    }
                }
                
                // If we didn't find it, check the top-level locked field
                if (status == "Unknown" && doc["result"].containsKey("locked")) {
                    bool locked = doc["result"]["locked"].as<bool>();
                    status = locked ? "LOCKED" : "UNLOCKED";
                    Serial.println("Using top-level locked status: " + status);
                }
            }
        } else {
            Serial.println("JSON parse error: " + String(error.c_str()));
            // Fallback to text parsing
            int lockedIndex = response.indexOf("\"locked\"");
            if (lockedIndex >= 0) {
                int trueIndex = response.indexOf("true", lockedIndex);
                int falseIndex = response.indexOf("false", lockedIndex);
                if (trueIndex > lockedIndex && (falseIndex < 0 || trueIndex < falseIndex)) {
                    status = "LOCKED";
                } else if (falseIndex > lockedIndex) {
                    status = "UNLOCKED";
                }
            }
        }
        
        Serial.println("Final loot status: " + status);
    } else {
        playSoftErrorTone();
        // Error response
        if (response.indexOf("not found") >= 0 || response.indexOf("Not found") >= 0) {
            status = "NOT FOUND";
        } else {
            status = "ERROR " + String(httpCode);
        }
        Serial.println("Loot flip failed. HTTP code: " + String(httpCode));
    }
    
    http.end();
    return status;
}

// Query server for badge data (player or item) using whatIsIt
// Returns: 1 = success, 0 = error, -1 = WiFi retry (ORANGE faction)
int fetchBadgeDataOld(String uuid, bool isPlayer) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return 0;
    }
    
    HTTPClient http;
    beginHttp(http, what_is_it_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    
    // Send whatIsIt query with UUID
    String queryData = "{\"uuid\":\"" + uuid + "\", \"mac\":\"" + macAddress + "\", \"serial\":\"" + serialNum + "\"}";
    Serial.println("=== Sending whatIsIt Query ===");
    Serial.println("URL: " + String(what_is_it_url));
    Serial.println("Request: " + queryData);
    int httpCode = http.POST(queryData);
    
    Serial.println("Response Code: " + String(httpCode));
    
    if (httpCode == 200 || httpCode == 201) {
        String payload = http.getString();
        Serial.println("Response Body: " + payload);
        
        // Parse JSON response
        StaticJsonDocument<2048> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            String responseType = doc["type"].as<String>();
            responseType.toLowerCase();
            JsonObject data = doc["data"].as<JsonObject>();

            if (isPlayer) {
                if (responseType == "badge" || responseType == "player") {
                    playerFaction = data["faction"].as<String>();

                    // Check if faction is ORANGE - this is a WiFi retry trigger
                    if (playerFaction.equalsIgnoreCase("ORANGE")) {
                        Serial.println("ORANGE faction detected - triggering WiFi retry");
                        http.end();
                        return -1;  // Special return code for WiFi retry
                    }

                    playerName = data["name"].as<String>();
                    if (playerName.length() == 0) playerName = data["player"].as<String>();
                    if (playerName.length() == 0) playerName = doc["name"].as<String>();
                    if (playerName.length() == 0) playerName = TEXT_UNKNOWN_PLAYER;
                } else {
                    playerFaction = "";
                    playerName = TEXT_UNKNOWN_PLAYER;
                }
            } else {
                if (responseType == "loot") {
                    itemName = data["name"].as<String>();
                    if (itemName.length() == 0) itemName = data["item"].as<String>();
                    if (itemName.length() == 0) itemName = doc["name"].as<String>();
                    if (itemName.length() == 0) itemName = TEXT_UNKNOWN_ITEM;
                    itemType = responseType;
                } else {
                    itemName = TEXT_UNKNOWN_ITEM;
                    itemType = responseType.length() > 0 ? responseType : "none";
                }
            }
            http.end();
            return (isPlayer ? (responseType == "badge" || responseType == "player") : responseType == "loot") ? 1 : 0;
        }
    }
    
    if (httpCode != 200 && httpCode != 201) {
        String errorPayload = http.getString();
        Serial.println("Failed to fetch badge data: " + String(httpCode));
        Serial.println("Error Response: " + errorPayload);
        
        // Get appropriate error message and display it
        String displayMsg = getErrorDisplayMessage(errorPayload);
        http.end();
        displayErrorMessage(displayMsg);
        playSoftErrorTone();
        return 0;
    }
    
    // Check for successful response but with error message
    if (httpCode == 200 || httpCode == 201) {
        String payload = http.getString();
        
        // Check if response contains error message even with 200 status
        if (payload.indexOf("IOT device not found") >= 0 || 
            payload.indexOf("error") >= 0 || 
            payload.indexOf("Error") >= 0) {
            Serial.println("Error in response body: " + payload);
            String displayMsg = getErrorDisplayMessage(payload);
            http.end();
            displayErrorMessage(displayMsg);
            playSoftErrorTone();
            return 0;
        }
    }
    
    // If we get here, no valid data was found
    http.end();
    displayErrorMessage("NO DATA\\nFOUND");
    playSoftErrorTone();
    return 0;
}

// Wrapper function to maintain compatibility with existing code
int fetchBadgeData(String uuid, bool isPlayer) {
    return fetchBadgeDataOld(uuid, isPlayer);
}

// Send transaction to server (player + loot)
void sendTransactionRequest(String playerUuid, String lootUuid) {
    if (WiFi.status() == WL_CONNECTED) {
        // Buy-station flow uses loot claim route, never relay updateUUID/updateUUID2.
        HTTPClient http;

        // Step 1: Resolve lootId from scanned loot UUID via whatIsIt.
        beginHttp(http, what_is_it_url);
        configureHttpClient(http);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);

        String lookupPayload = "{";
        lookupPayload += "\"uuid\":\"" + lootUuid + "\",";
        lookupPayload += "\"mac\":\"" + macAddress + "\",";
        lookupPayload += "\"serial\":\"" + serialNum + "\"";
        lookupPayload += "}";

        Serial.println("=== Sending Transaction ===");
        Serial.println("Step 1 URL: " + String(what_is_it_url));
        Serial.println("Step 1 Request: " + lookupPayload);

        int lookupCode = http.POST(lookupPayload);
        String lookupBody = http.getString();
        Serial.println("Step 1 Response Code: " + String(lookupCode));
        Serial.println("Step 1 Response Body: " + lookupBody);
        http.end();

        String lootId = "";
        if (lookupCode == 200 || lookupCode == 201) {
            StaticJsonDocument<2048> lookupDoc;
            DeserializationError lookupErr = deserializeJson(lookupDoc, lookupBody);
            if (!lookupErr) {
                String responseType = lookupDoc["type"].as<String>();
                responseType.toLowerCase();
                if (responseType == "loot") {
                    lootId = lookupDoc["data"]["_id"].as<String>();
                }
            }
        }

        if (lootId.length() == 0) {
            Serial.println("Transaction aborted: could not resolve lootId for lootUuid " + lootUuid);
            displayCenteredText(String(TEXT_FAILED) + "LOOT", FAIL_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
            delay(FAIL_MESSAGE_DELAY);
            playSoftErrorTone();
            return;
        }

        // Step 2: Claim loot using buy-station contract.
        beginHttp(http, buystation_transaction_url);
        configureHttpClient(http);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);

        String transactionData = "{";
        transactionData += "\"lootId\":\"" + lootId + "\",";
        transactionData += "\"lootUuid\":\"" + lootUuid + "\",";
        transactionData += "\"badgeUuid\":\"" + playerUuid + "\",";
        transactionData += "\"mac\":\"" + macAddress + "\",";
        transactionData += "\"serial\":\"" + serialNum + "\"";
        transactionData += "}";

        Serial.println("Step 2 URL: " + String(buystation_transaction_url));
        Serial.println("Step 2 Request: " + transactionData);

        int httpCode = http.POST(transactionData);
        String payload = http.getString();
        Serial.println("Step 2 Response Code: " + String(httpCode));
        Serial.println("Step 2 Response Body: " + payload);
        Serial.println("============================");
        http.end();
        
        if (httpCode == 200 || httpCode == 201) {
            playSoftAcceptTone();
            displayCenteredText(TEXT_SUCCESS, SUCCESS_TEXT_COLOR, PROMPT_BG_COLOR, 3);
            delay(SUCCESS_MESSAGE_DELAY);
        } else {
            String friendlyError = getErrorDisplayMessage(payload);
            if (httpCode == 409 && payload.indexOf("already claimed") >= 0) {
                friendlyError = "GREEDY!\nALREADY\nCLAIMED";
            }

            if (friendlyError != "SERVER\nERROR") {
                displayErrorMessage(friendlyError);
            } else {
                displayCenteredText(String(TEXT_FAILED) + String(httpCode), FAIL_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
                delay(FAIL_MESSAGE_DELAY);
            }
            playSoftErrorTone();
        }
    } else {
        Serial.println("Error in WiFi connection");
        displayCenteredText(TEXT_WIFI_NOT_CONNECTED, FAIL_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
        delay(FAIL_MESSAGE_DELAY);
        playSoftErrorTone();
    }
}

// Backpack Mode - Claim loot.
// Returns: "SUCCESS", "NOT FOUND", "ALREADY CLAIMED", or "ERROR"
String claimAndLockLoot(String lootUuid) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return "ERROR";
    }
    
    Serial.println("=== Backpack Mode: Claim Loot ===");
    Serial.println("Loot UUID: " + lootUuid);
    
    // Step 1: Get loot info using whatIsIt endpoint
    HTTPClient http;
    beginHttp(http, what_is_it_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    String payload = "{";
    payload += "\"uuid\":\"" + lootUuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\"";
    payload += "}";
    
    Serial.println("Step 1: Getting loot info...");
    int httpCode = http.POST(payload);
    String response = http.getString();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    
    if (httpCode != 200) {
        Serial.println("Failed to get loot info");
        playSoftErrorTone();
        http.end();
        return "NOT FOUND";
    }
    
    // Parse response
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.println("JSON parse error");
        http.end();
        return "ERROR";
    }
    
    String type = doc["type"].as<String>();
    if (type != "loot") {
        Serial.println("Not a loot item: " + type);
        http.end();
        return "NOT FOUND";
    }
    
    // Check if already locked/claimed
    bool locked = doc["data"]["locked"].as<bool>();
    if (locked) {
        playSoftErrorTone();
        Serial.println("Loot already claimed/locked");
        http.end();
        return "ALREADY CLAIMED";
    }
    
    // Get loot ID and badge ID
    String lootId = doc["data"]["_id"].as<String>();
    String badgeUuid = doc["data"]["badge_id"].as<String>();
    
    if (lootId.length() == 0 || badgeUuid.length() == 0) {
        Serial.println("Missing loot ID or badge ID");
        http.end();
        return "ERROR";
    }
    
    http.end();
    
    // Step 2: Claim the loot
    Serial.println("Step 2: Claiming loot...");
    beginHttp(http, buystation_transaction_url);
    configureHttpClient(http);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    
    payload = "{";
    payload += "\"lootId\":\"" + lootId + "\",";
    payload += "\"lootUuid\":\"" + lootUuid + "\",";
    payload += "\"badgeUuid\":\"" + badgeUuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\"";
    payload += "}";
    
    Serial.println("Claim Payload: " + payload);
    httpCode = http.POST(payload);
    response = http.getString();
    
    Serial.println("Claim Response Code: " + String(httpCode));
    Serial.println("Claim Response: " + response);
    
    if (httpCode == 200 || httpCode == 201) {
        playSoftAcceptTone();
        Serial.println("=== Claim Complete ===");
        return "SUCCESS";
    } else {
        Serial.println("Claim failed");
        playSoftErrorTone();
        return "ERROR";
    }
}

#endif // API_CLIENT_H
