# OTA Update Quick Reference

## 🚀 Quick Setup (3 Steps)

### 1. Edit OTAUpdate.h
```cpp
const char* GITHUB_USER = "YourUsername";
const char* GITHUB_REPO = "YourRepo";
const char* OTA_WIFI_SSID = "YourWiFi";
const char* OTA_WIFI_PASSWORD = "YourPassword";
String targetBranch = "main";
```

### 2. Build & Upload Firmware
```
Arduino IDE → Sketch → Export Compiled Binary
Upload the .bin file to your GitHub repo
```

### 3. Scan OTA Trigger Tag
```
RFID UID: 0x00 0x00 0x00 0x00
Device will auto-update and reboot
```

---

## 📝 Generated GitHub URL Format
```
https://raw.githubusercontent.com/{USER}/{REPO}/{BRANCH}/{FILENAME}
```

**Example:**
```
https://raw.githubusercontent.com/JohnDoe/M5-Firmware/main/buyStation_M5v3.ino.bin
```

---

## 🔧 Customize OTA Trigger UID

Don't have a 0x00 tag? Change it in `OTAUpdate.h`:

```cpp
const uint8_t OTA_TRIGGER_UID[] = {0xAB, 0xCD, 0xEF, 0x12};  // Your tag UID
```

To find your tag's UID, scan it and check Serial Monitor.

---

## 📊 Update Progress Messages

| Screen Display | Meaning |
|---------------|---------|
| `CONNECTING TO OTA WIFI...` | Joining update WiFi |
| `DOWNLOADING FIRMWARE...` | Downloading from GitHub |
| `XX%` | Download percentage |
| `UPDATE SUCCESS!` | Done! Rebooting... |
| `WIFI FAILED` | Check WiFi settings |
| `UPDATE FAILED` | Check GitHub URL |

---

## ⚡ Integration Code (Already Added)

The OTA check runs at the **start of loop()**:

```cpp
void loop() {
    M5Dial.update();
    
    if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
        if (isOTAUpdateTag()) {
            triggerGitHubUpdate();  // Auto-update & reboot
        }
    }
    
    // Your normal code continues...
}
```

**Non-blocking**: If tag is not OTA trigger, code continues normally.

---

## 🛠️ Common Issues

**Can't connect to WiFi?**
- Verify SSID and password
- Use 2.4GHz WiFi (ESP32 doesn't support 5GHz)

**Update fails?**
- Check GitHub repo is public
- Verify .bin file exists at the URL
- Test URL in browser

**Tag not detected?**
- Check UID matches `OTA_TRIGGER_UID[]`
- View Serial Monitor to see scanned UIDs

---

## 📂 Files Added/Modified

**New Files:**
- `OTAUpdate.h` - OTA update functions
- `Documents/OTA_UPDATE_GUIDE.md` - Full documentation

**Modified Files:**
- `buyStation_M5v3.ino` - Added `#include "OTAUpdate.h"` and loop() check

---

## 🔐 Security Note

Uses `WiFiClientSecure` with `setInsecure()` to bypass GitHub SSL validation.  
Safe for GitHub Raw URLs since domain is hardcoded.

---

## 🎯 Testing Checklist

- [ ] Configure GitHub settings in `OTAUpdate.h`
- [ ] Set OTA WiFi credentials
- [ ] Export compiled `.bin` file
- [ ] Upload `.bin` to GitHub
- [ ] Test GitHub Raw URL in browser
- [ ] Configure OTA trigger tag UID
- [ ] Test update with Serial Monitor
- [ ] Verify device reboots successfully

---

## 📞 Debug Tips

**Always check Serial Monitor during updates!**

Serial output shows:
- WiFi connection status
- Generated GitHub URL
- Download progress
- Error details

```
Serial.begin(115200);  // Already in your setup()
```

Open Serial Monitor at **115200 baud** before scanning OTA tag.

---

## 🎨 Customization Ideas

**Change update branch dynamically:**
```cpp
if (adminMenuSelection == BRANCH_SELECTION) {
    targetBranch = "dev";  // Switch to dev branch
}
```

**Multiple OTA tags for different branches:**
```cpp
if (tagUID == 0x00000000) targetBranch = "main";
if (tagUID == 0x11111111) targetBranch = "dev";
if (tagUID == 0x22222222) targetBranch = "testing";
```

**Add version checking:**
```cpp
// Compare FIRMWARE_VERSION with remote version file
// Only update if newer version available
```

---

**For full documentation, see:** [OTA_UPDATE_GUIDE.md](OTA_UPDATE_GUIDE.md)
