# Buy Station Flow - Complete Endpoint Guide

This document explains **exactly** how the Buy Station 2 works, step-by-step, with all API endpoints, when they're called, and what they return.

---

## 🎯 Overview

The Buy Station allows players to:
1. **Scan their player badge** to start a session
2. **Scan loot badges** to add items to their cart (up to their backpack limit)
3. **Press the button** to claim all loot in one transaction

---

## 📋 Complete Transaction Flow

### **STEP 1: Player Scans Their Badge**

**When**: Player taps their badge to start shopping  
**Endpoint**: `POST https://iot.rke.world/loot/whatIsIt`  

**Request Body**:
```json
{
  "uuid": "041774D75F6180",      // Scanned badge UUID
  "mac": "24:58:7C:5C:60:AC",    // Device MAC address
  "serial": "gurugamesbuy2"      // Device serial number
}
```

**Response** (Success - 200):
```json
{
  "type": "badge",
  "data": {
    "uuid": "041774D75F6180",
    "name": "John Doe",           // ← Player name extracted here
    "faction": "66a03bb56e7e39f9b4e28c5a"  // ← Faction ID
  },
  "hasBackpack": true,            // ← Does player have backpack?
  "globalBackpackLimit": 5        // ← Max items they can claim
}
```

**What Happens**:
- ✅ Device stores player name: `bs2BadgeName = "John Doe"`
- ✅ Device stores badge UUID: `bs2BadgeUuid = "041774D75F6180"`
- ✅ Device stores faction: `bs2BadgeFactionId = "66a03bb56e7e39f9b4e28c5a"`
- ✅ Sets loot limit: `bs2LootLimit = 5` (if backpack) or `1` (no backpack)
- ✅ Display shows: "JOHN DOE - SCAN LOOT"
- ✅ Session timer starts (30 seconds timeout)

**Possible Errors**:
- `type: "error"` - Badge not found in system
- HTTP 404/500 - Server error

---

### **STEP 2: Player Scans First Loot Badge**

**When**: Player taps a loot badge to add to cart  
**Endpoint**: `POST https://iot.rke.world/loot/whatIsIt`  

**Request Body**:
```json
{
  "uuid": "DA9E5CC1",            // Scanned loot tag UUID
  "mac": "24:58:7C:5C:60:AC",
  "serial": "gurugamesbuy2"
}
```

**Response** (Success - 200):
```json
{
  "type": "loot",
  "data": {
    "_id": "679c8e6aa4db0e57ab234567",  // ← Database ID (used for claiming)
    "uuid": "DA9E5CC1",
    "name": "Supply Crate",              // ← Item name
    "locked": false,                     // ← Is this loot locked?
    "rewards": [                         // ← What's inside this loot
      {
        "item": "673c1234abcd1234ef567890",  // Item ID
        "amount": 1
      },
      {
        "item": "673c5678dcba5678fe098765",
        "amount": 10
      }
    ],
    "uuids": [                          // Individual tag lock status
      {
        "uuid": "DA9E5CC1",
        "locked": false
      }
    ]
  }
}
```

**What Happens**:
- ✅ Device checks if loot is **locked** → if yes, show "LOCKED" and reject
- ✅ Device checks if loot is **duplicate** → if yes, show "ALREADY IN CART"
- ✅ Device adds to cart array: `bs2LootItems[0]`
- ✅ Device looks up reward item names by calling **item lookup** (see below)
- ✅ Display shows: "1/5 ITEMS - SCAN MORE"
- ✅ Session timer resets to 30 seconds

**Sub-Request: Lookup Item Names**  
For each reward in the `rewards` array, device calls:

**Endpoint**: `GET https://iot.rke.world/item/getItemById/{itemId}`  
Example: `GET https://iot.rke.world/item/getItemById/673c1234abcd1234ef567890`

**Response**:
```json
{
  "name": "Contraband"     // ← Item name extracted here
}
```

**Final Reward Text Built**:
```
Contraband: 1
Mutant Credit: 10
```

**Possible Errors**:
- `type: "locked"` - Loot is locked, cannot claim
- `type: "duplicate"` - Already scanned this exact tag
- HTTP 404 - Loot not found

---

### **STEP 3: Player Scans More Loot (Optional)**

**When**: Player keeps scanning loot badges (up to limit)  
**Endpoint**: Same as Step 2 - `POST https://iot.rke.world/loot/whatIsIt`

**What Happens**:
- ✅ Each scan adds another item to `bs2LootItems` array
- ✅ Display updates: "2/5 ITEMS - SCAN MORE"
- ✅ Session timer resets on each scan
- ✅ When limit reached: "5/5 ITEMS - PRESS TO CLAIM"

**Device State After Scanning 3 Items**:
```cpp
bs2BadgeUuid = "041774D75F6180"
bs2BadgeName = "John Doe"
bs2LootLimit = 5
bs2LootCount = 3
bs2LootItems[0] = {
  lootId: "679c8e6aa4db0e57ab234567",
  lootUuid: "DA9E5CC1",
  lootName: "Supply Crate",
  rewardText: "Contraband: 1\nMutant Credit: 10"
}
bs2LootItems[1] = { ... }
bs2LootItems[2] = { ... }
```

