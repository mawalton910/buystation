// BuyStation2.h - Buy Station 2 Logic with Loot Backpack System
#ifndef BUYSTATION2_H
#define BUYSTATION2_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"
#include "M5DialControl.h"
#include "HttpUtils.h"
#include "WiFiManager.h"
#include "SecureBuyStationClient.h"
#include "FactionCache.h"
#include "LegacyFactions.h"

// External variables from relay2_M5.ino
extern String macAddress;
extern String serialNum;
extern String gameId;
extern OperationalMode deviceMode;
extern SecureBuyStationClient SecureWs;

// Player Class cache (loaded at startup)
struct PlayerClass {
    String id;           // _id from API
    String name;         // Class name (e.g., "Commander")
    String keyword;      // First keyword (e.g., "Leader")
    int multiplier;      // Calculated multiplier (3x for Leader, etc.)
};

// Global player class cache (populated at boot)
PlayerClass playerClassCache[20];  // Support up to 20 classes
int playerClassCount = 0;

// Item name cache (populated on first lookup, survives the session)
struct ItemNameEntry {
    String id;
    String name;
};
ItemNameEntry itemNameCache[40];  // Support up to 40 distinct item types
int itemNameCacheCount = 0;

// Resolve faction ID from whatIsIt data, with fallbacks to known IDs
String resolveFactionId(JsonObjectConst data) {
    String factionId = "";

    // Common fields
    if (data.containsKey("faction_id")) factionId = data["faction_id"].as<String>();
    if (factionId.length() == 0 && data.containsKey("faction")) {
        if (data["faction"].is<String>()) {
            factionId = data["faction"].as<String>();
        } else if (data["faction"].is<JsonObjectConst>()) {
            JsonObjectConst fObj = data["faction"].as<JsonObjectConst>();
            if (fObj.containsKey("_id")) factionId = fObj["_id"].as<String>();
            if (factionId.length() == 0 && fObj.containsKey("id")) factionId = fObj["id"].as<String>();
        }
    }

    // Name-based fallback mapping
    if (factionId.length() == 0) {
        String factionName = "";
        if (data.containsKey("faction_name")) factionName = data["faction_name"].as<String>();
        if (factionName.length() == 0 && data.containsKey("factionName")) factionName = data["factionName"].as<String>();
        factionId = resolveFactionIdByName(factionName);
    }

    return factionId;
}

// Buy Station 2 state tracking
struct LootItem {
    String lootId;
    String lootUuid;
    String lootName;
    String rewardText;  // Formatted reward text (e.g., "Contraband: 1\nMutant Credit: 10")
    String rewardsJson;  // JSON array of rewards with multiplied amounts for API
    bool locked;
    bool claimed;
};

// Global Buy Station 2 variables
String bs2BadgeUuid = "";
String bs2BadgeName = "";
String bs2BadgeFaction = "";
String bs2BadgeFactionId = "";  // Faction ID from whatIsIt response
String bs2PlayerClass = "";  // Player class keyword (e.g., "Leader", "Scout", etc.)
String bs2PlayerClassId = "";  // Player class _id from API (for serial/requests)
String bs2PlayerClassName = "";  // Player class name for display (e.g., "Faction Leader (test)")
int bs2ClassMultiplier = 1;  // Loot multiplier based on class (default 1x, Leader 3x, etc.)
int bs2BackpackValue = 0;  // Number of backpacks detected for the current badge
int bs2LootLimit = 1;  // Max cart slots (default 1, scales with backpack count)
LootItem bs2LootItems[MAX_LOOT_CART_ITEMS];
int bs2LootCount = 0;
int bs2ClaimedCount = 0;
String bs2SecureSessionId = "";
String bs2LastRejectedLootName = "";
String bs2LastRejectedRoundTimeLeft = "";

// Session management
unsigned long bs2LastActivityTime = 0;
const unsigned long BS2_SESSION_TIMEOUT = 30000;  // 30 seconds inactivity timeout
extern bool bs2WaitingForConfirm;  // From main sketch
extern int bs2BrowsingIndex;       // From main sketch
extern unsigned long bs2SessionCheckTime;  // From main sketch

// Badge cooldown to prevent back-to-back usage
String bs2LastUsedBadge = "";  // Cleared when a DIFFERENT badge completes a transaction
bool bs2DevMode = false;  // When true, allows same badge back-to-back for testing

// Forward declarations
void resetBuyStation2State();

String formatRoundSecondsRemaining(int secondsRemaining) {
    if (secondsRemaining < 0) return "";
    int minutes = secondsRemaining / 60;
    int seconds = secondsRemaining % 60;
    if (minutes > 0) {
        String value = String(minutes) + "m";
        if (seconds > 0) value += " " + String(seconds) + "s";
        return value + " left";
    }
    return String(seconds) + "s left";
}

String readRoundTimeLeftText(JsonObjectConst root, JsonObjectConst data) {
    int secondsRemaining = -1;

    if (root.containsKey("roundSecondsRemaining")) secondsRemaining = root["roundSecondsRemaining"].as<int>();
    if (secondsRemaining < 0 && root.containsKey("round_seconds_remaining")) secondsRemaining = root["round_seconds_remaining"].as<int>();
    if (secondsRemaining < 0 && root.containsKey("secondsRemaining")) secondsRemaining = root["secondsRemaining"].as<int>();
    if (secondsRemaining < 0 && root.containsKey("remainingSeconds")) secondsRemaining = root["remainingSeconds"].as<int>();
    if (secondsRemaining < 0 && data.containsKey("roundSecondsRemaining")) secondsRemaining = data["roundSecondsRemaining"].as<int>();
    if (secondsRemaining < 0 && data.containsKey("round_seconds_remaining")) secondsRemaining = data["round_seconds_remaining"].as<int>();
    if (secondsRemaining < 0 && data.containsKey("secondsRemaining")) secondsRemaining = data["secondsRemaining"].as<int>();
    if (secondsRemaining < 0 && data.containsKey("remainingSeconds")) secondsRemaining = data["remainingSeconds"].as<int>();

    String timeLeft = formatRoundSecondsRemaining(secondsRemaining);
    if (timeLeft.length()) return timeLeft;

    if (root.containsKey("roundTimeLeft")) return String(root["roundTimeLeft"] | "");
    if (root.containsKey("round_time_left")) return String(root["round_time_left"] | "");
    if (data.containsKey("roundTimeLeft")) return String(data["roundTimeLeft"] | "");
    if (data.containsKey("round_time_left")) return String(data["round_time_left"] | "");

    return "";
}

