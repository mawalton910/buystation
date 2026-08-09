# OTA Update System Architecture

## System Flow Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        M5 DIAL DEVICE                            │
│                                                                  │
│  ┌────────────────────────────────────────────────────────┐    │
│  │                    Main Loop                            │    │
│  │                                                         │    │
│  │  1. M5Dial.update()                                    │    │
│  │  2. Check for RFID scan                                │    │
│  │  3. If OTA trigger tag (0x00 0x00 0x00 0x00)?         │    │
│  │     ├─ YES → triggerGitHubUpdate()                    │    │
│  │     └─ NO  → Continue with normal operations          │    │
│  │                                                         │    │
│  └────────────────────────────────────────────────────────┘    │
│                           ↓                                      │
│  ┌────────────────────────────────────────────────────────┐    │
│  │            triggerGitHubUpdate()                        │    │
│  │                                                         │    │
│  │  1. Disconnect from normal WiFi                        │    │
│  │  2. Connect to OTA WiFi                                │    │
│  │  3. Build GitHub URL                                   │    │
│  │  4. Download .bin file (with progress)                 │    │
│  │  5. Flash firmware to device                           │    │
│  │  6. Reboot into new firmware                           │    │
│  │                                                         │    │
│  └────────────────────────────────────────────────────────┘    │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
                               ↓
                               ↓ HTTPS Request
                               ↓
┌──────────────────────────────────────────────────────────────────┐
│                         GITHUB                                    │
│                                                                  │
│  Repository: {GITHUB_USER}/{GITHUB_REPO}                        │
│  Branch: {targetBranch}                                         │
│  File: {FIRMWARE_FILENAME}                                      │
│                                                                  │
│  https://raw.githubusercontent.com/                             │
│    {USER}/{REPO}/{BRANCH}/{FILE}                                │
│                                                                  │
│  Response: firmware.bin (binary file)                           │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Component Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     OTAUpdate.h                             │
│                                                             │
│  ┌───────────────────────────────────────────────────┐    │
│  │  Configuration Variables                          │    │
│  │  • OTA_TRIGGER_UID[] - RFID trigger              │    │
│  │  • OTA_WIFI_SSID - WiFi network                  │    │
│  │  • OTA_WIFI_PASSWORD - WiFi password             │    │
│  │  • GITHUB_USER - GitHub username                 │    │
│  │  • GITHUB_REPO - Repository name                 │    │
│  │  • FIRMWARE_FILENAME - .bin filename             │    │
│  │  • targetBranch - Branch name                    │    │
│  └───────────────────────────────────────────────────┘    │
│                                                             │
│  ┌───────────────────────────────────────────────────┐    │
│  │  Display Functions                                │    │
│  │  • displayOTAProgress() - Show download progress │    │
│  │  • displayOTAStatus() - Show status messages     │    │
│  └───────────────────────────────────────────────────┘    │
│                                                             │
│  ┌───────────────────────────────────────────────────┐    │
│  │  Core Functions                                   │    │
│  │  • isOTAUpdateTag() - Check if tag is trigger    │    │
│  │  • triggerGitHubUpdate() - Execute OTA update    │    │
│  │  • getOTAUrl() - Build GitHub URL                │    │
│  └───────────────────────────────────────────────────┘    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                              ↑
                              │ #include "OTAUpdate.h"
                              │
┌─────────────────────────────────────────────────────────────┐
│                buyStation_M5v3.ino                          │
│                                                             │
│  void loop() {                                              │
│      M5Dial.update();                                       │
│                                                             │
│      // OTA CHECK                                           │
│      if (M5Dial.Rfid.PICC_IsNewCardPresent() &&           │
│          M5Dial.Rfid.PICC_ReadCardSerial()) {              │
│          if (isOTAUpdateTag()) {                           │
│              triggerGitHubUpdate();                        │
│          }                                                  │
│      }                                                      │
│                                                             │
│      // Normal device operations...                        │
│  }                                                          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Update Sequence Diagram

```
User          M5 Dial         OTA WiFi        GitHub
 |               |                |              |
 |--Scan Tag---->|                |              |
 |               |                |              |
 |               |--Disconnect--->|              |
 |               |                |              |
 |               |--Connect------>|              |
 |               |<--Connected----|              |
 |               |                |              |
 |               |--Build URL-------------------->|
 |               |                |              |
 |               |--HTTPS GET-------------------->|
 |               |<--Send .bin--------------------|
 |               |                |              |
 |[Download     ]|                |              |
 |[Progress     ]|                |              |
 |[Display      ]|                |              |
 |               |                |              |
 |               |--Flash Firmware|              |
 |               |                |              |
 |               |--Reboot------->|              |
 |               |                |              |
 |               X                |              |
 |               |                |              |
 |[New Firmware ]|                |              |
 |[Boots        ]|                |              |
 |               |                |              |
```

---

## File Dependencies

