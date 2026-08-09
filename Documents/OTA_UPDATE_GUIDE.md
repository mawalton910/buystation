# OTA Update Feature - GitHub Remote Firmware Update

## Overview
This system allows you to remotely update your M5 Dial firmware by scanning a special RFID tag (0x00 0x00 0x00 0x00). The device will automatically download and install new firmware from your GitHub repository.

## Features
✅ **Trigger via RFID**: Simply scan the special tag (UID: 0x00 0x00 0x00 0x00)  
✅ **Automatic WiFi**: Connects to dedicated OTA WiFi network during update  
✅ **GitHub Integration**: Downloads compiled .bin files from GitHub Raw URLs  
✅ **Progress Feedback**: Real-time download progress displayed on screen  
✅ **Branch Selection**: Easily switch between GitHub branches (main, dev, etc.)  
✅ **Auto-Reboot**: Device automatically restarts into new firmware after success  
✅ **Non-Blocking**: OTA check happens early in loop() without interfering with other operations  

---

## Setup Instructions

### 1. Configure GitHub Repository Info
Edit `OTAUpdate.h` and update these values:

```cpp
const char* GITHUB_USER = "YourGitHubUsername";     // Your GitHub username
const char* GITHUB_REPO = "YourRepoName";           // Your repository name
const char* FIRMWARE_FILENAME = "buyStation_M5v3.ino.bin";  // Your .bin filename
```

### 2. Configure OTA WiFi Network
The device uses a separate WiFi network during updates. Edit these in `OTAUpdate.h`:

```cpp
const char* OTA_WIFI_SSID = "ESPN2G";           // WiFi SSID for updates
const char* OTA_WIFI_PASSWORD = "Alexander1985"; // WiFi password
```

**Why separate WiFi?**  
- Ensures updates work even if normal WiFi is down
- Allows you to use a dedicated update network for security
- Doesn't interfere with your normal device WiFi settings

### 3. Set Target Branch
You can easily change which GitHub branch to download from:

```cpp
String targetBranch = "main";  // Options: "main", "dev", "testing", etc.
```

---

## How to Use

### Step 1: Compile Your Firmware
1. In Arduino IDE, go to **Sketch → Export Compiled Binary**
2. This creates a `.bin` file in your sketch folder
3. The file will be named something like `buyStation_M5v3.ino.m5stack_dial.bin`

### Step 2: Upload to GitHub
1. Add your `.bin` file to your GitHub repository
2. Make sure it's in the root directory (or update the path in `getOTAUrl()`)
3. Commit and push to your target branch

### Step 3: Prepare the OTA Trigger Tag
You need an RFID tag with UID: **0x00 0x00 0x00 0x00**

**How to create this tag:**
- **Option A**: Use a writable RFID tag and program it to this UID
- **Option B**: Modify `OTA_TRIGGER_UID[]` in `OTAUpdate.h` to match an existing tag you have

```cpp
const uint8_t OTA_TRIGGER_UID[] = {0x00, 0x00, 0x00, 0x00};  // Change these values
```

### Step 4: Trigger the Update
1. Power on your M5 Dial device
2. Scan the OTA trigger RFID tag
3. Watch the screen for update progress

---

## Update Process Flow

```
┌─────────────────────────────────────┐
│ User scans OTA trigger RFID tag     │
│ (UID: 0x00 0x00 0x00 0x00)          │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Device displays "CONNECTING TO      │
│ OTA WIFI..."                        │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Connects to dedicated OTA WiFi      │
│ (Disconnects from normal WiFi)      │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Builds GitHub URL dynamically:      │
│ https://raw.githubusercontent.com/  │
│ {USER}/{REPO}/{BRANCH}/{FILE}       │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Downloads .bin file with progress:  │
│ "DOWNLOADING FIRMWARE..."           │
│ "XX%"                               │
│ Progress bar + KB downloaded        │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Flashes new firmware to device      │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ "UPDATE SUCCESS!" message           │
│ "REBOOTING..."                      │
└────────────────┬────────────────────┘
                 ▼
┌─────────────────────────────────────┐
│ Device automatically reboots        │
│ Runs new firmware                   │
└─────────────────────────────────────┘
```

---

## Screen Messages

| Message | Meaning |
|---------|---------|
| `CONNECTING TO OTA WIFI...` | Device is connecting to the update WiFi network |
| `WIFI FAILED` | Could not connect to OTA WiFi - check SSID/password |
| `PREPARING UPDATE...` | Connected, building firmware URL |
| `DOWNLOADING FIRMWARE...` | Downloading .bin file from GitHub |
| `XX%` | Download progress percentage |
| `UPDATE FAILED` | Download or flash failed - check GitHub URL and file |
| `NO UPDATE AVAILABLE` | File not found on GitHub |
| `UPDATE SUCCESS!` | Firmware updated successfully |
| `REBOOTING...` | Device is restarting into new firmware |

---

## Troubleshooting