bool readLootClaimedThisRound(JsonObjectConst root, JsonObjectConst data) {
    int cacheClaimed = 0;

    if (root.containsKey("lootCacheClaimed")) cacheClaimed = root["lootCacheClaimed"].as<int>();
    if (cacheClaimed <= 0 && root.containsKey("loot_cache_claimed")) cacheClaimed = root["loot_cache_claimed"].as<int>();
    if (cacheClaimed <= 0 && root.containsKey("Loot Cache Claimed")) cacheClaimed = root["Loot Cache Claimed"].as<int>();
    if (cacheClaimed <= 0 && data.containsKey("lootCacheClaimed")) cacheClaimed = data["lootCacheClaimed"].as<int>();
    if (cacheClaimed <= 0 && data.containsKey("loot_cache_claimed")) cacheClaimed = data["loot_cache_claimed"].as<int>();
    if (cacheClaimed <= 0 && data.containsKey("Loot Cache Claimed")) cacheClaimed = data["Loot Cache Claimed"].as<int>();

    if (cacheClaimed > 0) return true;
    if (root["claimedThisRound"].as<bool>()) return true;
    if (root["claimed_this_round"].as<bool>()) return true;
    if (root["alreadyClaimedThisRound"].as<bool>()) return true;
    if (root["already_claimed_this_round"].as<bool>()) return true;
    if (data["claimedThisRound"].as<bool>()) return true;
    if (data["claimed_this_round"].as<bool>()) return true;
    if (data["alreadyClaimedThisRound"].as<bool>()) return true;
    if (data["already_claimed_this_round"].as<bool>()) return true;

    return false;
}

// Class keyword to multiplier mapping
int getMultiplierForClass(String classKeyword) {
    // All multipliers set to 1x - server-side issue prevents device-side multiplication
    return 1;
}

// Fetch ALL player classes for the game at startup - populates playerClassCache
bool fetchGamePlayerClasses() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    beginHttp(http, player_classes_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Build POST body with game ID, mac, serial (NO badge UUID)
    String payload = "{";
    payload += "\"gameId\":\"" + gameId + "\",";
    payload += "\"mac_address\":\"" + macAddress + "\",";
    payload += "\"serial_number\":\"" + serialNum + "\"";
    payload += "}";
    
    Serial.println("=== Fetch Game Player Classes ===");
    Serial.println("Payload: " + payload);
    
    int httpCode = http.POST(payload);
    String response = http.getString();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("=================================");
    
    bool success = false;
    
    if (httpCode == 200 || httpCode == 201) {
        DynamicJsonDocument doc(8192);  // Larger buffer for all classes
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            if (doc.containsKey("playerClasses") && doc["playerClasses"].is<JsonArray>()) {
                JsonArray classes = doc["playerClasses"].as<JsonArray>();
                
                playerClassCount = 0;
                
                // Iterate through all classes and store them
                for (JsonObject classObj : classes) {
                    if (playerClassCount >= 20) break;  // Array limit
                    
                    playerClassCache[playerClassCount].id = classObj["_id"].as<String>();
                    playerClassCache[playerClassCount].name = classObj["name"].as<String>();
                    
                    // Extract first keyword if available
                    if (classObj.containsKey("keywords") && classObj["keywords"].is<JsonArray>()) {
                        JsonArray keywords = classObj["keywords"].as<JsonArray>();
                        if (keywords.size() > 0) {
                            playerClassCache[playerClassCount].keyword = keywords[0].as<String>();
                        } else {
                            playerClassCache[playerClassCount].keyword = "";
                        }
                    } else {
                        playerClassCache[playerClassCount].keyword = "";
                    }
                    
                    // Calculate multiplier
                    playerClassCache[playerClassCount].multiplier = getMultiplierForClass(playerClassCache[playerClassCount].keyword);
                    
                    Serial.println("Loaded Class: " + playerClassCache[playerClassCount].name + 
                                   " (" + playerClassCache[playerClassCount].keyword + ") = " + 
                                   String(playerClassCache[playerClassCount].multiplier) + "x");
                    
                    playerClassCount++;
                }
                
                Serial.println("Total player classes loaded: " + String(playerClassCount));
                success = true;
            }
        } else {
            Serial.println("JSON parse error: " + String(error.c_str()));
        }
    } else {
        Serial.println("HTTP Error fetching player classes");
    }
    
    http.end();
    return success;
}

// Clear player class cache
void clearPlayerClassCache() {
    for (int i = 0; i < 20; i++) {
        playerClassCache[i].id = "";
        playerClassCache[i].name = "";
        playerClassCache[i].keyword = "";
        playerClassCache[i].multiplier = 1;
    }
    playerClassCount = 0;
    // Also clear item name cache
    for (int i = 0; i < 40; i++) {
        itemNameCache[i].id = "";
        itemNameCache[i].name = "";
    }
    itemNameCacheCount = 0;
    Serial.println("Player class + item name cache cleared");
}

