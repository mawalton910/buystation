# Guru Games - Buy Station
**Current Version**: BuyStation 26.2.0

A specialized IoT device system built on the M5Stack Dial (ESP32-S3) for managing player transactions, loot distribution, and game mechanics in live-action role-playing events.

## 🎮 What It Does

The Buy Station is an RFID-enabled device that revolutionizes LARP gameplay by providing:

### For Players
- **Purchase & Claim Loot**: Scan your badge to browse and claim available items
- **Loot Browsing**: Use the rotary dial to scroll through faction-specific rewards
- **Multi-Claim Sessions**: Claim multiple items in one session before timeout
- **Player Class Multipliers**: Certain player classes automatically multiply loot reward amounts
- **Backpack System**: Players with a backpack can claim up to 5 items per visit (1 without)
- **Inventory Transfer**: Transfer loot between player factions seamlessly
- **Interactive Maps**: View terrain maps with current location tracking
- **Quick Badge Updates**: Fast badge scanning for relay and photobooth integrations

### For Game Masters
- **WiFi Management**: Scan for networks and configure credentials directly on-device — no reflash needed
- **Device Monitoring**: View real-time device info, MAC, serial, firmware version, and network status
- **Loot Control**: Lock/unlock individual loot tags on the fly
- **Mode Switching**: Toggle between all operational modes from the admin menu
- **Dev Mode**: Bypass badge cooldown for rapid back-to-back testing
- **Variable Viewer**: Inspect all live session variables directly on-screen
- **Cache Management**: Clear all session data and player class cache without rebooting
- **Remote Updates**: Push firmware updates to all devices simultaneously

## 🔧 Hardware

- **M5Stack Dial** (ESP32-S3) with built-in RFID reader
- 240x240 round touchscreen display
- Rotary encoder with button
- WiFi connectivity for server integration

## 📦 Operational Modes

### 1. Buy Station 2 Mode (Default)
Multi-claim loot system where players can:
- Scan badge to identify player and load loot backpack limit
- Browse items using rotary encoder
- Claim multiple items in one session (up to backpack limit)
- Auto-timeout protection (30s session inactivity)
- Badge cooldown: same badge cannot be used twice back-to-back — another player must go first
- Player class multipliers applied automatically to reward amounts at scan time

### 2. Backpack Mode
Simple loot claim system with badge-based inventory capacity. The player claim flow no longer flips lock/unlock state after a successful claim.

### 3. Loot Transfer Mode
Transfer inventory items between different player factions:
- Scan two player badges from different factions
- Browse available items and select transfer amounts
- Validates faction eligibility before processing

### 4. Relay Mode ("Load Tag Mode")
Raw NFC badge reader — posts the scanned UUID directly to the server with no player/loot logic. Used as a check-in station, attendance scanner, or game event trigger. The server maps the device `serial_number` to the active game.

### 5. Photobooth Mode
Dedicated photobooth badge reader mode. Scans are posted to the IoT device record as `last_uuid` using the same endpoint and payload shape as Relay Mode, so Creator/Loot photobooth tools can poll the selected reader without extra device logic.

### 6. Map Mode
Display interactive terrain maps with location markers for navigation during gameplay. NFC tag scans can update the displayed position.

## 🎯 How Players Use It

1. **Scan Your Badge** → System identifies you, checks for backpack, and loads your loot limit
2. **Browse Items** → Rotate the dial to scroll through available loot items and see reward contents
3. **Claim Items** → Press the button to claim all items in your cart at once
4. **Auto-Save** → Session ends automatically; all claims are recorded server-side

## 🛠️ Admin Menu (15 Options)

Accessed by scanning an authorized admin badge at any time. Navigate with the rotary encoder, confirm with button press.

| # | Option | Description |
|---|--------|-------------|
| 0 | Device Info | Shows serial, MAC, firmware version, and current mode |
| 1 | WiFi Status | Displays SSID, IP, signal strength (RSSI), and connection state |
| 2 | WiFi Reconnect | Forces a reconnect using saved credentials |
| 3 | WiFi Scan/Connect | Scans for nearby networks and allows on-device password entry |
| 4 | Loot Lock/Unlock | Scan any loot tag to toggle its locked state on the server |
| 5 | Backpack Mode | Switch device to Backpack Mode |
| 6 | Buy Station 2 Mode | Switch device to Buy Station 2 Mode (default) |
| 7 | Loot Transfer Mode | Switch device to Loot Transfer Mode |
| 8 | Relay Mode | Switch device to Relay/Load Tag Mode |
| 9 | Photobooth | Switch device to Photobooth reader mode |
| 10 | Map Mode | Switch device to Map Mode |
| 11 | Dev Mode Toggle | Enable/disable badge cooldown bypass for testing |
| 12 | View Variables | Scroll through all live session variables on-screen |
| 13 | Clear Variables | Wipe all session state and item name cache |
| 14 | Exit Admin | Return to previous operational mode |

> Admin auto-exits after **60 seconds** of inactivity and returns to the previous mode.

## 📡 Over-the-Air Updates

Devices support remote firmware updates for seamless version management:
- Scan a designated trigger card to start update
- Device automatically downloads and installs new firmware
- Progress displayed with percentage and download statistics
- Automatic reboot after successful installation
- All devices can be updated simultaneously

## ⚡ Performance Notes

