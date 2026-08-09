# API Testing Guide

Use this to test your API endpoints before integrating them into the device.

---

## 1. Test Authentication

```bash
curl -X GET "https://api.yourdomain.com/private/games/YOUR_GAME_ID" \
  -H "Authorization: Bearer YOUR_AUTH0_TOKEN"
```

**Expected**: Should return game data (not 401 Unauthorized)

---

## 2. Test RFID Check - Player Badge

```bash
curl -X POST "https://api.yourdomain.com/private/rfid/check" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_AUTH0_TOKEN" \
  -d '{
    "uuid": "041774D75F6180",
    "gameId": "668718757c02bf39a7a80fd4",
    "deviceId": "gurugamesbuy2",
    "type": "player"
  }'
```

**Look for**:
- What fields are in the response?
- Where is the player name? (data.name? player.name? name?)
- Where is the faction? (data.faction? faction?)
- What indicates success? (valid: true? success: true? status: ok?)

**Copy the actual response and update checkRFID() function accordingly**

---

## 3. Test RFID Check - Loot Badge

```bash
curl -X POST "https://api.yourdomain.com/private/rfid/check" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_AUTH0_TOKEN" \
  -d '{
    "uuid": "DA9E5CC1",
    "gameId": "668718757c02bf39a7a80fd4",
    "deviceId": "gurugamesbuy2",
    "type": "loot"
  }'
```

**Look for**:
- Where is the item name?
- Where is the item type?
- Is the structure the same as player check?

---

## 4. Test Loot Claim

```bash
curl -X POST "https://api.yourdomain.com/private/loot/claim" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_AUTH0_TOKEN" \
  -d '{
    "playerUuid": "041774D75F6180",
    "lootUuid": "DA9E5CC1",
    "gameId": "668718757c02bf39a7a80fd4",
    "deviceId": "gurugamesbuy2",
    "playerName": "Test Player",
    "itemName": "Test Item"
  }'
```

**Look for**:
- Success indicator field name
- Error message location
- Any transaction ID or confirmation data

---

## 5. Test Error Cases

### Invalid UUID
```bash
curl -X POST "https://api.yourdomain.com/private/rfid/check" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer YOUR_AUTH0_TOKEN" \
  -d '{
    "uuid": "INVALID12345",
    "gameId": "668718757c02bf39a7a80fd4",
    "type": "player"
  }'
```

**Look for**: Error message structure

### Missing Auth Token
```bash
curl -X POST "https://api.yourdomain.com/private/rfid/check" \
  -H "Content-Type: application/json" \
  -d '{
    "uuid": "041774D75F6180",
    "gameId": "668718757c02bf39a7a80fd4",
    "type": "player"
  }'
```

**Expected**: 401 Unauthorized

---

## 6. Document Actual Responses

### RFID Check Response (Player):
```json
// PASTE YOUR ACTUAL RESPONSE HERE


```

### RFID Check Response (Loot):
```json
// PASTE YOUR ACTUAL RESPONSE HERE


```

### Loot Claim Response (Success):
```json
// PASTE YOUR ACTUAL RESPONSE HERE


```

### Error Response:
```json
// PASTE YOUR ACTUAL RESPONSE HERE


```

---

## 7. Update Code Based on Responses

Once you have actual responses, update these functions in relay2_M5.ino:

1. **checkRFID()** (Line ~328)
   - Match request structure
   - Match response field names
   - Extract correct data fields

2. **claimLoot()** (Line ~442)
   - Match request structure
   - Match response field names
   - Extract success/error indicators

---

## Testing Checklist

- [ ] Auth token works (no 401 errors)
- [ ] RFID check returns player data
- [ ] RFID check returns loot data
- [ ] Loot claim succeeds
- [ ] Error responses are handled
- [ ] Documented all actual response structures
- [ ] Updated code to match API responses
- [ ] Tested with device Serial Monitor

---

## Tips

1. **Use Postman** if you prefer a GUI over curl
2. **Save all responses** to reference while coding
3. **Test invalid data** to see error message formats
4. **Check HTTP status codes** (200=success, 400=bad request, 401=unauthorized, etc.)
5. **Watch for case sensitivity** in JSON field names
6. **Test with real badge UUIDs** from your system

---

## Common Issues

### Issue: 401 Unauthorized
- Check Bearer token is correct
- Verify token hasn't expired
- Ensure "Bearer " prefix is included

### Issue: 400 Bad Request
- Check JSON structure matches API expectations
- Verify all required fields are present
- Check field names match exactly (case-sensitive)

### Issue: 404 Not Found
- Verify API base URL is correct
- Check endpoint path is correct
- Ensure gameId exists in system

---

**Next Step**: Once you have real responses, update the TODO sections in relay2_M5.ino
