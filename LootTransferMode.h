// LootTransferMode.h - Loot Transfer Mode Logic (Transfer inventory between factions)
#ifndef LOOTTRANSFERMODE_H
#define LOOTTRANSFERMODE_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"
#include "HttpUtils.h"

// External variables
extern String macAddress;
extern String serialNum;
extern String gameId;

// TransferInventoryItem struct is defined in M5DialControl.h

// Transaction state tracking
enum LootTransferState {
    LT_WAITING_FIRST_BADGE,      // Waiting for first commander badge scan
    LT_WAITING_SECOND_BADGE,     // Waiting for second commander badge scan
    LT_BROWSING_FACTION_A,       // Browsing faction A's inventory
    LT_SELECTING_AMOUNT_A,       // Selecting quantity from faction A
    LT_BROWSING_FACTION_B,       // Browsing faction B's inventory
    LT_SELECTING_AMOUNT_B,       // Selecting quantity from faction B
    LT_CHECKOUT_CONFIRM          // Confirming transaction
};

// Faction A (first scanned badge) variables
String ltFactionA_BadgeUuid = "";
String ltFactionA_BadgeName = "";
String ltFactionA_FactionName = "";
String ltFactionA_FactionId = "";
TransferInventoryItem ltFactionA_Items[20];
int ltFactionA_ItemCount = 0;
int ltFactionA_SelectedIndex = -1;
int ltFactionA_SelectedAmount = 0;

// Faction B (second scanned badge) variables
String ltFactionB_BadgeUuid = "";
String ltFactionB_BadgeName = "";
String ltFactionB_FactionName = "";
String ltFactionB_FactionId = "";
TransferInventoryItem ltFactionB_Items[20];
int ltFactionB_ItemCount = 0;
int ltFactionB_SelectedIndex = -1;
int ltFactionB_SelectedAmount = 0;

// Current state
LootTransferState ltCurrentState = LT_WAITING_FIRST_BADGE;
int ltBrowsingIndex = 0;
int ltTempAmount = 1;  // Temporary amount during selection

// Session management
unsigned long ltLastActivityTime = 0;
const unsigned long LT_SESSION_TIMEOUT = 120000;  // 120 seconds inactivity timeout

// Forward declarations
void resetLootTransferState();

