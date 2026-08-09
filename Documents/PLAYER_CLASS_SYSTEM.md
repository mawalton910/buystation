# Player Class System - Implementation Guide

## 🎯 Overview

The player class system provides loot multipliers based on player class assignments. Classes are **fetched once at device boot** and cached locally for fast lookup during transactions.

---

## 🔄 How It Works

### **1. Boot Sequence (Device Startup)**

**When**: After WiFi connects during `setup()`  
**Endpoint**: `POST https://iot.rke.world/loot/getPlayerClasses`  

**Request**:
```json
{
  "gameId": "681ae10fac40fb2222f9c27c",
  "mac_address": "24:58:7C:5C:60:AC",
  "serial_number": "gurugamesbuy2"
}
```

**Response**:
```json
{
  "playerClasses": [
    {
      "_id": "690b601f7de208659b6f331c",
      "name": "Commander",
      "keywords": ["Leader"],
      "game": "690a63467de208659b6f0ed0",
      "description": "A leader of a faction in the game."
    },
    {
      "_id": "690b602f7de208659b6f332d",
      "name": "Scout",
      "keywords": ["Scout"],
      "game": "690a63467de208659b6f0ed0"
    },
    {
      "_id": "690b603f7de208659b6f333e",
      "name": "Medic",
      "keywords": ["Medic"],
      "game": "690a63467de208659b6f0ed0"
    }
  ],
  "message": "Player classes retrieved successfully"
}
```

**What Happens**:
1. Device sends request with game ID (not badge-specific)
2. Server returns **all player classes** for the game
3. Device stores each class in `playerClassCache[]` array:
   - `id` - Class database ID (e.g., "690b601f7de208659b6f331c")
   - `name` - Class display name (e.g., "Commander")
   - `keyword` - First keyword for matching (e.g., "Leader")
   - `multiplier` - Calculated multiplier (e.g., 3 for Leader)

**Example Cache After Boot**:
```cpp
playerClassCache[0] = { 
  id: "690b601f7de208659b6f331c", 
  name: "Commander", 
  keyword: "Leader", 
  multiplier: 3 
}
playerClassCache[1] = { 
  id: "690b602f7de208659b6f332d", 
  name: "Scout", 
  keyword: "Scout", 
  multiplier: 2 
}
playerClassCache[2] = { 
  id: "690b603f7de208659b6f333e", 
  name: "Medic", 
  keyword: "Medic", 
  multiplier: 2 
}
playerClassCount = 3
```

---

### **2. Player Badge Scan (Runtime)**

**When**: Player scans their badge to start shopping  
**Endpoint**: `POST https://iot.rke.world/loot/whatIsIt` (existing)

**Response** (with player class):
```json
{
  "type": "badge",
  "data": {
    "uuid": "041774D75F6180",
    "name": "John Doe",
    "faction": "66a03bb56e7e39f9b4e28c5a",
    "playerClass": "690b601f7de208659b6f331c"  // ← Class ID reference
  },
  "hasBackpack": true,
  "globalBackpackLimit": 5
}
```

**What Happens**:
1. Device receives badge data including `playerClass` ID
2. Device searches `playerClassCache[]` for matching ID
3. If found: Sets `bs2PlayerClass` and `bs2ClassMultiplier`
4. If not found: Defaults to 1x multiplier

**Code Flow**:
```cpp
// In checkWhatIsIt() function:
if (data.containsKey("playerClass")) {
    String playerClassId = data["playerClass"].as<String>();
    
    // Search cache
    for (int i = 0; i < playerClassCount; i++) {
        if (playerClassCache[i].id == playerClassId) {
            bs2PlayerClass = playerClassCache[i].keyword;      // "Leader"
            bs2ClassMultiplier = playerClassCache[i].multiplier; // 3
            break;
        }
    }
}
```

---

### **3. Display During Shopping**

**When**: Player browses loot items before claiming

**Display Shows**:
```
┌─────────────────────┐
│    John Doe         │  ← Player name
│    Leader x3        │  ← Class + multiplier (orange)
│                     │
│   ITEM 1/3          │
│                     │
│  Supply Crate       │  ← Item name (green)
│                     │
│ Contraband: 1       │  ← Rewards (yellow)
│ Mutant Credit: 10   │
└─────────────────────┘
```

**Code Location**: [M5DialControl.h](../M5DialControl.h) - `displayBuyStation2LootBrowser()`

---

## 🎮 Class Multiplier Mapping

Default mappings in `getMultiplierForClass()`:

| **Keyword** | **Multiplier** | **Example Classes** |
|------------|---------------|-------------------|
| Leader     | 3x            | Commander, General |
| Scout      | 2x            | Scout, Recon |
| Medic      | 2x            | Medic, Healer |
| Engineer   | 2x            | Engineer, Builder |
| *(other)*  | 1x            | Default for unlisted classes |

**To Add New Class Types**:
Edit `getMultiplierForClass()` in [BuyStation2.h](../BuyStation2.h):

```cpp
int getMultiplierForClass(String classKeyword) {
    String keyword = classKeyword;
    keyword.toLowerCase();
    
    if (keyword == "leader") return 3;
    if (keyword == "scout") return 2;
    if (keyword == "sniper") return 2;  // ← Add new mappings
    if (keyword == "tank") return 4;    // ← Higher multiplier example
    
    return 1;  // Default
}
```