// Lookup item name from item ID via API
String getItemName(String itemId) {
    // Check cache before making a network call
    for (int i = 0; i < itemNameCacheCount; i++) {
        if (itemNameCache[i].id == itemId) {
            Serial.println("Item name cache hit: " + itemId + " -> " + itemNameCache[i].name);
            return itemNameCache[i].name;
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return itemId;  // Return ID if can't lookup
    }
    
    HTTPClient http;
    String url = item_lookup_url;
    
    Serial.println("Looking up item name: " + url);
    
    beginHttp(http, url);
    configureHttpClient(http);
    http.setTimeout(3000);  // 3 second timeout for item lookup
    http.addHeader("Content-Type", "application/json");
    
    // Build JSON payload
    StaticJsonDocument<512> payload;
    payload["item_id"] = itemId;
    //payload["mac_address"] = macAddress;
    //payload["serial_number"] = serialNum;
    payload["mac"] = macAddress;
    payload["serial"] = serialNum;
    
    String jsonPayload;
    serializeJson(payload, jsonPayload);
    Serial.println("Payload: " + jsonPayload);
    
    int httpCode = http.POST(jsonPayload);
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Check if response has "item" wrapper or direct "name" field
            String itemName = "";
            if (doc.containsKey("item") && doc["item"].containsKey("name")) {
                itemName = doc["item"]["name"].as<String>();
            } else if (doc.containsKey("name")) {
                itemName = doc["name"].as<String>();
            }
            
            if (itemName.length() > 0) {
                Serial.println("Item name found: " + itemName);
                // Cache for future lookups
                if (itemNameCacheCount < 40) {
                    itemNameCache[itemNameCacheCount].id = itemId;
                    itemNameCache[itemNameCacheCount].name = itemName;
                    itemNameCacheCount++;
                }
                http.end();
                return itemName;
            } else {
                Serial.println("JSON response: " + payload);
            }
        } else {
            Serial.println("JSON parse error: " + String(error.c_str()));
        }
    } else {
        Serial.println("HTTP error code: " + String(httpCode));
        String errorResponse = http.getString();
        if (errorResponse.length() > 0) {
            Serial.println("Error response: " + errorResponse);
        }
    }
    
    Serial.println("Could not lookup item name, using ID");
    http.end();
    // Cache the fallback ID so we don't retry a broken/unknown item
    if (itemNameCacheCount < 40) {
        itemNameCache[itemNameCacheCount].id = itemId;
        itemNameCache[itemNameCacheCount].name = itemId;
        itemNameCacheCount++;
    }
    return itemId;  // Return ID if lookup fails
}

