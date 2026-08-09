# API Migration Guide - Buy Station Mode

## Overview
The Buy Station functionality has been updated to use the new API endpoints while maintaining backward compatibility for Relay Mode.

---

## What Changed

### 1. **Dual API System**
- **Buy Station Mode**: Uses NEW API endpoints (`/private/rfid/check`, `/private/loot/claim`)
- **Relay Mode**: Still uses OLD API endpoint (`/iot/updateUUID`)

### 2. **Authentication Added**
- All NEW API endpoints require Auth0 Bearer token authentication
- Old relay endpoint remains unauthenticated

### 3. **Structured Data Flow**
- Player badge scan → `POST /private/rfid/check` (validate player)
- Loot badge scan → `POST /private/rfid/check` (validate loot)
- Confirm transaction → `POST /private/loot/claim` (claim loot)

---

## Required Configuration

### In `secrets.h` - UPDATE THESE VALUES:

#### 1. **API Base URL** (Line ~69)
```cpp
const char* apiBaseUrl = "https://api.yourdomain.com";  // <<< UPDATE THIS
```
Replace with your actual API base URL (REACT_APP_API_HOST value)

#### 2. **Auth0 Bearer Token** (Line ~84)
```cpp
const char* authToken = "Bearer YOUR_AUTH0_TOKEN_HERE";  // <<< UPDATE THIS
```
Add your Auth0 Bearer token here. Format: `"Bearer eyJ0eXAiOiJKV1QiLCJhbGc..."`

#### 3. **Game ID** (Line ~88)
```cpp
String gameId = "668718757c02bf39a7a80fd4";  // <<< VERIFY THIS
```
Verify this is the correct game ID for your system

---

## API Request/Response Formats

### 1. POST /private/rfid/check

#### Current Request Format (ASSUMED - Update if needed):
```json
{
  "uuid": "041774D75F6180",
  "gameId": "668718757c02bf39a7a80fd4",
  "deviceId": "gurugamesbuy2",
  "type": "player"  // or "loot"
}
```

#### Expected Response Format (ASSUMED - Update if needed):
```json
{
  "valid": true,
  "data": {
    "name": "Player Name" or "Item Name",
    "faction": "Faction Name",  // for players
    "type": "item type"  // for loot
  }
}
```

**Update in `relay2_M5.ino`** → Function `checkRFID()` (Lines ~318-410)

---

### 2. POST /private/loot/claim

#### Current Request Format (ASSUMED - Update if needed):
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

#### Expected Response Format (ASSUMED - Update if needed):
```json
{
  "success": true,
  "message": "Loot claimed successfully",
  "data": {
    "transaction_id": "...",
    "timestamp": "..."
  }
}
```

**Update in `relay2_M5.ino`** → Function `claimLoot()` (Lines ~412-508)

---

## Files Modified

### 1. secrets.h
- Added new API base URL
- Added new endpoint URLs (rfidCheckUrl, lootClaimUrl)
- Added authToken for Bearer authentication
- Kept old URLs for relay mode (update_uuid_url)

### 2. relay2_M5.ino

#### New Functions Added:
- `checkRFID(uuid, isPlayer)` - Validates badges using `/private/rfid/check`
- `claimLoot(playerUuid, lootUuid)` - Claims loot using `/private/loot/claim`

#### Modified Functions:
- `fetchBadgeData()` - Now calls `checkRFID()` instead of old API
- `sendTransactionRequest()` - Now calls `claimLoot()` instead of old API

#### Legacy Functions (kept for reference):
- `fetchBadgeDataOld()` - Original implementation
- `sendTransactionRequestOld()` - Original implementation

---

## Testing Checklist

### Before Testing:
- [ ] Update `apiBaseUrl` in secrets.h
- [ ] Add Auth0 Bearer token in secrets.h
- [ ] Verify `gameId` is correct
- [ ] Update JSON request/response structures if needed

### Buy Station Mode Testing:
1. [ ] Switch to Buy Station Mode in admin menu
2. [ ] Scan player badge - should validate and show player info
3. [ ] Scan loot badge - should validate and show item info
4. [ ] Confirm transaction - should claim loot successfully
5. [ ] Check Serial Monitor for API requests/responses
6. [ ] Verify error handling for invalid badges
7. [ ] Test WiFi disconnection handling

### Relay Mode Testing:
1. [ ] Switch to Relay Mode in admin menu
2. [ ] Scan any badge - should send to `update_uuid_url`
3. [ ] Verify old API endpoint still works
4. [ ] Check Serial Monitor for relay updates

---

## Troubleshooting

### Issue: "HTTP 401 Unauthorized"
**Solution**: Check Auth0 Bearer token in secrets.h is correct and not expired

### Issue: "HTTP 400 Bad Request"
**Solution**: Review JSON request format in `checkRFID()` or `claimLoot()` functions

### Issue: "Parse Error" on response
**Solution**: Update JSON parsing in functions to match actual API response structure

### Issue: Player/Loot data not displaying
**Solution**: Check JSON field names in response parsing (e.g., `data.name`, `data.faction`)

### Issue: Relay mode not working
**Solution**: Verify `update_uuid_url` is still correct in secrets.h

---

## Serial Monitor Debug Output

All API calls output detailed logs:

```
=== RFID Check Request ===
URL: https://api.yourdomain.com/private/rfid/check
Request: {"uuid":"041774D75F6180","gameId":"668...","deviceId":"guru...","type":"player"}
Response Code: 200
Response: {"valid":true,"data":{...}}
Player: John Doe (Red Team)
```

Monitor the Serial output to debug API integration issues.

---

## Next Steps

1. **Get actual API documentation** for:
   - Exact request body structures for `/private/rfid/check`
   - Exact response structures
   - Exact request/response for `/private/loot/claim`

2. **Update JSON parsing** in:
   - `checkRFID()` function (lines ~318-410)
   - `claimLoot()` function (lines ~412-508)

3. **Test thoroughly** with real badges and actual API

4. **Optional enhancements**:
   - Add more detailed error messages based on API error codes
   - Cache player/item data to reduce API calls
   - Add retry logic for failed API calls
   - Implement GET endpoints for items/factions if needed

---

## Support

If you encounter issues:
1. Check Serial Monitor output for detailed error messages
2. Verify all TODO comments in code have been addressed
3. Confirm API endpoints are accessible and authentication works
4. Test with curl/Postman first to validate API responses

---

**Last Updated**: December 16, 2025
**Version**: 1.0