---

### **STEP 4: Player Presses Button to Claim All**

**When**: Player presses the M5Dial button to complete transaction  
**Endpoint**: `POST https://iot.rke.world/loot/claimLoot` (called for EACH item)

**Request Body** (Claim #1):
```json
{
  "lootId": "679c8e6aa4db0e57ab234567",    // Database _id from whatIsIt
  "lootUuid": "DA9E5CC1",                  // Physical tag UUID
  "badgeUuid": "041774D75F6180",           // Player badge UUID
  "mac": "24:58:7C:5C:60:AC",
  "serial": "gurugamesbuy2"
}
```

**Response** (Success - 200/201):
```json
{
  "success": true,
  "message": "Loot claimed successfully"
}
```

**What Happens**:
1. Display shows: "CLAIMING 1/3: Supply Crate"
2. Device sends claim request for item #1
3. Waits 500ms
4. Display shows: "CLAIMING 2/3: Medkit"
5. Device sends claim request for item #2
6. Waits 500ms
7. Display shows: "CLAIMING 3/3: Ammo Box"
8. Device sends claim request for item #3
9. Display shows: "SUCCESS: 3/3 CLAIMED"
10. Session ends, state clears
11. Display shows: "SCAN BADGE"

**Possible Errors Per Item**:
- HTTP 409 - Item already claimed or locked
- HTTP 404 - Item not found
- HTTP -1 - Network timeout
- HTTP 500 - Server error

**Partial Success Example**:
- Item 1: ✅ Success (200)
- Item 2: ❌ Failed (409 - already claimed)
- Item 3: ✅ Success (200)
- **Result**: "SUCCESS: 2/3 CLAIMED"

---

## 🔄 Complete API Sequence Diagram

```
Player                    Device                           Server
  |                         |                                |
  |---[Scan Badge]--------->|                                |
  |                         |---POST /loot/whatIsIt--------->|
  |                         |   { uuid: "041774D75F6180" }   |
  |                         |                                |
  |                         |<--200 OK-----------------------|
  |                         |   { type: "badge",            |
  |                         |     name: "John Doe",         |
  |                         |     hasBackpack: true,        |
  |                         |     globalBackpackLimit: 5 }  |
  |                         |                                |
  |<--[Display: JOHN DOE]---|                                |
  |                         |                                |
  |---[Scan Loot #1]------->|                                |
  |                         |---POST /loot/whatIsIt--------->|
  |                         |   { uuid: "DA9E5CC1" }        |
  |                         |                                |
  |                         |<--200 OK-----------------------|
  |                         |   { type: "loot",             |
  |                         |     data: { _id, name, ... }} |
  |                         |                                |
  |                         |---GET /item/getItemById/xxx--->| (for each reward)
  |                         |<--200 OK-----------------------|
  |                         |   { name: "Contraband" }      |
  |                         |                                |
  |<--[Display: 1/5]--------|                                |
  |                         |                                |
  |---[Scan Loot #2]------->| (repeat whatIsIt)              |
  |---[Scan Loot #3]------->| (repeat whatIsIt)              |
  |                         |                                |
  |<--[Display: 3/5]--------|                                |
  |                         |                                |
  |---[Press Button]------->|                                |
  |                         |---POST /loot/claimLoot-------->|
  |                         |   { lootId, lootUuid, badge }  |
  |                         |<--200 OK-----------------------|
  |                         |                                |
  |                         |---POST /loot/claimLoot-------->| (item 2)
  |                         |<--200 OK-----------------------|
  |                         |                                |
  |                         |---POST /loot/claimLoot-------->| (item 3)
  |                         |<--200 OK-----------------------|
  |                         |                                |
  |<--[Display: 3/3 OK]-----|                                |
  |<--[Display: SCAN]-------|                                |
```

---

## 🛡️ Anti-Cheat / Session Rules

### **Badge Cooldown**
- After claiming, that badge is **blocked** until a **different** badge completes a transaction
- Prevents same player from immediately claiming again
- Example:
  - Player A claims → `bs2LastUsedBadge = "041774D75F6180"`
  - Player A tries again → ❌ "WAIT FOR NEXT PLAYER"
  - Player B claims → ✅ Allowed, cooldown cleared
  - Player A tries again → ✅ Now allowed

### **Session Timeout**
- 30 seconds of inactivity = session ends
- Timer resets on every badge scan or button press
- On timeout: Display shows "SESSION TIMEOUT", then clears

### **Backpack System**
- No backpack: `bs2LootLimit = 1` (only 1 item)
- With backpack: `bs2LootLimit = globalBackpackLimit` (e.g., 5 items)
- Only claims up to the limit, even if more scanned

---

## 📡 All API Endpoints Summary

| **Endpoint** | **Method** | **Purpose** | **When Called** |
|-------------|-----------|------------|----------------|
| `/loot/whatIsIt` | POST | Check if badge/loot, get details | Every badge scan |
| `/item/getItemById/{id}` | GET | Get item name from ID | After loot scan (for rewards) |
| `/loot/claimLoot` | POST | Claim single loot item | Button press (once per item) |

---

## 🧪 Testing Your Routes

### **Test 1: Check Player Badge**
```bash
curl -X POST https://iot.rke.world/loot/whatIsIt \
  -H "Content-Type: application/json" \
  -d '{
    "uuid": "041774D75F6180",
    "mac": "24:58:7C:5C:60:AC",
    "serial": "gurugamesbuy2"
  }'
```

**Expected**: `"type": "badge"` with player name and backpack status

---

### **Test 2: Check Loot Badge**
```bash
curl -X POST https://iot.rke.world/loot/whatIsIt \
  -H "Content-Type: application/json" \
  -d '{
    "uuid": "DA9E5CC1",
    "mac": "24:58:7C:5C:60:AC",
    "serial": "gurugamesbuy2"
  }'
```

**Expected**: `"type": "loot"` with item details and rewards array

---

### **Test 3: Lookup Item Name**
```bash
curl -X GET https://iot.rke.world/item/getItemById/673c1234abcd1234ef567890
```

**Expected**: `{ "name": "Contraband" }`

---

### **Test 4: Claim Loot**
```bash
curl -X POST https://iot.rke.world/loot/claimLoot \
  -H "Content-Type: application/json" \
  -d '{
    "lootId": "679c8e6aa4db0e57ab234567",
    "lootUuid": "DA9E5CC1",
    "badgeUuid": "041774D75F6180",
    "mac": "24:58:7C:5C:60:AC",
    "serial": "gurugamesbuy2"
  }'
```

**Expected**: `200 OK` with success message

---

## 🐛 Common Issues & Fixes

| **Problem** | **Cause** | **Solution** |
|-----------|---------|-----------|
| "LOCKED" message | Loot `locked: true` in API | Unlock loot in admin or via API |
| "ALREADY IN CART" | Same UUID scanned twice | Normal behavior, scan different loot |
| "WAIT FOR NEXT PLAYER" | Badge cooldown active | Have different player use station first |
| Session timeout | 30s inactivity | Scan badge again to restart |
| Some items fail to claim | 409 conflict (already claimed) | Item was claimed by another station |
| Network timeout | WiFi or server issue | Check WiFi connection, retry |

---

## 💡 Dev Mode (Testing)

Set `bs2DevMode = true` in code to **disable badge cooldown** for testing (allows same badge back-to-back).

**Location**: [BuyStation2.h](BuyStation2.h#L32)

---

## 📝 Key Variables in Code

| **Variable** | **Purpose** | **Example Value** |
|------------|-----------|-----------------|
| `bs2BadgeUuid` | Current player's badge UUID | `"041774D75F6180"` |
| `bs2BadgeName` | Current player's name | `"John Doe"` |
| `bs2LootLimit` | Max items player can claim | `5` (with backpack) or `1` |
| `bs2LootCount` | Number of items scanned | `3` |
| `bs2LootItems[]` | Array of loot in cart | `[{lootId, lootUuid, name, rewards}, ...]` |
| `bs2LastUsedBadge` | Last badge that claimed | Used for cooldown |
| `bs2LastActivityTime` | Last interaction timestamp | Used for timeout |

---

## 🎮 Example Full Transaction

```
1. Player "John Doe" (041774D75F6180) scans badge
   → API: POST /loot/whatIsIt → Returns: badge, name, backpack limit 5
   
2. Display: "JOHN DOE - SCAN LOOT (0/5)"

3. Player scans Supply Crate (DA9E5CC1)
   → API: POST /loot/whatIsIt → Returns: loot, ID, rewards
   → API: GET /item/getItemById/xxx → Returns: "Contraband"
   → API: GET /item/getItemById/yyy → Returns: "Mutant Credit"
   
4. Display: "1/5 ITEMS - SCAN MORE"

5. Player scans Medkit (ABCD1234)
   → API: POST /loot/whatIsIt → Returns: loot, ID, rewards
   
6. Display: "2/5 ITEMS - SCAN MORE"

7. Player presses button
   → API: POST /loot/claimLoot (Supply Crate) → 200 OK
   → API: POST /loot/claimLoot (Medkit) → 200 OK
   
8. Display: "SUCCESS: 2/2 CLAIMED"

9. System clears session, stores "041774D75F6180" as last used badge

10. Display: "SCAN BADGE"
```

---

## 📚 Related Docs

- [API_TESTING_GUIDE.md](API_TESTING_GUIDE.md) - cURL examples
- [ADMIN_MODE_GUIDE.md](ADMIN_MODE_GUIDE.md) - Admin menu usage
- [SUMMARY.md](SUMMARY.md) - Complete codebase overview

---

**Last Updated**: December 27, 2025  
**Firmware Version**: BuyStation 25.1.0