void restoreBuyStation2ReadyScreen() {
    if (bs2WaitingForConfirm && bs2LootCount > 0) {
        if (bs2BrowsingIndex >= bs2LootCount) bs2BrowsingIndex = bs2LootCount - 1;
        if (bs2BrowsingIndex < 0) bs2BrowsingIndex = 0;
        displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        return;
    }

    if (bs2BadgeUuid.length() > 0) {
        M5Dial.Display.fillScreen(BLACK);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.drawString("Slots: " + String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        return;
    }

    displayBuyStation2Scan();
}

String secureBuyStationStartPlayerSession(const String& uuid) {
    if (!SecureWs.ready()) {
        Serial.println("[secure-bs2] player session blocked: secure WS not authenticated");
        return "error";
    }
    if (!SecureWs.supportsLootStation()) {
        Serial.println("[secure-bs2] player session blocked: widget does not allow loot-station");
        return "error";
    }

    StaticJsonDocument<256> request;
    request["badgeUuid"] = uuid;

    DynamicJsonDocument response(8192);
    const bool ok = SecureWs.requestBlocking("beginLootSession", request.as<JsonVariantConst>(), response);
    JsonVariantConst body = response["body"];
    const int status = response["status"] | 0;
    Serial.printf("[secure-bs2] beginLootSession status=%d ok=%s\n", status, ok ? "true" : "false");

    if (!ok) {
        Serial.println("[secure-bs2] beginLootSession error: " + String(body["message"] | "Request denied"));
        return "error";
    }

    JsonObjectConst badge = body["badge"].as<JsonObjectConst>();
    bs2SecureSessionId = String(body["sessionId"] | "");
    bs2BadgeUuid = String(badge["uuid"] | uuid);
    bs2BadgeName = String(badge["callsign"] | "");
    if (!bs2BadgeName.length()) bs2BadgeName = String(badge["display_name"] | "");
    if (!bs2BadgeName.length()) bs2BadgeName = String(badge["profileName"] | "");
    if (!bs2BadgeName.length()) bs2BadgeName = String(badge["name"] | "Unknown Player");
    bs2BadgeFaction = String(badge["faction_name"] | "");
    if (!bs2BadgeFaction.length()) bs2BadgeFaction = String(badge["factionName"] | "");
    bs2BadgeFactionId = resolveFactionId(badge);
    if (!bs2BadgeFaction.length()) bs2BadgeFaction = displayFactionNameById(bs2BadgeFactionId);

    int capacity = body["capacity"] | 0;
    if (capacity <= 0) capacity = body["remaining"] | 1;
    bs2LootLimit = capacity > 0 ? capacity : 1;
    if (bs2LootLimit > MAX_LOOT_CART_ITEMS) bs2LootLimit = MAX_LOOT_CART_ITEMS;
    bs2BackpackValue = body["backpackCount"] | (bs2LootLimit > 1 ? 1 : 0);

    bs2PlayerClass = "";
    bs2PlayerClassId = "";
    bs2PlayerClassName = "";
    bs2ClassMultiplier = 1;

    playAcceptTone();
    return "badge";
}

String secureBuyStationIdentifyLoot(const String& uuid) {
    if (!SecureWs.ready()) {
        Serial.println("[secure-bs2] identify blocked: secure WS not authenticated");
        return "error";
    }
    if (!SecureWs.supportsLootStation()) {
        Serial.println("[secure-bs2] identify blocked: widget does not allow loot-station");
        return "error";
    }

    if (bs2BadgeUuid.length() == 0 || bs2SecureSessionId.length() == 0) {
        return "loot";
    }

    bool isDuplicate = false;
    int duplicateIndex = -1;
    for (int i = 0; i < bs2LootCount; i++) {
        if (bs2LootItems[i].lootUuid == uuid) {
            isDuplicate = true;
            duplicateIndex = i;
            break;
        }
    }

    if (isDuplicate) {
        if (duplicateIndex >= 0 && duplicateIndex < bs2LootCount) {
            const String removedName = bs2LootItems[duplicateIndex].lootName;
            for (int i = duplicateIndex; i < bs2LootCount - 1; i++) {
                bs2LootItems[i] = bs2LootItems[i + 1];
            }
            bs2LootCount--;
            bs2LootItems[bs2LootCount].lootId = "";
            bs2LootItems[bs2LootCount].lootUuid = "";
            bs2LootItems[bs2LootCount].lootName = "";
            bs2LootItems[bs2LootCount].rewardText = "";
            bs2LootItems[bs2LootCount].rewardsJson = "";
            bs2LootItems[bs2LootCount].locked = false;
            bs2LootItems[bs2LootCount].claimed = false;
            Serial.println("[secure-bs2] removed from cart: " + removedName);
            return "removed";
        }
        return "duplicate";
    }

    if (bs2LootCount >= bs2LootLimit || bs2LootCount >= MAX_LOOT_CART_ITEMS) {
        playErrorTone();
        return "limit";
    }

    StaticJsonDocument<256> request;
    request["uuid"] = uuid;

    DynamicJsonDocument response(8192);
    const bool ok = SecureWs.requestBlocking("identify", request.as<JsonVariantConst>(), response);
    JsonVariantConst body = response["body"];
    const int status = response["status"] | 0;
    Serial.printf("[secure-bs2] identify status=%d ok=%s\n", status, ok ? "true" : "false");

    if (!ok) {
        Serial.println("[secure-bs2] identify error: " + String(body["message"] | "Request denied"));
        return "error";
    }

    String result = String(body["type"] | "");
    result.toLowerCase();
    if (result != "loot") return result.length() ? result : "unknown";

    JsonObjectConst data = body["data"].as<JsonObjectConst>();
    String scannedLootName = String(data["name"] | "");
    if (!scannedLootName.length()) scannedLootName = String(data["item"] | "");
    if (!scannedLootName.length()) scannedLootName = "This loot";
    JsonObjectConst bodyObj = body.as<JsonObjectConst>();
    if (readLootClaimedThisRound(bodyObj, data)) {
        bs2LastRejectedLootName = scannedLootName;
        bs2LastRejectedRoundTimeLeft = readRoundTimeLeftText(bodyObj, data);
        Serial.println("[secure-bs2] loot already claimed for this round - not adding to cart");
        Serial.println("[secure-bs2] loot: " + bs2LastRejectedLootName);
        if (bs2LastRejectedRoundTimeLeft.length()) {
            Serial.println("[secure-bs2] round time left: " + bs2LastRejectedRoundTimeLeft);
        }
        playErrorTone();
        return "round_claimed";
    }

    bool locked = body["locked"] | false;
    if (!locked) locked = data["locked"] | false;
    if (locked) {
        Serial.println("[secure-bs2] loot locked");
        return "locked";
    }

    bs2LootItems[bs2LootCount].lootId = String(data["_id"] | "");
    if (!bs2LootItems[bs2LootCount].lootId.length()) bs2LootItems[bs2LootCount].lootId = String(data["id"] | "");
    bs2LootItems[bs2LootCount].lootUuid = uuid;
    bs2LootItems[bs2LootCount].lootName = scannedLootName;

    String rewardText = "";
    String rewardsJson = "[";
    bool firstReward = true;
    if (data.containsKey("rewards") && data["rewards"].is<JsonArrayConst>()) {
        for (JsonObjectConst reward : data["rewards"].as<JsonArrayConst>()) {
            String itemId = reward["item"].as<String>();
            int amount = reward["amount"].as<int>();
            const int multipliedAmount = amount * bs2ClassMultiplier;
            String rewardName = getItemName(itemId);
            if (rewardText.length() > 0) rewardText += "\n";
            rewardText += rewardName + ": " + String(multipliedAmount);
            if (!firstReward) rewardsJson += ",";
            rewardsJson += "{\"item\":\"" + itemId + "\",\"amount\":" + String(multipliedAmount) + "}";
            firstReward = false;
        }
    }
    rewardsJson += "]";

    bs2LootItems[bs2LootCount].rewardText = rewardText;
    bs2LootItems[bs2LootCount].rewardsJson = rewardsJson;
    bs2LootItems[bs2LootCount].locked = false;
    bs2LootItems[bs2LootCount].claimed = false;
    bs2LootCount++;

    playAcceptTone();
    return "loot";
}

// Check what type of badge was scanned - returns "badge", "loot", "removed", "duplicate", "locked", "round_claimed", "limit", "error", or "unknown"
String checkWhatIsIt(String uuid) {
    bs2LastRejectedLootName = "";
    bs2LastRejectedRoundTimeLeft = "";

#if USE_SECURE_BUYSTATION_WS
    if (deviceMode == MODE_BUY_STATION_2) {
        if (bs2BadgeUuid.length() == 0) {
            return secureBuyStationStartPlayerSession(uuid);
        }
        return secureBuyStationIdentifyLoot(uuid);
    }
#endif

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return "error";
    }
    
    HTTPClient http;
    beginHttp(http, what_is_it_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Build POST body with uuid, mac, serial
    String payload = "{";
    payload += "\"uuid\":\"" + uuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\"";
    payload += "}";
    
    Serial.println("=== What Is It Check ===");
    Serial.println("Payload: " + payload);
    
    int httpCode = http.POST(payload);
    String response = http.getString();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("========================");
    
    String result = "unknown";
    
    if (httpCode == 200 || httpCode == 201) {
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            if (doc.containsKey("type")) {
                result = doc["type"].as<String>();
                result.toLowerCase();
                if (result == "player") {
                    Serial.println("Badge type: player (normalized to badge)");
                    result = "badge"; // Treat player response as badge for player scans
                } else {
                    Serial.println("Badge type: " + result);
                }
                
                // If it's a badge, extract name and UUID
                if (result == "badge" && doc.containsKey("data")) {
                    JsonObject data = doc["data"];
                    bs2BadgeName = data["name"].as<String>();
                    if (bs2BadgeName.length() == 0) {
                        bs2BadgeName = "Unknown Player";
                    }
                    bs2BadgeUuid = data["uuid"].as<String>();
                    if (bs2BadgeUuid.length() == 0) {
                        bs2BadgeUuid = uuid;  // Fallback to scanned UUID
                    }
                    
                    // Extract faction ID (for loot transfer mode) with fallbacks
                    bs2BadgeFactionId = resolveFactionId(data);
                    bs2BadgeFaction = String(data["faction_name"] | "");
                    if (!bs2BadgeFaction.length()) bs2BadgeFaction = String(data["factionName"] | "");
                    if (!bs2BadgeFaction.length()) bs2BadgeFaction = displayFactionNameById(bs2BadgeFactionId);
                    
                    Serial.println("Badge Name: " + bs2BadgeName);
                    Serial.println("Badge UUID: " + bs2BadgeUuid);
                    Serial.println("Faction Name: " + (bs2BadgeFaction.length() ? bs2BadgeFaction : String("(unknown)")));
                    Serial.println("Faction ID: " + bs2BadgeFactionId);
                    
                    // Look up player class from cache using playerClass field from API
                    bs2PlayerClass = "";
                    bs2PlayerClassId = "";
                    bs2PlayerClassName = "";
                    bs2ClassMultiplier = 1;  // Default
                    
                    if (data.containsKey("playerClass") && !data["playerClass"].isNull()) {
                        // playerClass comes as an array from API
                        JsonArray playerClassArray = data["playerClass"].as<JsonArray>();
                        
                        if (playerClassArray.size() > 0) {
                            String playerClassId = playerClassArray[0].as<String>();
                            
                            // Search cache for matching class ID
                            for (int i = 0; i < playerClassCount; i++) {
                                if (playerClassCache[i].id == playerClassId) {
                                    bs2PlayerClassId = playerClassCache[i].id;  // Store ID for serial/requests
                                    bs2PlayerClassName = playerClassCache[i].name;  // Store name for display
                                    bs2PlayerClass = playerClassCache[i].keyword;  // Store keyword for logic
                                    bs2ClassMultiplier = playerClassCache[i].multiplier;
                                    Serial.println("Matched Player Class:");
                                    Serial.println("  ID: " + bs2PlayerClassId);
                                    Serial.println("  Name: " + bs2PlayerClassName);
                                    Serial.println("  Keyword: " + bs2PlayerClass);
                                    Serial.println("  Multiplier: " + String(bs2ClassMultiplier) + "x");
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (bs2PlayerClass.length() == 0) {
                        Serial.println("No player class assigned or found - using default 1x multiplier");
                    }
                    
                    // Determine backpack-driven slot count.
                    // New payload support (preferred): backpackCount + slotsPerBackpack.
                    // Backward compatible fallback: hasBackpack + globalBackpackLimit.
                    bool hasBackpack = doc["hasBackpack"].as<bool>();
                    int backpackCount = 0;
                    int slotsPerBackpack = 5;

                    if (doc.containsKey("backpackCount") && !doc["backpackCount"].isNull()) {
                        backpackCount = doc["backpackCount"].as<int>();
                    } else if (doc.containsKey("data") && doc["data"].is<JsonObject>()) {
                        JsonObject backpackData = doc["data"].as<JsonObject>();
                        if (backpackData.containsKey("backpackCount") && !backpackData["backpackCount"].isNull()) {
                            backpackCount = backpackData["backpackCount"].as<int>();
                        }
                    }

                    if (doc.containsKey("slotsPerBackpack") && !doc["slotsPerBackpack"].isNull()) {
                        slotsPerBackpack = doc["slotsPerBackpack"].as<int>();
                    } else if (doc.containsKey("globalBackpackLimit") && !doc["globalBackpackLimit"].isNull()) {
                        // Legacy response often used this as the total allowed slots for one backpack.
                        int legacySlots = doc["globalBackpackLimit"].as<int>();
                        if (legacySlots > 0) slotsPerBackpack = legacySlots;
                    }

                    if (slotsPerBackpack < 1) slotsPerBackpack = 5;

                    if (backpackCount <= 0 && hasBackpack) {
                        // Legacy behavior: if only hasBackpack=true is available, assume one backpack.
                        backpackCount = 1;
                    }

                    bs2BackpackValue = backpackCount;

                    if (backpackCount > 0) {
                        bs2LootLimit = backpackCount * slotsPerBackpack;
                        // Clamp to local firmware storage capacity.
                        if (bs2LootLimit > MAX_LOOT_CART_ITEMS) bs2LootLimit = MAX_LOOT_CART_ITEMS;
                    } else {
                        bs2LootLimit = 1;
                    }

                    Serial.println("Backpacks: " + String(bs2BackpackValue));
                    Serial.println("Slots per backpack: " + String(slotsPerBackpack));
                    Serial.println("Loot limit: " + String(bs2LootLimit));

                    // Accepted player badge
                    playAcceptTone();
                }
                // If it's loot, process duplicate toggle first, then enforce limits for new adds.
                else if (result == "loot" && doc.containsKey("data")) {
                    JsonObject data = doc["data"];
                    
                    String lootId = data["_id"].as<String>();
                    String scannedLootName = data["name"].as<String>();
                    if (scannedLootName.length() == 0) scannedLootName = data["item"].as<String>();
                    if (scannedLootName.length() == 0) scannedLootName = "This loot";
                    
                    // Check if this exact tag (by UUID) is already in the array
                    bool isDuplicate = false;
                    int duplicateIndex = -1;
                    for (int i = 0; i < bs2LootCount; i++) {
                        if (bs2LootItems[i].lootUuid == uuid) {
                            isDuplicate = true;
                            duplicateIndex = i;
                            Serial.println("Duplicate tag scanned - already in cart");
                            break;
                        }
                    }
                    
                    if (isDuplicate) {
                        // Toggle behavior: scanning the same loot UUID removes it from the cart.
                        if (duplicateIndex >= 0 && duplicateIndex < bs2LootCount) {
                            String removedName = bs2LootItems[duplicateIndex].lootName;

                            for (int i = duplicateIndex; i < bs2LootCount - 1; i++) {
                                bs2LootItems[i] = bs2LootItems[i + 1];
                            }

                            // Clear trailing slot after shift.
                            bs2LootItems[bs2LootCount - 1].lootId = "";
                            bs2LootItems[bs2LootCount - 1].lootUuid = "";
                            bs2LootItems[bs2LootCount - 1].lootName = "";
                            bs2LootItems[bs2LootCount - 1].rewardText = "";
                            bs2LootItems[bs2LootCount - 1].rewardsJson = "";
                            bs2LootItems[bs2LootCount - 1].locked = false;
                            bs2LootItems[bs2LootCount - 1].claimed = false;

                            bs2LootCount--;
                            Serial.println("Removed from cart: " + removedName + " (" + uuid + ")");
                            Serial.println("Total loot items: " + String(bs2LootCount));
                            result = "removed";
                        } else {
                            // Fallback: keep legacy result if index is invalid.
                            result = "duplicate";
                        }
                    } else {
                        JsonObjectConst rootConst = doc.as<JsonObjectConst>();
                        JsonObjectConst dataConst = doc["data"].as<JsonObjectConst>();
                        if (readLootClaimedThisRound(rootConst, dataConst)) {
                            bs2LastRejectedLootName = scannedLootName;
                            bs2LastRejectedRoundTimeLeft = readRoundTimeLeftText(rootConst, dataConst);
                            Serial.println("Loot already claimed for this round - not adding to cart");
                            Serial.println("Loot: " + bs2LastRejectedLootName);
                            if (bs2LastRejectedRoundTimeLeft.length()) {
                                Serial.println("Round time left: " + bs2LastRejectedRoundTimeLeft);
                            }
                            result = "round_claimed";
                            playErrorTone();
                            http.end();
                            return result;
                        }

                        // Enforce cart limits only for new additions.
                        if (bs2LootCount >= bs2LootLimit) {
                            Serial.println("Loot limit reached - not adding to cart");
                            result = "limit";
                            playErrorTone();
                            http.end();
                            return result;
                        }

                        // Hard cap for local array storage.
                        if (bs2LootCount >= MAX_LOOT_CART_ITEMS) {
                            Serial.println("Cart storage full (" + String(MAX_LOOT_CART_ITEMS) + ") - not adding to cart");
                            result = "limit";
                            playErrorTone();
                            http.end();
                            return result;
                        }

                        // Check if this specific UUID is locked in the uuids array
                        bool isLocked = false;
                        
                        // Check the top-level locked field first
                        isLocked = data["locked"].as<bool>();
                        
                        // Also check if this specific UUID is locked in the uuids array
                        if (!isLocked && data.containsKey("uuids")) {
                            JsonArray uuids = data["uuids"].as<JsonArray>();
                            for (JsonObject uuidObj : uuids) {
                                String uuidStr = uuidObj["uuid"].as<String>();
                                if (uuidStr.equalsIgnoreCase(uuid)) {
                                    isLocked = uuidObj["locked"].as<bool>();
                                    Serial.println("Found matching UUID in array, locked: " + String(isLocked));
                                    break;
                                }
                            }
                        }
                        
                        Serial.println("Final locked status: " + String(isLocked));
                        
                        if (isLocked) {
                            // Return "locked" so main loop can show locked message
                            Serial.println("Loot is LOCKED - not adding to cart");
                            result = "locked";
                        } else {
                            // Add to array
                            bs2LootItems[bs2LootCount].lootId = lootId;
                            
                            // ALWAYS use the scanned UUID from the physical tag
                            bs2LootItems[bs2LootCount].lootUuid = uuid;
                            
                            bs2LootItems[bs2LootCount].lootName = data["name"].as<String>();
                            if (bs2LootItems[bs2LootCount].lootName.length() == 0) {
                                bs2LootItems[bs2LootCount].lootName = data["item"].as<String>();
                            }
                            if (bs2LootItems[bs2LootCount].lootName.length() == 0) {
                                bs2LootItems[bs2LootCount].lootName = "Unknown Item";
                            }

                            // Accepted loot tag
                            playAcceptTone();
                            
                            // Parse and format rewards array
                            String rewardText = "";
                            String rewardsJson = "[";
                            bool firstReward = true;
                            
                            if (data.containsKey("rewards") && data["rewards"].is<JsonArray>()) {
                                JsonArray rewards = data["rewards"].as<JsonArray>();
                                for (JsonObject reward : rewards) {
                                    String itemId = reward["item"].as<String>();
                                    int amount = reward["amount"].as<int>();
                                    
                                    // Apply player class multiplier to amount
                                    int multipliedAmount = amount * bs2ClassMultiplier;
                                    
                                    // Lookup item name from ID
                                    String itemName = getItemName(itemId);
                                    
                                    // Build display text
                                    if (rewardText.length() > 0) {
                                        rewardText += "\n";
                                    }
                                    rewardText += itemName + ": " + String(multipliedAmount);
                                    
                                    // Build JSON array with multiplied amounts for claim API
                                    if (!firstReward) {
                                        rewardsJson += ",";
                                    }
                                    rewardsJson += "{\"item\":\"" + itemId + "\",\"amount\":" + String(multipliedAmount) + "}";
                                    firstReward = false;
                                    
                                    // Show multiplier in serial if not 1x
                                    if (bs2ClassMultiplier > 1) {
                                        Serial.println("  Reward: " + itemName + " x" + String(amount) + " * " + String(bs2ClassMultiplier) + " = " + String(multipliedAmount));
                                    }
                                }
                            }
                            rewardsJson += "]";
                            
                            bs2LootItems[bs2LootCount].rewardText = rewardText;
                            bs2LootItems[bs2LootCount].rewardsJson = rewardsJson;
                            
                            bs2LootItems[bs2LootCount].locked = false;  // We know it's unlocked at this point
                            bs2LootItems[bs2LootCount].claimed = false;
                            
                            Serial.println("Loot added: " + bs2LootItems[bs2LootCount].lootName);
                            Serial.println("Loot ID: " + bs2LootItems[bs2LootCount].lootId);
                            Serial.println("Loot UUID: " + bs2LootItems[bs2LootCount].lootUuid);
                            Serial.println("Rewards: " + bs2LootItems[bs2LootCount].rewardText);
                            
                            bs2LootCount++;
                            Serial.println("Total loot items: " + String(bs2LootCount));
                        }
                    }
                }
            }
        } else {
            Serial.println("JSON parse error");
        }
    } else {
        Serial.println("HTTP Error: " + String(httpCode));
        result = "error";  // Return error for HTTP failures
        playErrorTone();
    }
    
    http.end();
    return result;
}

// DEPRECATED: checkBadgeBackpack - No longer needed since whatIsIt provides all info
// Keeping function stub for compatibility but it does nothing
int checkBadgeBackpack(String badgeUuid) {
    Serial.println("checkBadgeBackpack DEPRECATED - data comes from whatIsIt");
    return 0;
}

// Claim single loot item - returns true if successful
bool claimLootItem(String lootId, String lootUuid, String rewardsJson) {
#if USE_SECURE_BUYSTATION_WS
    if (deviceMode == MODE_BUY_STATION_2) {
        if (!SecureWs.ready()) {
            Serial.println("[secure-bs2] claim blocked: secure WS not authenticated");
            return false;
        }
        if (!SecureWs.supportsLootStation()) {
            Serial.println("[secure-bs2] claim blocked: widget does not allow loot-station");
            return false;
        }
        if (!bs2SecureSessionId.length()) {
            Serial.println("[secure-bs2] claim blocked: no active loot session");
            return false;
        }

        StaticJsonDocument<256> request;
        request["sessionId"] = bs2SecureSessionId;
        request["lootUuid"] = lootUuid;

        DynamicJsonDocument response(8192);
        displayCenteredText(TEXT_SENDING, SENDING_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
        const bool ok = SecureWs.requestBlocking("claimLoot", request.as<JsonVariantConst>(), response, HTTP_REQUEST_TIMEOUT_MS + 3000UL);
        JsonVariantConst body = response["body"];
        const int status = response["status"] | 0;

        Serial.printf("[secure-bs2] claimLoot status=%d ok=%s loot=%s\n", status, ok ? "true" : "false", lootUuid.c_str());
        if (!ok) {
            Serial.println("[secure-bs2] claimLoot error: " + String(body["message"] | "Request denied"));
            return false;
        }

        Serial.println("[secure-bs2] loot claimed successfully");
        return true;
    }
#endif

    if (!ensureWiFiConnected()) {
        Serial.println("claimLootItem: WiFi not connected");
        return false;
    }
    if (!networkState.tlsReady) {
        Serial.println("claimLootItem: TLS not ready yet");
        return false;
    }
    
    HTTPClient http;
    beginHttp(http, buystation_transaction_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);
    
    // Build POST body with multiplied rewards
    String payload = "{";
    payload += "\"lootId\":\"" + lootId + "\",";
    payload += "\"lootUuid\":\"" + lootUuid + "\",";
    payload += "\"badgeUuid\":\"" + bs2BadgeUuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\",";
    payload += "\"rewards\":" + rewardsJson + ",";
    payload += "\"playerClassId\":\"" + bs2PlayerClassId + "\"";
    payload += "}";
    
    Serial.println("=== Claim Loot ===");
    Serial.println("Payload: " + payload);
    
    startNetworkSpinner("CLAIM");
    unsigned long startMs = markHttpStart();
    int httpCode = http.POST(payload);
    String response = http.getString();
    markHttpEnd(startMs);
    stopNetworkSpinner();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("==================");
    
    bool success = (httpCode == 200 || httpCode == 201);
    
    if (httpCode == -1) {
        Serial.println("Network timeout or connection error!");
    } else if (httpCode == 409) {
        Serial.println("Item already claimed or locked!");
    } else if (httpCode == 404) {
        Serial.println("Item not found!");
    } else if (success) {
        Serial.println("Loot claimed successfully!");
    } else {
        Serial.println("Loot claim failed with code: " + String(httpCode));
    }
    
    http.end();
    return success;
}

// Version of claimLootItem that accepts badge UUID as parameter (for Backpack mode)
bool claimLootItemWithBadge(String lootId, String lootUuid, String badgeUuid, String rewardsJson) {
#if USE_SECURE_BUYSTATION_WS
    if (deviceMode == MODE_BUY_STATION_2 && bs2SecureSessionId.length()) {
        return claimLootItem(lootId, lootUuid, rewardsJson);
    }
#endif

    if (!ensureWiFiConnected()) {
        Serial.println("claimLootItemWithBadge: WiFi not connected");
        return false;
    }
    if (!networkState.tlsReady) {
        Serial.println("claimLootItemWithBadge: TLS not ready yet");
        return false;
    }
    
    HTTPClient http;
    beginHttp(http, buystation_transaction_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);
    
    // Build POST body with multiplied rewards
    String payload = "{";
    payload += "\"lootId\":\"" + lootId + "\",";
    payload += "\"lootUuid\":\"" + lootUuid + "\",";
    payload += "\"badgeUuid\":\"" + badgeUuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\",";
    payload += "\"rewards\":" + rewardsJson + ",";
    payload += "\"playerClassId\":\"" + bs2PlayerClassId + "\"";
    payload += "}";
    
    Serial.println("=== Claim Loot ===");
    Serial.println("Payload: " + payload);
    
    startNetworkSpinner("CLAIM");
    unsigned long startMs = markHttpStart();
    int httpCode = http.POST(payload);
    String response = http.getString();
    markHttpEnd(startMs);
    stopNetworkSpinner();
    
    Serial.println("Response Code: " + String(httpCode));
    Serial.println("Response: " + response);
    Serial.println("==================");
    
    bool success = (httpCode == 200 || httpCode == 201);
    
    if (httpCode == -1) {
        Serial.println("Network timeout or connection error!");
    } else if (httpCode == 409) {
        Serial.println("Item already claimed or locked!");
    } else if (httpCode == 404) {
        Serial.println("Item not found!");
    } else if (success) {
        Serial.println("Loot claimed successfully!");
    } else {
        Serial.println("Loot claim failed with code: " + String(httpCode));
    }
    
    http.end();
    return success;
}

// Claim all loot items - returns number of successfully claimed items
int claimAllLoot() {
    int successCount = 0;
    
    // Determine how many items to claim (limited by bs2LootLimit)
    int itemsToClaim = min(bs2LootCount, bs2LootLimit);
    if (itemsToClaim <= 0) {
        Serial.println("No items to claim");
        return 0;
    }
    
    Serial.println("=== Claiming Loot ===");
    Serial.println("Total available: " + String(bs2LootCount));
    Serial.println("Loot limit: " + String(bs2LootLimit));
    Serial.println("Items to claim: " + String(itemsToClaim));
    
    for (int i = 0; i < itemsToClaim; i++) {
        Serial.println("Claiming item " + String(i+1) + "/" + String(itemsToClaim) + ": " + bs2LootItems[i].lootName);
        
        // Update display to show current progress
        displayBuyStation2Claiming(i + 1, itemsToClaim, bs2LootItems[i].lootName);
        
        bool success = claimLootItem(bs2LootItems[i].lootId, bs2LootItems[i].lootUuid, bs2LootItems[i].rewardsJson);
        bs2LootItems[i].claimed = success;
        
        if (success) {
            successCount++;
            bs2ClaimedCount++;
        }
        
        // Delay between claims to prevent overwhelming server
        delay(500);
    }
    
    Serial.println("=== Claiming Complete ===");
    Serial.println("Successfully claimed: " + String(successCount) + "/" + String(itemsToClaim));
    Serial.println("=========================");
    
    // Don't clear variables yet - results screen needs them
    // Variables will be cleared when returning to scan screen
    
    return successCount;
}

// Reset Buy Station 2 state
void resetBuyStation2State() {
#if USE_SECURE_BUYSTATION_WS
    if (bs2SecureSessionId.length()) {
        SecureWs.endLootSession(bs2SecureSessionId);
    }
#endif
    bs2SecureSessionId = "";
    bs2BadgeUuid = "";
    bs2BadgeName = "";
    bs2BadgeFaction = "";
    bs2PlayerClass = "";
    bs2PlayerClassId = "";
    bs2PlayerClassName = "";
    bs2ClassMultiplier = 1;
    bs2BackpackValue = 0;
    bs2LootLimit = 1;  // Reset to default (no backpack)
    bs2LootCount = 0;
    bs2ClaimedCount = 0;
    bs2WaitingForConfirm = false;
    bs2BrowsingIndex = 0;
    bs2LastActivityTime = 0;
    bs2SessionCheckTime = 0;
    
    for (int i = 0; i < MAX_LOOT_CART_ITEMS; i++) {
        bs2LootItems[i].lootId = "";
        bs2LootItems[i].lootUuid = "";
        bs2LootItems[i].lootName = "";
        bs2LootItems[i].rewardText = "";
        bs2LootItems[i].rewardsJson = "";
        bs2LootItems[i].locked = false;
        bs2LootItems[i].claimed = false;
    }
}

// Clear all non-critical variables (keeps device config, player class cache, and critical system vars)
void clearNonCriticalVariables() {
    Serial.println("=== Clearing Non-Critical Variables ===");
    
    // Clear Buy Station 2 session data
#if USE_SECURE_BUYSTATION_WS
    if (bs2SecureSessionId.length()) {
        SecureWs.endLootSession(bs2SecureSessionId);
    }
#endif
    bs2BadgeUuid = "";
    bs2BadgeName = "";
    bs2BadgeFaction = "";
    bs2BadgeFactionId = "";
    bs2PlayerClass = "";
    bs2PlayerClassId = "";
    bs2PlayerClassName = "";
    bs2ClassMultiplier = 1;
    bs2BackpackValue = 0;
    bs2LootLimit = 1;
    bs2LootCount = 0;
    bs2ClaimedCount = 0;
    bs2LastUsedBadge = "";  // Clear badge cooldown
    bs2SecureSessionId = "";
    
    // Clear loot items array
    for (int i = 0; i < MAX_LOOT_CART_ITEMS; i++) {
        bs2LootItems[i].lootId = "";
        bs2LootItems[i].lootUuid = "";
        bs2LootItems[i].lootName = "";
        bs2LootItems[i].rewardText = "";
        bs2LootItems[i].locked = false;
        bs2LootItems[i].claimed = false;
    }
    
    // Clear relay mode variables
    extern String relayLastUuid;
    extern String relayLastResponse;
    extern bool relayLastSuccess;
    relayLastUuid = "";
    relayLastResponse = "";
    relayLastSuccess = false;
    
    // Clear loot flip variables
    extern String lootFlipLastUuid;
    extern String lootFlipLastStatus;
    lootFlipLastUuid = "";
    lootFlipLastStatus = "";
    
    // Reset session timestamps
    extern unsigned long bs2LastActivityTime;
    bs2LastActivityTime = 0;
    
    Serial.println("Non-critical variables cleared");
    Serial.println("Preserved: Device config, Player class cache, WiFi settings");
    Serial.println("========================================");
}

#endif // BUYSTATION2_H
