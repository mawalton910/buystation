# Admin Mode - View & Clear Variables

## 🎯 New Features Added

Two new admin menu options have been added to help with debugging and device maintenance:

### **1. View Variables (Menu Item 11)**
Shows all device variables in a scrollable list

### **2. Clear Variables (Menu Item 12)**
Clears all non-critical variables while preserving device configuration

---

## 📋 View Variables

### **Access**:
1. Scan admin badge
2. Rotate encoder to "View Variables"
3. Press button to enter

### **Display Shows**:
Scrollable list of all device variables organized by category:

#### **Device Info**
- Serial number
- MAC address
- Game ID

#### **Mode**
- Current operational mode (BuyStation2, Relay, etc.)

#### **Buy Station 2 Variables**
- `BS2:Badge` - Current badge UUID
- `BS2:Name` - Current player name
- `BS2:Class` - Player class keyword
- `BS2:Multiplier` - Loot multiplier (e.g., "3x")
- `BS2:LootLimit` - Max items player can claim
- `BS2:LootCount` - Items currently scanned
- `BS2:Claimed` - Items successfully claimed
- `BS2:LastUsed` - Last badge that completed transaction (for cooldown)
- `BS2:DevMode` - Dev mode status (YES/NO)
- `BS2:Session` - Time since last activity

#### **Player Class Cache**
- `ClassCache` - Number of classes loaded at boot

#### **Relay Mode**
- `Relay:UUID` - Last relay UUID
- `Relay:Success` - Last relay success status

#### **Loot Flip**
- `LootFlip:UUID` - Last loot flipped
- `LootFlip:Status` - Lock status (LOCKED/UNLOCKED)

#### **WiFi**
- `WiFi` - Connection status
- `RSSI` - Signal strength in dBm

#### **System**
- `FreeHeap` - Available memory in KB
- `Uptime` - Device uptime in minutes

### **Navigation**:
- **Rotate Encoder**: Scroll through variables
- **Press Button**: Return to admin menu
- **Admin Badge**: Exit admin mode completely

### **Display Format**:
```
┌─────────────────────┐
│   Variables         │
│                     │
│ Serial:   guru...   │
│ MAC:      24:58:... │
│ Mode:     BuyStation2│
│ BS2:Badge: 041774... │
│ BS2:Name:  John Doe │
│ BS2:Class: Leader   │
│ BS2:Mult:  3x       │
│ BS2:Limit: 5        │
│                     │
│ Rotate: Scroll (1/3)│
│ Press: Back to Menu │
└─────────────────────┘
```

---

## 🗑️ Clear Variables

### **Access**:
1. Scan admin badge
2. Rotate encoder to "Clear Variables"
3. Press button to clear

### **What Gets Cleared**:

#### **Session Data**
- ✅ Current player badge & name
- ✅ Player faction & class
- ✅ Loot items array
- ✅ Badge cooldown (`bs2LastUsedBadge`)
- ✅ Session timestamps
- ✅ Relay mode last scan data
- ✅ Loot flip last scan data

### **What is PRESERVED**:

#### **Critical System Data**
- ❌ Device serial number
- ❌ MAC address
- ❌ Game ID
- ❌ Current operational mode
- ❌ Player class cache (loaded at boot)
- ❌ WiFi connection & settings
- ❌ Admin badge UIDs

### **Use Cases**:
- Reset device between game sessions
- Clear stuck transactions
- Test fresh player interactions
- Debug session timeout issues

### **Confirmation**:
```
┌─────────────────────┐
│    CLEARING...      │
└─────────────────────┘
        ↓
┌─────────────────────┐
│ VARIABLES CLEARED   │
└─────────────────────┘
```

### **Serial Output**:
```
=== Clearing Non-Critical Variables ===
Non-critical variables cleared
Preserved: Device config, Player class cache, WiFi settings
========================================
```

---

## 🎮 Admin Menu Layout

Complete admin menu (14 items total):