---

## 📊 Data Flow Diagram

```
BOOT SEQUENCE:
═══════════════
Device Startup
    ↓
WiFi Connect
    ↓
POST /loot/getPlayerClasses (gameId, mac, serial)
    ↓
Receive ALL game classes
    ↓
Store in playerClassCache[20]
    ↓
Ready for use


PLAYER SCAN:
════════════
Player scans badge
    ↓
POST /loot/whatIsIt (uuid)
    ↓
Response includes playerClass: "690b601f..."
    ↓
Lookup in playerClassCache by ID
    ↓
Match found? Set bs2PlayerClass + bs2ClassMultiplier
    ↓
Display shows "Leader x3"
```

---

## 🗂️ Key Variables

### **Global Cache (Boot Time)**
| Variable | Type | Purpose | Example |
|---------|------|---------|---------|
| `playerClassCache[]` | Array of structs | Stores all game classes | `[{id, name, keyword, multiplier}, ...]` |
| `playerClassCount` | int | Number of classes loaded | `3` |

### **Session Variables (Per Player)**
| Variable | Type | Purpose | Example |
|---------|------|---------|---------|
| `bs2PlayerClass` | String | Current player's class keyword | `"Leader"` |
| `bs2ClassMultiplier` | int | Current player's multiplier | `3` |

---

## 🔄 Cache Lifecycle

### **Cache is Loaded**:
- ✅ At device boot (after WiFi connects)
- ✅ Function: `fetchGamePlayerClasses()` in [BuyStation2.h](../BuyStation2.h)

### **Cache is Cleared**:
- ✅ On device restart/reboot (power cycle)
- ✅ Manual call to `clearPlayerClassCache()` (if needed)

### **Cache is NOT Cleared**:
- ❌ Between player transactions
- ❌ When switching device modes (Buy Station ↔ Relay, etc.)
- ❌ When entering/exiting admin mode

---

## 🧪 Testing the System

### **Test 1: Boot Sequence**
```bash
# Watch Serial Monitor during device boot - should see:
Fetching player classes for game...
Loaded Class: Commander (Leader) = 3x
Loaded Class: Scout (Scout) = 2x
Loaded Class: Medic (Medic) = 2x
Total player classes loaded: 3
Player classes loaded successfully
```

### **Test 2: Player Scan with Class**
```bash
# Player badge scan with class assigned:
Badge Type: badge
Badge Name: John Doe
Matched Player Class: Commander (Leader) = 3x

# Display should show: "Leader x3"
```

### **Test 3: Player Scan without Class**
```bash
# Player badge scan with NO class assigned:
Badge Type: badge
Badge Name: Jane Smith
No player class assigned or found - using default 1x multiplier

# Display shows NO class line (just player name)
```

### **Test 4: Manual API Test**
```bash
curl -X POST https://iot.rke.world/loot/getPlayerClasses \
  -H "Content-Type: application/json" \
  -d '{
    "gameId": "681ae10fac40fb2222f9c27c",
    "mac_address": "24:58:7C:5C:60:AC",
    "serial_number": "gurugamesbuy2"
  }'
```

**Expected**: JSON array of all player classes for the game

---

## 🛠️ Implementation Files

| File | Changes Made |
|------|-------------|
| [secrets.h](../secrets.h) | Added `player_classes_url` endpoint |
| [BuyStation2.h](../BuyStation2.h) | Added cache struct, fetch function, lookup logic |
| [buyStation_M5v3.ino](../buyStation_M5v3.ino) | Added `fetchGamePlayerClasses()` call in `setup()` |
| [M5DialControl.h](../M5DialControl.h) | Added class display in `displayBuyStation2LootBrowser()` |

---

## 💡 Usage Notes

### **Multiplier Storage vs Application**
- The `bs2ClassMultiplier` variable contains the multiplier value (1-4)
- **Currently stored but NOT applied to loot values**
- Ready for you to implement in claiming logic when rewards are distributed

### **Example Application** (Future Implementation):
```cpp
// When distributing rewards after claim:
int baseAmount = reward["amount"].as<int>();
int finalAmount = baseAmount * bs2ClassMultiplier;

// If Leader (3x): 10 credits → 30 credits
// If Scout (2x): 10 credits → 20 credits
// If no class (1x): 10 credits → 10 credits
```

### **Error Handling**
- If class fetch fails at boot: Device continues with default 1x multipliers
- If player has invalid class ID: Defaults to 1x multiplier
- If player has no class assigned: Defaults to 1x multiplier (no display)

---

## 📝 API Requirements

Your backend `/loot/getPlayerClasses` endpoint must:
1. Accept `gameId`, `mac_address`, `serial_number` in POST body
2. Return all player classes for that game
3. Include `_id`, `name`, and `keywords[]` for each class
4. Return empty array if no classes exist (not an error)

Your `/loot/whatIsIt` endpoint must:
1. Include `playerClass` field in badge responses (can be null)
2. Value should be the player class `_id` (not name or keyword)
3. If player has no class: omit field or set to null

---

**Last Updated**: December 27, 2025  
**Firmware Version**: BuyStation 25.1.0
