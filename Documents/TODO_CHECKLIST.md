# Deployment Checklist

Use this before every event deployment.

---

## REQUIRED  Must update before every deployment

### `secrets.h`

| Variable | What to set |
|---|---|
| `ssid` | Event WiFi network name |
| `password` | Event WiFi password |
| `serialNum` | This device's serial (e.g. `"gurugamesbuy3"`)  must match API registration |
| `macAddress` | This device's MAC address (check label or Serial Monitor on boot) |
| `gameId` | The `_id` of the active game from the API |

> The `serialNum` also determines which game relay-mode scans are attributed to server-side.

---

## CONDITIONAL  Update if these differ from last event

### `config.h`

| Variable | When to change |
|---|---|
| `ADMIN_BADGE_UIDS[]` | Different admin badges for this event |
| `WIFI_RETRY_TAGS[]` | Different WiFi-retry NFC tags |
| `MAP_LOCATIONS[]` | Different field locations / NFC tag UIDs for map mode |
| `HTTP_REQUEST_TIMEOUT_MS` | If API is known to be slower (default: 5000 ms) |

### `BuyStation2.h`

| Variable | When to change |
|---|---|
| `FACTION_ID_SELECT_FEW` | New game has different faction `_id` for Select Few |
| `FACTION_ID_WAVY` | New game has different faction `_id` for Wavy |
| `FACTION_ID_HEIST_ADMIN` | New game has different admin faction `_id` |
| `FACTION_ID_SUPPLIERS` | New game has different Suppliers faction `_id` |

> Also update the name-matching strings in `resolveFactionId()` if faction names changed.

---

## Pre-Event Smoke Test

1. Flash firmware and power on
2. Confirm it connects to WiFi and shows the Buy Station idle screen
3. Scan a known player badge  confirm name, faction, and loot limit appear correctly
4. Scan a known loot tag  confirm item name and reward text appear
5. Press button  confirm claim sends successfully (check Serial Monitor for HTTP 200)
6. Scan admin badge  confirm admin menu opens
7. Admin  WiFi Status  confirm SSID, IP, and RSSI look correct
8. Admin  Device Info  confirm serial and gameId are correct for this event
9. Admin  View Variables  spot-check session state is clean
10. Admin  Exit  confirm device returns to idle screen

---

## Mid-Event Quick Fixes

| Problem | Fix |
|---|---|
| WiFi dropped | Admin  WiFi Reconnect |
| Changed venue WiFi | Admin  WiFi Scan/Connect |
| Session stuck / wrong player loaded | Admin  Clear Variables |
| Loot tag needs locking/unlocking | Admin  Loot Lock/Unlock, scan the tag |
| Same badge can't be used back-to-back in testing | Admin  Dev Mode Toggle |

---

## Status:  Ready for deployment once `secrets.h` is updated