- **Item name cache**: Item names fetched from the server are cached in memory for the session. Repeat scans of the same loot type do not make additional network calls.
- **HTTP timeout**: Configured to 5 seconds (`HTTP_REQUEST_TIMEOUT_MS`). Failed/unreachable API calls unblock the device within 5 seconds.
- **JSON allocation**: Small request/response documents use stack-allocated `StaticJsonDocument` to avoid heap fragmentation.
- **Keep-alive**: A shared `WiFiClientSecure` client is reused across requests (`http.setReuse(true)`) to avoid TLS handshake overhead on every scan.

## Secure Buy Station 2 WebSocket Mode

`MODE_BUY_STATION_2` can now use the authenticated IoT `/ws` Universal Dial action channel while keeping the existing buy-station screens and cart behavior.

The switch is in `config.h`:

```cpp
#define USE_SECURE_BUYSTATION_WS 1
```

When enabled, Buy Station 2 uses:

- Player badge scan -> `beginLootSession`
- Loot prop scan -> `identify`
- Cart claim -> `claimLoot`
- Session reset/timeout/complete -> `endLootSession`

The older HTTP implementation remains in the file as a fallback if `USE_SECURE_BUYSTATION_WS` is set to `0`.

To register the device with the backend:

1. Create or update the IoT device record in Creator.
2. The IoT device record must use the same `serialNum` and `macAddress` values from `secrets.h`.
3. Provision a device key for that IoT record.
4. Paste the provisioned values into `secrets.h`:

```cpp
const char* deviceKeyId = "dev-...";
const char* deviceSecretHex = "64-character-lowercase-hex-secret";
```

5. Paste the production IoT root CA into `ROOT_CA_PEM`.
6. Assign the IoT device to one enabled widget in the target game.
7. On that widget, enable Universal Dial config with at least loot station access:

```json
{
  "universalDial": {
    "enabled": true,
    "allowedModes": ["loot-station"]
  }
}
```

On boot, Serial Monitor should show:

```text
[secure-auth] device authenticated iot=<id> widget=<id> game=<id>
[secure-auth] capabilities: loot
```

If it says `credential accepted; attach device to a widget`, the key is valid but Creator has not attached the IoT device to a widget. If it says `device authentication failed`, the secret does not match the provisioned IoT device credential.

After secure authentication, the backend widget assignment is authoritative. The firmware updates the in-memory `gameId` from `hello.widget.game` when the server provides it, then loads faction and player-class caches for that attached game. The `gameId` in `secrets.h` is still useful as a setup fallback, but a correctly assigned Creator widget should be treated as the source of truth.

The admin **View Variables** page now shows:

- `SecureWS`: whether the device is authenticated
- `SecureWidget`: the attached widget ID prefix
- `SecureGame`: the backend game ID prefix from the widget
- `SecureModes`: allowed Universal Dial modes returned by the backend

## 🔧 Configuring for a New Game

When deploying this device for a new game/event, update the following variables:

### `secrets.h` — Change every time

| Variable | Purpose |
|---|---|
| `ssid` | WiFi network name |
| `password` | WiFi password |
| `serialNum` | This device's registered serial number in the API |
| `macAddress` | This device's MAC address |
| `gameId` | The new game's `_id` from the API |
| `deviceKeyId` | Provisioned secure IoT device key ID |
| `deviceSecretHex` | Provisioned 64-character secure IoT device secret |
| `ROOT_CA_PEM` | Production IoT TLS root certificate |

> The `serialNum` is also how the server maps Relay and Photobooth scans to the correct IoT device record. Those scan payloads update `last_uuid`; Creator/Loot then read that value from the selected IoT device.

### `config.h` — Change if admin badges or map locations differ

| Variable | Purpose |
|---|---|
| `ADMIN_BADGE_UIDS[]` | Array of NFC UIDs for admin badges (supports multiple) |
| `WIFI_RETRY_TAGS[]` | NFC UIDs used to force WiFi reconnect |
| `MAP_LOCATIONS[]` | NFC tag UIDs and GPS coordinates for map mode |
| `HTTP_REQUEST_TIMEOUT_MS` | API call timeout in ms (default: 5000) |
| `BS2_SESSION_TIMEOUT` | Player session inactivity timeout in ms (default: 30000) |

### Faction and game data

On successful secure device initialization, the dial should use backend-provided game/widget context and cached faction names where available. Keep legacy fallback names only as a safety net for first boot or backend failure.

### Quick Checklist
- [ ] `secrets.h` → `gameId`, `serialNum`, `macAddress`, `ssid`, `password`
- [ ] `config.h` → `ADMIN_BADGE_UIDS[]`
- [ ] Creator/API device registry → attach this serial/MAC to the correct widget/game
- [ ] Creator Photobooth → select this IoT device as the booth badge reader when using Photobooth mode

---

## 🎮 Game Integration

The system integrates with backend servers to:
- Identify player badges and load class/faction/backpack data via `POST /loot/whatIsIt`
- Claim loot and record transactions via `POST /loot/claimLoot`
- Relay raw badge scans via `POST /iot/updateUUID2`
- Send periodic heartbeats via `POST /iot/getActiveMinigame`
- Load player class metadata at boot via `POST /loot/getPlayerClasses`

## 🔐 Security

- Physical RFID badge required for all transactions
- Faction-based access control enforced server-side
- Session timeout protection (30s inactivity)
- TLS-encrypted communication with backend servers (`WiFiClientSecure`)
- Admin access requires a physical admin badge (configurable array of UIDs)
- Badge cooldown prevents the same player from spamming the device back-to-back

## 🙏 Acknowledgments

Built with M5Stack Dial hardware, ESP32-S3 platform, and the Arduino framework.

---

**Current Version**: BuyStation 26.2.0
**Last Updated**: March 31, 2026