```
buyStation_M5v3.ino
    │
    ├── WiFi.h (ESP32 core)
    ├── M5Dial.h (M5Stack library)
    │
    └── OTAUpdate.h
            │
            ├── WiFiClientSecure.h (ESP32 core)
            ├── HTTPUpdate.h (ESP32 core)
            ├── esp_task_wdt.h (ESP32 core)
            └── M5Dial.h (for display)
```

**Required Libraries:**
- M5Dial (M5Stack official)
- WiFi (ESP32 core - included)
- WiFiClientSecure (ESP32 core - included)
- HTTPUpdate (ESP32 core - included)

---

## State Machine

```
┌─────────────┐
│   IDLE      │ ← Normal device operation
└──────┬──────┘
       │
       │ RFID scan detected
       │
       ↓
  ┌─────────────┐
  │ Is OTA Tag? │
  └──┬──────┬───┘
     │      │
     NO     YES
     │      │
     │      ↓
     │  ┌──────────────────┐
     │  │ CONNECTING_WIFI  │ Display: "CONNECTING TO OTA WIFI..."
     │  └────────┬─────────┘
     │           │
     │           ↓
     │  ┌──────────────────┐
     │  │ PREPARING_UPDATE │ Display: "PREPARING UPDATE..."
     │  └────────┬─────────┘
     │           │
     │           ↓
     │  ┌──────────────────┐
     │  │  DOWNLOADING     │ Display: "DOWNLOADING... XX%"
     │  └────────┬─────────┘
     │           │
     │           ↓
     │  ┌──────────────────┐
     │  │   FLASHING       │ Built-in HTTPUpdate handles this
     │  └────────┬─────────┘
     │           │
     │           ↓
     │  ┌──────────────────┐
     │  │   REBOOTING      │ Display: "UPDATE SUCCESS! REBOOTING..."
     │  └────────┬─────────┘
     │           │
     │           ↓
     │       ESP.restart()
     │
     ↓
┌─────────────┐
│   IDLE      │ ← Continue normal operation
└─────────────┘
```

---

## Error Handling Flow

```
                 triggerGitHubUpdate()
                         │
                         ↓
              ┌──────────────────┐
              │ Connect to WiFi  │
              └────┬─────────┬───┘
                   │         │
              Success     Failure
                   │         │
                   │         ↓
                   │    Display "WIFI FAILED"
                   │         │
                   │         ↓
                   │    ESP.restart()
                   │
                   ↓
         ┌──────────────────────┐
         │ Download from GitHub │
         └────┬───────────┬─────┘
              │           │
         Success      Failure
              │           │
              │           ↓
              │    Display "UPDATE FAILED"
              │           │
              │           ↓
              │    ESP.restart()
              │
              ↓
        ┌─────────────────┐
        │ Flash Firmware  │
        └────┬────────┬───┘
             │        │
        Success   Failure
             │        │
             │        ↓
             │   Display "UPDATE FAILED"
             │        │
             │        ↓
             │   ESP.restart()
             │
             ↓
     Display "UPDATE SUCCESS!"
             │
             ↓
        ESP.restart()
             │
             ↓
     New firmware boots
```

---

## Memory Management

```
Flash Memory Layout:
┌───────────────────────────────┐
│  Bootloader                   │ ← Protected
├───────────────────────────────┤
│  Partition Table              │ ← Protected
├───────────────────────────────┤
│  Current Firmware (app0)      │ ← Running firmware
├───────────────────────────────┤
│  OTA Firmware (app1)          │ ← Downloaded here during OTA
├───────────────────────────────┤
│  NVS (Non-Volatile Storage)   │ ← Config data
├───────────────────────────────┤
│  SPIFFS/LittleFS (if used)    │ ← File system
└───────────────────────────────┘

Update Process:
1. Download new firmware to OTA partition (app1)
2. Verify checksum
3. Set boot partition to app1
4. Reboot
5. Bootloader loads firmware from app1
6. Next update will write to app0 (ping-pong)
```

---

## Network Communication

```
M5 Dial                                    GitHub Server
   │                                              │
   │──────── HTTPS GET Request ──────────────────>│
   │  GET /user/repo/branch/file.bin               │
   │  Host: raw.githubusercontent.com              │
   │  User-Agent: ESP32HTTPUpdate                  │
   │                                               │
   │<──────── HTTP Response ─────────────────────│
   │  HTTP/1.1 200 OK                             │
   │  Content-Type: application/octet-stream      │
   │  Content-Length: XXXXX                       │
   │                                               │
   │<──────── Binary Data Stream ────────────────│
   │  [firmware binary data...]                   │
   │  [progress updates shown on screen]          │
   │                                               │
   │<──────── Transfer Complete ─────────────────│
   │                                               │
   
   WiFi Security:
   ┌──────────────────────────────┐
   │  WiFiClientSecure            │
   │  • TLS/SSL encryption        │
   │  • setInsecure() mode        │
   │  • No cert validation        │
   │  • Safe: GitHub URLs locked  │
   └──────────────────────────────┘
```

---

## Configuration Variables Map

