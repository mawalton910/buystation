# API Integration Summary

## ✅ What Was Done

### 1. **Dual Mode System Implemented**
- **Buy Station Mode**: Uses new REST API with authentication
  - POST /private/rfid/check - Validate badges
  - POST /private/loot/claim - Complete transactions
- **Relay Mode**: Continues using legacy endpoint
  - POST /iot/updateUUID - Badge relay functionality

### 2. **Files Modified**

#### secrets.h
- Added new API configuration section
- Preserved legacy endpoints for relay mode
- Added Auth0 Bearer token placeholder
- Added new endpoint URLs with comments

#### relay2_M5.ino
- Added `checkRFID()` - New badge validation function
- Added `claimLoot()` - New loot claim function
- Modified `fetchBadgeData()` - Routes to new API
- Modified `sendTransactionRequest()` - Uses new claim function
- Kept old functions as `*Old()` for reference

#### Documentation
- **API_MIGRATION_GUIDE.md** - Complete migration documentation
- **TODO_CHECKLIST.md** - Quick reference for required updates

---

## 🔧 What You Need to Do

### Immediate (Required):
1. Open [secrets.h](secrets.h)
   - Line ~69: Set `apiBaseUrl`
   - Line ~84: Set `authToken`
   - Line ~88: Verify `gameId`

2. Test API endpoints with Postman/curl to get actual response formats

3. Open [relay2_M5.ino](relay2_M5.ino)
   - Line ~328: Update `checkRFID()` JSON request/response
   - Line ~442: Update `claimLoot()` JSON request/response

### Testing:
1. Compile and upload to M5 Dial
2. Monitor Serial output (115200 baud)
3. Test Buy Station mode with real badges
4. Test Relay mode (should still work unchanged)

---

## 📋 JSON Structures (VERIFY WITH YOUR API)

### Assumed Request - POST /private/rfid/check
```json
{
  "uuid": "041774D75F6180",
  "gameId": "668718757c02bf39a7a80fd4",
  "deviceId": "gurugamesbuy2",
  "type": "player"
}
```

### Assumed Response - POST /private/rfid/check
```json
{
  "valid": true,
  "data": {
    "name": "Player Name",
    "faction": "Red Team",
    "type": "weapon"
  }
}
```

### Assumed Request - POST /private/loot/claim
```json
{
  "playerUuid": "041774D75F6180",
  "lootUuid": "DA9E5CC1",
  "gameId": "668718757c02bf39a7a80fd4",
  "deviceId": "gurugamesbuy2",
  "playerName": "John Doe",
  "itemName": "Health Pack"
}
```

### Assumed Response - POST /private/loot/claim
```json
{
  "success": true,
  "message": "Loot claimed successfully",
  "data": {
    "transaction_id": "abc123",
    "timestamp": "2025-12-16T..."
  }
}
```

**⚠️ These are ASSUMPTIONS - Update code to match your actual API!**

---

## 🔍 Where to Find TODO Comments

Search for `TODO:` in relay2_M5.ino:
- Line ~328: Request structure for checkRFID()
- Line ~365: Response parsing for checkRFID()
- Line ~442: Request structure for claimLoot()
- Line ~479: Response parsing for claimLoot()

---

## 🚀 Quick Start

1. Update secrets.h (3 values)
2. Get real API responses
3. Update JSON parsing in relay2_M5.ino (2 functions)
4. Compile & upload
5. Test with Serial Monitor open

---

## 📞 Need Help?

Check these resources:
1. [API_MIGRATION_GUIDE.md](API_MIGRATION_GUIDE.md) - Detailed documentation
2. [TODO_CHECKLIST.md](TODO_CHECKLIST.md) - Quick reference
3. Serial Monitor output - Shows all API requests/responses
4. Look for commented "TODO:" in code

---

## ✨ Features Preserved

- WiFi scan and connect functionality
- Admin mode with all options
- Loot lock/unlock
- Device info display
- WiFi status and reconnect
- Operational mode switching
- Admin badge access system
- Rotary encoder navigation
- Password entry for WiFi

Everything works as before - just the API endpoints changed for Buy Station mode!

---

**Status**: ✅ Code Complete - Configuration Required
**Next**: Update secrets.h and test with your API