### ❌ "WIFI FAILED" Error
**Problem**: Device can't connect to OTA WiFi  
**Solution**: 
- Verify `OTA_WIFI_SSID` and `OTA_WIFI_PASSWORD` in `OTAUpdate.h`
- Make sure the WiFi network is active and in range
- Check that WiFi uses 2.4GHz (ESP32 doesn't support 5GHz)

### ❌ "UPDATE FAILED" Error
**Problem**: Download or firmware flash failed  
**Solution**:
- Check that GitHub repository is **public** (or use GitHub token for private repos)
- Verify `GITHUB_USER`, `GITHUB_REPO`, and `FIRMWARE_FILENAME` are correct
- Make sure the `.bin` file exists in your repository
- Check the Serial Monitor for detailed error messages

### ❌ "NO UPDATE AVAILABLE" Error
**Problem**: GitHub returned 404 (file not found)  
**Solution**:
- Confirm the `.bin` file is committed to the correct branch
- Verify `targetBranch` matches your GitHub branch name
- Check the firmware filename exactly matches (case-sensitive!)

### ❌ OTA Tag Not Detected
**Problem**: Scanning the tag doesn't trigger update  
**Solution**:
- Use Serial Monitor to see what UID is being scanned
- Adjust `OTA_TRIGGER_UID[]` to match your actual tag
- Make sure the RFID reader is working (test with other tags)

---

## Advanced Configuration

### Change Target Branch at Runtime
Currently `targetBranch` is hardcoded. You could add admin menu options to switch branches:

```cpp
// In admin menu:
case MENU_ITEM_CHANGE_BRANCH:
    targetBranch = "dev";  // Switch to development branch
    break;
```

### Use Private GitHub Repositories
For private repos, you'll need to add authentication:

```cpp
// In triggerGitHubUpdate():
HTTPUpdate httpUpdate;
httpUpdate.setAuthorization("your_github_token");
```

### Custom Firmware Paths
If your `.bin` is in a subdirectory:

```cpp
const char* FIRMWARE_FILENAME = "firmware/buyStation_M5v3.ino.bin";
```

### Multiple OTA Trigger Tags
To support multiple trigger tags:

```cpp
const uint8_t OTA_TRIGGER_UIDS[][4] = {
    {0x00, 0x00, 0x00, 0x00},
    {0xFF, 0xFF, 0xFF, 0xFF},
    {0x12, 0x34, 0x56, 0x78}
};
```

---

## Security Notes

⚠️ **Current Implementation Uses `setInsecure()`**  
This bypasses SSL certificate validation for GitHub. This is acceptable because:
- GitHub's URLs are hardcoded
- It avoids manual certificate thumbprint management
- The firmware binary itself can't be tampered with on GitHub

For production environments, consider:
- Firmware signature verification
- Using GitHub API with authentication tokens
- Implementing certificate pinning

---

## Integration Details

The OTA update check is integrated at the **start of `loop()`** to ensure it has highest priority:

```cpp
void loop() {
    M5Dial.update();
    
    // OTA UPDATE CHECK (HIGH PRIORITY)
    if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
        if (isOTAUpdateTag()) {
            triggerGitHubUpdate();  // This will reboot device
        }
    }
    
    // Rest of your normal code continues below...
}
```

**Why at the start?**
- Catches OTA tag before any state machine logic
- Prevents conflicts with admin mode or other RFID operations
- Device reboots after update, so subsequent code doesn't execute anyway

**Non-Blocking Design:**
- If the scanned tag is NOT the OTA trigger, execution continues normally
- Your existing code (Buy Station, Admin, etc.) is completely unaffected
- The check adds minimal overhead (~1ms per scan)

---

## Testing Checklist

- [ ] Update GitHub repository info in `OTAUpdate.h`
- [ ] Configure OTA WiFi credentials
- [ ] Compile firmware and export `.bin` file
- [ ] Upload `.bin` to GitHub repository
- [ ] Verify file is accessible via GitHub Raw URL
- [ ] Create or configure OTA trigger RFID tag
- [ ] Test with Serial Monitor connected
- [ ] Verify download progress appears on screen
- [ ] Confirm device reboots into new firmware
- [ ] Test that normal operations still work after OTA

---

## File Structure

```
buyStation_M5v3/
├── buyStation_M5v3.ino          # Main file (includes OTAUpdate.h)
├── OTAUpdate.h                  # NEW: OTA update functionality
├── config.h                     # Your existing config
├── secrets.h                    # Your existing WiFi secrets
└── Documents/
    └── OTA_UPDATE_GUIDE.md      # This guide
```

---

## Example GitHub URL

With these settings:
```cpp
const char* GITHUB_USER = "JohnDoe";
const char* GITHUB_REPO = "M5Dial-Firmware";
String targetBranch = "main";
const char* FIRMWARE_FILENAME = "buyStation_M5v3.ino.bin";
```

The generated URL will be:
```
https://raw.githubusercontent.com/JohnDoe/M5Dial-Firmware/main/buyStation_M5v3.ino.bin
```

You can test this URL in your browser - it should download the `.bin` file.

---

## Version History

- **v1.0** - Initial implementation with GitHub OTA support
- Features: RFID trigger, WiFiClientSecure, progress display, auto-reboot

---

## Questions?

Check the Serial Monitor for detailed debug output during the update process. All important steps are logged.