```cpp
// OTAUpdate.h Configuration Map

├─ RFID Trigger
│  └─ OTA_TRIGGER_UID[] = {0x00, 0x00, 0x00, 0x00}
│
├─ WiFi Settings
│  ├─ OTA_WIFI_SSID = "ESPN2G"
│  └─ OTA_WIFI_PASSWORD = "Alexander1985"
│
├─ GitHub Repository
│  ├─ GITHUB_USER = "YourUsername"           ← CHANGE THIS
│  ├─ GITHUB_REPO = "YourRepo"               ← CHANGE THIS
│  ├─ FIRMWARE_FILENAME = "firmware.bin"
│  └─ targetBranch = "main"
│
└─ Generated URL Format
   └─ https://raw.githubusercontent.com/{USER}/{REPO}/{BRANCH}/{FILE}
```

---

## Display States

```
State 1: CONNECTING TO WIFI
┌─────────────────────────┐
│                         │
│    CONNECTING TO        │
│      OTA WIFI...        │
│                         │
└─────────────────────────┘

State 2: DOWNLOADING
┌─────────────────────────┐
│   DOWNLOADING           │
│   FIRMWARE...           │
│                         │
│       45%               │
│                         │
│  ▓▓▓▓▓▓▓▓▓░░░░░░░      │
│                         │
│   123KB / 274KB         │
└─────────────────────────┘

State 3: SUCCESS
┌─────────────────────────┐
│                         │
│   UPDATE SUCCESS!       │
│                         │
│    REBOOTING...         │
│                         │
└─────────────────────────┘

State 4: ERROR
┌─────────────────────────┐
│                         │
│    UPDATE FAILED        │
│                         │
│  [Error message]        │
│                         │
└─────────────────────────┘
```

---

## Integration Points

```
buyStation_M5v3.ino Structure:

Lines 1-20:    #include statements
               ├─ Line 21: #include "OTAUpdate.h" ← ADDED

Lines 85-110:  setup() function

Lines 111-1698: loop() function
               ├─ Line 115: M5Dial.update()
               ├─ Lines 117-125: OTA CHECK ← ADDED
               │   if (M5Dial.Rfid.PICC_IsNewCardPresent()) {
               │       if (isOTAUpdateTag()) {
               │           triggerGitHubUpdate();
               │       }
               │   }
               └─ Line 127+: Existing device logic
```

---

## Testing Scenarios

```
Test 1: Normal Operation (Non-OTA Tag)
   User scans regular RFID → Device continues normally ✓

Test 2: OTA Trigger (Success Path)
   User scans OTA tag → WiFi connects → Download → Flash → Reboot ✓

Test 3: WiFi Failure
   User scans OTA tag → WiFi fails → Display error → Reboot ✓

Test 4: Download Failure
   User scans OTA tag → WiFi OK → Download fails → Display error ✓

Test 5: GitHub URL Error
   Wrong URL config → 404 error → "NO UPDATE AVAILABLE" ✓

Test 6: Interrupted Update
   Power loss during download → Safe: ESP32 partition system protects ✓
```

---

## Performance Metrics

```
Operation               Time        Notes
─────────────────────────────────────────────────────
WiFi Connect           5-10s       Depends on network
Build URL              <1ms        String concatenation
HTTPS Handshake        2-3s        SSL negotiation
Download (500KB)       10-30s      Depends on speed
Flash Firmware         5-10s       Writing to flash
Reboot                 2-3s        ESP32 boot time
─────────────────────────────────────────────────────
Total (typical)        25-60s      Full update cycle
```

---

## Security Considerations

```
✓ HTTPS/TLS encryption for downloads
✓ Physical access required (RFID scan)
✓ Dedicated WiFi network (network isolation)
✓ Hardcoded GitHub domain (no URL injection)
✓ ESP32 secure boot capable (optional)

⚠ setInsecure() used (no cert pinning)
⚠ No firmware signature verification
⚠ Public GitHub repo required (or add auth token)

Recommended Enhancements:
• Add firmware signature verification
• Implement rollback on boot failure
• Add version checking before download
• Use GitHub API with auth for private repos
```

---

## Troubleshooting Decision Tree

```
                    OTA Update Fails
                          │
                          ↓
              ┌───────────────────────┐
              │ Check Serial Monitor  │
              └───────┬───────────────┘
                      │
        ┌─────────────┴─────────────┐
        │                           │
        ↓                           ↓
  "WiFi failed"              "HTTP Error XXX"
        │                           │
        ↓                           ↓
  Check WiFi settings         Check GitHub URL
  • SSID correct?             • Repo public?
  • Password correct?         • File exists?
  • 2.4GHz network?          • Branch correct?
        │                           │
        ↓                           ↓
  Fix & retry                 Fix & retry
```

---

**For complete documentation, see:**
- [OTA_UPDATE_GUIDE.md](OTA_UPDATE_GUIDE.md) - Full guide
- [OTA_QUICK_REFERENCE.md](OTA_QUICK_REFERENCE.md) - Quick start
- [OTA_IMPLEMENTATION_SUMMARY.md](OTA_IMPLEMENTATION_SUMMARY.md) - Overview
