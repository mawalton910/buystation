# Admin Mode Guide

## Overview
Admin mode provides diagnostic, configuration, and operational control functions. It is accessed by scanning any authorized admin badge at any time during normal operation  even mid-session.

## Setup

### 1. Configure Admin Badges
Admin badges are defined as an **array** in `config.h`  multiple badges are supported:
```cpp
const String ADMIN_BADGE_UIDS[] = {
    "23859FFE",
    "BF1D2A40",
    "4F51D264",
    "43D6ACFE"
};
```
To add a badge: scan it, find its UID in the Serial Monitor output, then add it to the array.

## Accessing Admin Mode
- **Trigger**: Scan any admin badge at ANY time
- **Visual Feedback**: Screen flashes "ADMIN ACCESS"
- **Navigation**: Rotate encoder to scroll menu items, press button to confirm
- **Auto-exit**: Returns to the previous operational mode after **60 seconds** of inactivity

---

## Admin Menu Reference (15 Options)

### 0  Device Info
Displays serial number, MAC address, firmware version, and current mode. Auto-returns to menu after 5 seconds.

### 1  WiFi Status
Shows detailed connection info:
- SSID, IP address, MAC address
- RSSI (signal strength) with color coding: Green > -70 dBm, Yellow -70 to -80, Red < -80
- Auto-returns after 4 seconds

### 2  WiFi Reconnect
Forces a disconnect and reconnect using credentials stored in `secrets.h`. Use when connection is stale or dropped.

### 3  WiFi Scan / Connect
- Scans for nearby networks and lists them with signal strength
- Scroll with encoder to select a network
- Press button to connect (open networks connect immediately; secured networks prompt for password entry)
- Password entry uses the encoder to scroll through the character set and the button to confirm each character; select `<DEL>` to backspace, `<DONE>` to connect

### 4  Loot Lock / Unlock
- Put device into persistent loot-flip mode
- Scan any loot tag to toggle its `locked` state on the server
- Display shows the tag UUID and resulting status (LOCKED / UNLOCKED / NOT FOUND / ERROR)
- Press button to exit back to admin menu
- Scanning an admin badge also exits

### 5  Backpack Mode
Switches the device to Backpack Mode and exits admin. Resets Backpack Mode state.

### 6  Buy Station 2 Mode
Switches the device to Buy Station 2 Mode (the default) and exits admin. Resets Buy Station 2 state.

### 7  Loot Transfer Mode
Switches the device to Loot Transfer Mode and exits admin. Resets Loot Transfer state.

### 8  Relay Mode
Switches the device to Relay (Load Tag) Mode and exits admin. Device will post every scanned UUID directly to the server with no player/loot logic.

### 9  Map Mode
Switches the device to Map Mode and exits admin. Resets map state.

### 10  Dev Mode Toggle
Toggles the `bs2DevMode` flag. **When ON**: the badge cooldown is disabled, allowing the same badge to be scanned back-to-back. Useful for rapid testing. A success blink confirms the toggle. The current state is visible in the View Variables screen.

### 11  View Variables
Scrolls through all live session variables on-screen (badge UUIDs, names, faction IDs, loot counts, player class, multiplier, session timers, network state, etc.). Rotate encoder to scroll. Press button to exit.

### 12  Clear Variables
Wipes all session state without rebooting:
- Clears all Buy Station 2 session data (badge, loot items, cart)
- Clears Loot Transfer session data
- Clears Relay mode last scan
- Clears the player class cache and item name cache
- Useful mid-event to recover from a stuck or corrupt session

### 13  Exit Admin
Returns to the previous operational mode and shows the appropriate idle screen.

---

## Configuration Reference (`config.h`)

```cpp
// Admin badges (supports multiple)
const String ADMIN_BADGE_UIDS[] = { "UID1", "UID2", ... };

// Admin menu
#define ADMIN_MENU_ITEMS 14             // Total number of menu items
#define ADMIN_MENU_SCROLL_DELAY 200     // ms debounce between encoder steps
#define ADMIN_EXIT_TIMEOUT 30000        // Auto-exit menu after 30s (ms)

// Relay mode
#define RELAY_MODE_TIMEOUT 60000        // Exit relay mode after 60s of inactivity
#define RELAY_SCAN_DEBOUNCE 200         // NFC debounce for relay (ms)
```

---

## Usage Scenarios

### Scenario: WiFi Connection Issues
1. Scan admin badge  WiFi Status (check connection)
2. If disconnected  WiFi Reconnect
3. If network changed  WiFi Scan/Connect to select and enter new credentials
4. Exit, resume

### Scenario: Testing New Badges
1. Scan admin badge  Relay Mode
2. Scan each badge to verify NFC read and server recognition
3. Check UUID and HTTP response code on-screen
4. Press button to exit when done

### Scenario: Locked Loot Not Scanning
1. Scan admin badge  Loot Lock/Unlock (option 4)
2. Scan the locked loot tag  display shows current status
3. If LOCKED, scan again to flip to UNLOCKED
4. Press button to exit

### Scenario: Stuck Session / Bad State
1. Scan admin badge  Clear Variables (option 12)
2. Exit admin  device returns to idle scan screen with clean state

---

## Troubleshooting

### Admin Badge Not Working
- Check Serial Monitor to see what UUID is actually being read
- Verify that UUID is in the `ADMIN_BADGE_UIDS[]` array in `config.h` (exact match, uppercase, no spaces)

### Encoder Not Scrolling Menu
- Rotate more slowly  each detent = 4 encoder ticks (`ADMIN_ENCODER_STEP`)
- Encoder position is tracked continuously; large fast spins may skip items

### Relay Mode Shows FAILED
- Check Serial Monitor for HTTP response details
- Confirm WiFi is connected (WiFi Status first)
- Verify `update_uuid_url` in `secrets.h` is reachable

### Dev Mode Left On Accidentally
- Admin menu  Dev Mode Toggle (option 10) to turn it off
- Clear Variables does NOT reset dev mode  toggle it explicitly

---

## Serial Monitor Output

Key log lines to watch (115200 baud):
```
Admin badge detected!
Relay mode scan: DA9E5CC1
=== Relay Mode Update ===
Response Code: 200
Admin mode timeout - exiting
```

---

## Security Note

Admin badges grant privileged access to all device functions. Keep UIDs private. If a badge is lost:
1. Remove its UID from `ADMIN_BADGE_UIDS[]` in `config.h`
2. Reflash all devices

---

## Theme Support

All admin screens use the active theme colors from `config.h`:
- `THEME_ACCENT_COLOR`  Headers, selections, highlights
- `GREEN`  Success indicators, strong WiFi
- `RED`  Error indicators, weak WiFi, disconnected
- `WHITE`  Primary text
- `BLACK`  Background
- `0x7BEF` (Gray)  Secondary text, instructions

Available themes: `BLACK_MARKET`, `CYBERPUNK`, `MILITARY`, `CLASSIC`, `NEON`, `GURU_GAMES` (active: `GURU_GAMES`).