| **Index** | **Menu Item** | **Icon** | **Function** |
|-----------|--------------|----------|-------------|
| 0 | Device Info | ℹ️ | Show device details |
| 1 | WiFi Status | 📶 | Show WiFi connection |
| 2 | WiFi Reconnect | 🔄 | Reconnect to WiFi |
| 3 | WiFi Scan | 🔍 | Scan & connect to network |
| 4 | Loot Lock | 🔒 | Flip loot lock status |
| 5 | Backpack | 🎒 | Switch to Backpack mode |
| 6 | Buy Station 2 | 🏪 | Switch to Buy Station 2 mode |
| 7 | Loot Transfer | ↔️ | Switch to Loot Transfer mode |
| 8 | Relay Mode | 📡 | Switch to Relay mode |
| 9 | Map Mode | 🗺️ | Switch to Map mode |
| 10 | Dev Mode | 🔧 | Toggle dev mode (badge cooldown) |
| **11** | **View Variables** | 📄 | **View all variables** |
| **12** | **Clear Variables** | 🗑️ | **Clear non-critical data** |
| 13 | Exit Admin | 🚪 | Exit admin mode |

---

## 💻 Implementation Details

### **Files Modified**:

#### [config.h](../config.h)
- Updated `ADMIN_MENU_ITEMS` from 12 to 14

#### [M5DialControl_ADMIN_DISPLAYS.h](../M5DialControl_ADMIN_DISPLAYS.h)
- Added `displayVariableViewer()` function
- Added menu icons for cases 11 & 12
- Added menu names for new items

#### [BuyStation2.h](../BuyStation2.h)
- Added `clearNonCriticalVariables()` function

#### [buyStation_M5v3.ino](../buyStation_M5v3.ino)
- Added case 11 handler (View Variables)
- Added case 12 handler (Clear Variables)
- Updated menu wrapping to 0-13 range
- Included `M5DialControl_ADMIN_DISPLAYS.h` header

---

## 🧪 Testing

### **Test View Variables**:
1. Scan admin badge
2. Rotate to "View Variables"
3. Press button
4. Verify all variables display correctly
5. Rotate encoder to scroll
6. Press button to return to menu

### **Test Clear Variables**:
1. Start a transaction (scan player badge, scan loot)
2. Scan admin badge
3. Rotate to "Clear Variables"
4. Press button
5. Check Serial Monitor for confirmation
6. Return to normal mode
7. Verify session is cleared (display shows "SCAN BADGE")

### **Test Scrolling**:
1. View Variables screen
2. Count total variables (should be ~24)
3. Rotate encoder fully down
4. Verify scroll indicator updates
5. Verify all variables are accessible

---

## 📝 Example Output

### **Typical Buy Station 2 Session**:
```
Variables Display:

Serial:      gurugamesbuy2
MAC:         24:58:7C:5C:60:AC
GameId:      681ae10fac40...
Mode:        BuyStation2
BS2:Badge:   041774D75F...
BS2:Name:    John Doe
BS2:Class:   Leader
BS2:Mult:    3x
BS2:Limit:   5
BS2:LootCnt: 2
BS2:Claimed: 0
BS2:LastUsed: (none)
BS2:DevMode: NO
BS2:Session: 15s ago
ClassCache:  3 classes
Relay:UUID:  (none)
WiFi:        Connected
RSSI:        -45 dBm
FreeHeap:    187 KB
Uptime:      42 min
```

---

## 🔍 Debugging Use Cases

### **Troubleshoot Badge Cooldown**:
1. View Variables
2. Check `BS2:LastUsed` - shows which badge is blocked
3. Use Clear Variables to reset cooldown

### **Check Player Class**:
1. Scan player badge
2. View Variables
3. Check `BS2:Class` and `BS2:Mult` values
4. Verify against `ClassCache` count

### **Monitor Session Timeout**:
1. Start transaction
2. View Variables during shopping
3. Check `BS2:Session` value (time since last activity)
4. Verify timeout happens at ~30 seconds

### **Memory Issues**:
1. View Variables
2. Check `FreeHeap` value
3. If low (<50 KB), clear variables or restart device

---

**Last Updated**: December 27, 2025  
**Firmware Version**: BuyStation 25.1.0
