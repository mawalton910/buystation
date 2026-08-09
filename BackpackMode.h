// BackpackMode.h - Backpack Mode Logic (Claim Loot)
#ifndef BACKPACKMODE_H
#define BACKPACKMODE_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"
#include "HttpUtils.h"
#include "BuyStation2.h"  // Reuse LootItem struct and helper functions

// External variables
extern String macAddress;
extern String serialNum;
extern String gameId;

// Backpack Mode state tracking (same structure as Buy Station 2)
String bpBadgeUuid = "";
String bpBadgeName = "";
String bpBadgeFaction = "";
int bpBackpackValue = 0;
int bpLootLimit = 1;  // Max items that can be claimed
LootItem bpLootItems[MAX_LOOT_CART_ITEMS];
int bpLootCount = 0;
int bpClaimedCount = 0;

// Session management
unsigned long bpLastActivityTime = 0;
const unsigned long BP_SESSION_TIMEOUT = 30000;  // 30 seconds inactivity timeout
extern unsigned long bs2SessionCheckTime;  // Shared session timer used in main loop

// Backpack mode specific
bool bpWaitingForConfirm = false;
int bpBrowsingIndex = 0;

// Forward declarations
void resetBackpackModeState();

// Claim all loot items. The old unlock step is intentionally not part of the
// successful player loot flow anymore; admin lock/unlock remains available.
int claimAndUnlockAllLoot() {
    int successCount = 0;
    
    // Determine how many items to claim (limited by bpLootLimit)
    int itemsToClaim = min(bpLootCount, bpLootLimit);
    
    Serial.println("=== Backpack Mode: Claiming Loot ===");
    Serial.println("Total available: " + String(bpLootCount));
    Serial.println("Loot limit: " + String(bpLootLimit));
    Serial.println("Items to claim: " + String(itemsToClaim));
    Serial.println("Badge UUID (bpBadgeUuid): '" + bpBadgeUuid + "'");
    Serial.println("Badge Name: '" + bpBadgeName + "'");
    
    // CRITICAL: Do NOT overwrite bs2BadgeUuid - claimLootItem uses it internally
    // Instead, we need to ensure bpBadgeUuid is passed correctly
    
    // Claim all loot items
    for (int i = 0; i < itemsToClaim; i++) {
        Serial.println("Claiming item " + String(i+1) + "/" + String(itemsToClaim) + ": " + bpLootItems[i].lootName);
        Serial.println("Using badge UUID: '" + bpBadgeUuid + "'");
        
        // Update display to show current progress
        displayBuyStation2Claiming(i + 1, itemsToClaim, bpLootItems[i].lootName);
        
        // Directly call the API instead of using claimLootItem (which uses bs2BadgeUuid)
        bool success = claimLootItemWithBadge(bpLootItems[i].lootId, bpLootItems[i].lootUuid, bpBadgeUuid, bpLootItems[i].rewardsJson);
        bpLootItems[i].claimed = success;
        
        if (success) {
            successCount++;
            bpClaimedCount++;
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

// Reset Backpack Mode state
void resetBackpackModeState() {
    bpBadgeUuid = "";
    bpBadgeName = "";
    bpBadgeFaction = "";
    bpBackpackValue = 0;
    bpLootLimit = 1;  // Reset to default (no backpack)
    bpLootCount = 0;
    bpClaimedCount = 0;
    bpWaitingForConfirm = false;
    bpBrowsingIndex = 0;
    bpLastActivityTime = 0;
    bs2SessionCheckTime = 0;
    
    for (int i = 0; i < MAX_LOOT_CART_ITEMS; i++) {
        bpLootItems[i].lootId = "";
        bpLootItems[i].lootUuid = "";
        bpLootItems[i].lootName = "";
        bpLootItems[i].locked = false;
        bpLootItems[i].claimed = false;

        // Backpack mode uses bs2 arrays as temporary decode buffers via checkWhatIsIt().
        // Clear them too so stale bs2 cart entries cannot trigger false limit checks.
        bs2LootItems[i].lootId = "";
        bs2LootItems[i].lootUuid = "";
        bs2LootItems[i].lootName = "";
        bs2LootItems[i].rewardText = "";
        bs2LootItems[i].rewardsJson = "";
        bs2LootItems[i].locked = false;
        bs2LootItems[i].claimed = false;
    }

    bs2LootCount = 0;
    bs2LootLimit = 1;
}

#endif // BACKPACKMODE_H