// Fetch faction inventory from server and store in specified faction slot
bool fetchFactionInventory(String badgeUuid, bool isFactionA) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    String url = "https://iot.rke.world/iot/getFactionInventory";
    
    HTTPClient http;
    beginHttp(http, url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // 10 second timeout
    
    // Build request body
    String payload = "{";
    payload += "\"mac_address\":\"" + macAddress + "\",";
    payload += "\"serial_number\":\"" + serialNum + "\",";
    payload += "\"uuid\":\"" + badgeUuid + "\"";
    payload += "}";
    
    Serial.println("=== Fetch Faction Inventory ===");
    Serial.println("Faction: " + String(isFactionA ? "A (First)" : "B (Second)"));
    Serial.println("URL: " + url);
    Serial.println("Method: POST");
    Serial.println("Content-Type: application/json");
    Serial.println("Badge UUID: " + badgeUuid);
    Serial.println("Mac Address: " + macAddress);
    Serial.println("Serial Number: " + serialNum);
    Serial.println("Request Payload: " + payload);
    Serial.println("Payload Length: " + String(payload.length()));
    
    int httpCode = http.POST(payload);
    String response = http.getString();
    
    Serial.println("--- RESPONSE ---");
    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("Response Length: " + String(response.length()));
    Serial.println("Response Body: " + response);
    
    // Check for common HTTP errors
    if (httpCode == 404) {
        Serial.println("ERROR: Endpoint not found (404)");
        Serial.println("Double check the server has this route deployed");
    } else if (httpCode == 400) {
        Serial.println("ERROR: Bad Request (400) - Check request format");
    } else if (httpCode == 401 || httpCode == 403) {
        Serial.println("ERROR: Authentication/Authorization failed");
    } else if (httpCode < 0) {
        Serial.println("ERROR: Connection failed - " + http.errorToString(httpCode));
    }
    Serial.println("==============================");
    
    if (httpCode == 200 || httpCode == 201) {
        // Parse response
        DynamicJsonDocument doc(8192);
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            Serial.println("JSON parse error: " + String(error.c_str()));
            http.end();
            return false;
        }
        
        // Extract faction info
        // Get faction inventory
        JsonObject factionInventory = doc["factionInventory"];
        
        // Get the inventory items (with amounts)
        JsonArray inventoryItems = factionInventory["items"];
        
        // Get the full item details
        JsonArray itemDetails = doc["items"];
        
        int itemCount = 0;
        TransferInventoryItem* targetItems = isFactionA ? ltFactionA_Items : ltFactionB_Items;
        
        // Loop through inventory items and match with details
        for (JsonObject invItem : inventoryItems) {
            if (itemCount >= 20) break;  // Max 20 items
            
            String itemId = invItem["item"].as<String>();
            int amount = invItem["amount"].as<int>();
            
            // Skip items with 0 or negative amounts
            if (amount < 1) {
                Serial.println("Skipping item with amount < 1: " + itemId);
                continue;
            }
            
            // Find matching item details
            for (JsonObject detail : itemDetails) {
                String detailId = detail["_id"].as<String>();
                if (detailId == itemId) {
                    targetItems[itemCount].itemId = itemId;
                    targetItems[itemCount].itemName = detail["name"].as<String>();
                    targetItems[itemCount].amount = amount;
                    
                    // API now filters by Consumable/Ingredient keywords, so all items are transferable
                    targetItems[itemCount].canTransfer = true;
                    
                    Serial.println("Item " + String(itemCount) + ": " + 
                                 targetItems[itemCount].itemName + 
                                 " x" + String(targetItems[itemCount].amount));
                    
                    itemCount++;
                    break;
                }
            }
        }
        
        // Store item count (faction ID already set from whatIsIt response)
        if (isFactionA) {
            ltFactionA_ItemCount = itemCount;
        } else {
            ltFactionB_ItemCount = itemCount;
        }
        
        http.end();
        return true;
    }
    
    http.end();
    return false;
}

// Execute both transfers (A to B and B to A) using new endpoint
bool executeTransaction() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    bool success = true;
    
    // Build items array for A to B transfer
    String itemsAtoB = "[";
    if (ltFactionA_SelectedIndex >= 0 && ltFactionA_SelectedAmount > 0) {
        itemsAtoB += "{";
        itemsAtoB += "\"item_id\":\"" + ltFactionA_Items[ltFactionA_SelectedIndex].itemId + "\",";
        itemsAtoB += "\"amount\":" + String(ltFactionA_SelectedAmount);
        itemsAtoB += "}";
    }
    itemsAtoB += "]";
    
    // Build items array for B to A transfer
    String itemsBtoA = "[";
    if (ltFactionB_SelectedIndex >= 0 && ltFactionB_SelectedAmount > 0) {
        itemsBtoA += "{";
        itemsBtoA += "\"item_id\":\"" + ltFactionB_Items[ltFactionB_SelectedIndex].itemId + "\",";
        itemsBtoA += "\"amount\":" + String(ltFactionB_SelectedAmount);
        itemsBtoA += "}";
    }
    itemsBtoA += "]";
    
    // Transfer A to B (if selected)
    if (ltFactionA_SelectedIndex >= 0 && ltFactionA_SelectedAmount > 0) {
        Serial.println("=== Transfer A to B ===");
        
        HTTPClient http;
        beginHttp(http, "https://iot.rke.world/iot/transferFactionItems");
        configureHttpClient(http);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000);
        
        String payload = "{";
        payload += "\"mac_address\":\"" + macAddress + "\",";
        payload += "\"serial_number\":\"" + serialNum + "\",";
        payload += "\"sender_faction_id\":\"" + ltFactionA_FactionId + "\",";
        payload += "\"receiver_faction_id\":\"" + ltFactionB_FactionId + "\",";
        payload += "\"items\":" + itemsAtoB;
        payload += "}";
        
        Serial.println("Sender Faction: " + ltFactionA_FactionName + " (ID: " + ltFactionA_FactionId + ")");
        Serial.println("Receiver Faction: " + ltFactionB_FactionName + " (ID: " + ltFactionB_FactionId + ")");
        Serial.println("Item: " + ltFactionA_Items[ltFactionA_SelectedIndex].itemName);
        Serial.println("Amount: " + String(ltFactionA_SelectedAmount));
        Serial.println("Payload: " + payload);
        
        int httpCode = http.POST(payload);
        String response = http.getString();
        
        Serial.println("Response Code: " + String(httpCode));
        Serial.println("Response: " + response);
        
        if (httpCode != 200 && httpCode != 201) {
            success = false;
            Serial.println("Transfer A to B failed!");
        }
        http.end();
    }
    
    // Transfer B to A (if selected)
    if (ltFactionB_SelectedIndex >= 0 && ltFactionB_SelectedAmount > 0) {
        Serial.println("=== Transfer B to A ===");
        
        HTTPClient http;
        beginHttp(http, "https://iot.rke.world/iot/transferFactionItems");
        configureHttpClient(http);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000);
        
        String payload = "{";
        payload += "\"mac_address\":\"" + macAddress + "\",";
        payload += "\"serial_number\":\"" + serialNum + "\",";
        payload += "\"sender_faction_id\":\"" + ltFactionB_FactionId + "\",";
        payload += "\"receiver_faction_id\":\"" + ltFactionA_FactionId + "\",";
        payload += "\"items\":" + itemsBtoA;
        payload += "}";
        
        Serial.println("Sender Faction: " + ltFactionB_FactionName + " (ID: " + ltFactionB_FactionId + ")");
        Serial.println("Receiver Faction: " + ltFactionA_FactionName + " (ID: " + ltFactionA_FactionId + ")");
        Serial.println("Item: " + ltFactionB_Items[ltFactionB_SelectedIndex].itemName);
        Serial.println("Amount: " + String(ltFactionB_SelectedAmount));
        Serial.println("Payload: " + payload);
        
        int httpCode = http.POST(payload);
        String response = http.getString();
        
        Serial.println("Response Code: " + String(httpCode));
        Serial.println("Response: " + response);
        
        if (httpCode != 200 && httpCode != 201) {
            success = false;
            Serial.println("Transfer B to A failed!");
        }
        http.end();
    }
    
    Serial.println("=== Transaction Complete ===");
    Serial.println("Success: " + String(success ? "YES" : "NO"));
    
    return success;
}

// Reset all loot transfer state
void resetLootTransferState() {
    ltCurrentState = LT_WAITING_FIRST_BADGE;
    
    // Clear Faction A
    ltFactionA_BadgeUuid = "";
    ltFactionA_BadgeName = "";
    ltFactionA_FactionName = "";
    ltFactionA_FactionId = "";
    ltFactionA_ItemCount = 0;
    ltFactionA_SelectedIndex = -1;
    ltFactionA_SelectedAmount = 0;
    
    // Clear Faction B
    ltFactionB_BadgeUuid = "";
    ltFactionB_BadgeName = "";
    ltFactionB_FactionName = "";
    ltFactionB_FactionId = "";
    ltFactionB_ItemCount = 0;
    ltFactionB_SelectedIndex = -1;
    ltFactionB_SelectedAmount = 0;
    
    // Clear temp variables
    ltBrowsingIndex = 0;
    ltTempAmount = 1;
    ltLastActivityTime = millis();
    
    Serial.println("Loot Transfer state reset");
}

#endif // LOOTTRANSFERMODE_H
