# Buy Station M5 - Mutant Warfare LARP System
**Current Version**: BuyStation 25.3.1

A comprehensive IoT device system built on the M5Stack Dial (ESP32-S3) for managing player transactions, loot distribution, and game mechanics in live-action role-playing events.

## 🎮 Overview

The Buy Station M5 is a specialized RFID-enabled device that allows players to:
- Purchase and claim loot items using player badges
- Browse available loot inventory with faction-specific rewards
- Transfer inventory between players
- View interactive terrain maps with location tracking
- Manage game state through administrative controls

## 🔧 Hardware Requirements

- **M5Stack Dial** (ESP32-S3)
- Built-in **PN532 NFC/RFID Reader**
- 240x240 Round Display
- Rotary Encoder with Button
- WiFi Connectivity (2.4GHz)

## 📦 Features

### Player Modes
- **Buy Station 2 Mode**: Multi-claim loot system with session management
- **Backpack Mode**: Claim and unlock loot items
- **Loot Transfer Mode**: Transfer inventory between factions
- **Relay Mode**: Quick badge scanning for game integration
- **Map Mode**: Display terrain maps with current location

### Administrative Features
- WiFi management and network scanning
- Device information display
- Loot lock/unlock controls
- Variable viewing and clearing
- Mode switching capabilities

### Over-the-Air (OTA) Updates
- **Remote firmware updates** via GitHub
- Triggered by scanning designated RFID tag
- Automatic download and installation
- Progress display with percentage and download stats
- Automatic reboot after successful update

## 🚀 Getting Started

### Prerequisites

1. **Arduino IDE** with ESP32 board support
2. **M5Dial Library**: Install via Arduino Library Manager
3. **Required Libraries**:
   - WiFi (ESP32 core)
   - HTTPClient
   - ArduinoJson
   - WiFiClientSecure
   - HTTPUpdate

### Installation

1. Configure your settings in `buyStation_M5v3.ino`:
   ```cpp
   const char* WIFI_SSID = "YourWiFiSSID";
   const char* WIFI_PASSWORD = "YourWiFiPassword";
   ```

2. Update device identifiers in `secrets.h`:
   ```cpp
   const char* DEVICE_SERIAL = "your_device_serial";
   const char* DEVICE_MAC = "your_mac_address";
   ```

3. Update API endpoints in `config.h` if needed

4. Select **Crabik SLot ESP32-S3** as your board

5. Upload to your M5Stack Dial device

## 📡 OTA Updates

### Triggering an Update

1. Scan designated OTA trigger RFID card
2. Device connects to OTA WiFi (can be different from normal WiFi)
3. Downloads firmware from: `https://raw.githubusercontent.com/[USER]/[REPO]/main/update.ino.bin`
4. Displays progress with percentage and download size
5. Installs and reboots automatically

### Deploying New Firmware

1. **Export Compiled Binary**:
   - Arduino IDE → Sketch → Export Compiled Binary
   
2. **Rename the file** to: `update.ino.bin` (must match FIRMWARE_FILENAME in OTAUpdate.h)

3. **Upload to GitHub**:
   - Place file in the **root** of the `main` branch
   - Commit and push changes

4. **Deploy**:
   - Scan the OTA trigger card on any device
   - All devices will update to the new version

### OTA Configuration

Configure in `OTAUpdate.h`:
```cpp
// OTA Trigger Card (scan to start update)
const String OTA_TRIGGER_UIDS[] = {
    "YOUR_OTA_TAG_UID",  // Add more trigger cards as needed
};

// Update WiFi (can be different from normal WiFi)
const char* OTA_WIFI_SSID = "YourOTAWiFi";
const char* OTA_WIFI_PASSWORD = "YourOTAPassword";

// GitHub Settings
const char* GITHUB_USER = "your_github_username";
const char* GITHUB_REPO = "your_repository_name";
const char* FIRMWARE_FILENAME = "update.ino.bin";
String targetBranch = "main";
```

## 🎯 Usage

### Admin Access

Scan admin badge to enter admin mode. Navigate using rotary encoder:
- **Rotate**: Scroll through menu
- **Press**: Select option
- **Scan Admin Badge Again**: Exit admin mode

### Device Modes

Navigate admin menu using rotary encoder:
- **Rotate**: Scroll through options
- **Press**: Select option
- **Scan Admin Badge Again**: Exit admin mode

Available admin options:
1. Device Info
2. WiFi Status
3. WiFi Reconnect
4. WiFi Scan/Connect
5. Loot Lock Management
6. Backpack Mode
7. Buy Station 2 Mode (Default)
8. Loot Transfer Mode
9. Relay Mode
10. Map Mode
11. Development Mode
12. View Variables
13. Clear Variables
14. Exit Admin

### Buy Station 2 Workflow

1. **Scan Player Badge** → System fetches available loot
2. **Browse Items** → Use rotary encoder to view loot
3. **Press Button** → Claim selected item
4. **Repeat** → Claim multiple items in one session
5. **Auto-Timeout** → Session ends after inactivity

## 📁 Project Structure

```
buyStation_M5v3/
├── buyStation_M5v3.ino          # Main program file
├── OTAUpdate.h                  # OTA update functionality
├── config.h                     # Configuration settings
├── secrets.h                    # WiFi and device credentials
├── BadgeUtils.h                 # RFID badge utilities
├── ApiClient.h                  # API communication
├── WiFiManager.h                # WiFi management
├── ErrorHandler.h               # Error message handling
├── BuyStation2.h                # Buy Station 2 mode
├── BackpackMode.h               # Backpack functionality
├── LootTransferMode.h           # Loot transfer system
├── MapMode.h                    # Map display mode
├── M5DialControl.h              # Display controls
├── initSequence_M5.h            # Initialization routines
├── AdminDeviceInfo.h            # Device info display
├── AdminLootLock.h              # Loot lock controls
├── AdminWiFi.h                  # WiFi admin functions
└── Documents/
    ├── OTA_UPDATE_GUIDE.md      # Complete OTA documentation
    ├── OTA_QUICK_REFERENCE.md   # Quick OTA setup guide
    ├── OTA_ARCHITECTURE.md      # System architecture diagrams
    ├── ADMIN_MODE_GUIDE.md      # Admin mode documentation
    ├── API_MIGRATION_GUIDE.md   # API integration guide
    └── SUMMARY.md               # Project summary
```

## 🔐 Security

- HTTPS/TLS encryption for all API calls
- WiFiClientSecure for OTA downloads
- Physical RFID access required for admin functions
- Session timeout protection (configurable)
- Dedicated WiFi network for OTA updates

## 🌐 API Integration

The system integrates with a backend API for:
- Player badge data retrieval
- Loot inventory management
- Transaction recording
- Faction management
- Game state synchronization

**Primary API Endpoints:**
```cpp
const String API_ENDPOINT = "https://your-server.com/api/endpoint";
const String LOOT_ENDPOINT = "https://your-server.com/api/lootManagement";
```

**Data Submitted:**
- Player badge UUID
- Loot item claims and transfers
- Faction information
- Device identifiers (serial, MAC)

API endpoints configured in `ApiClient.h`.

## 🐛 Troubleshooting

### WiFi Connection Issues
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check SSID and password in configuration files
- Scan admin badge → WiFi Status to diagnose
- Use "WiFi Reconnect" option in admin menu

### OTA Update Failures
- **"WIFI FAILED"**: Check OTA WiFi credentials in `OTAUpdate.h`
- **"UPDATE FAILED"**: Verify `.bin` file exists at GitHub URL
- **"NO UPDATE AVAILABLE"**: Check branch name and filename
- Test URL in browser: `https://raw.githubusercontent.com/[USER]/[REPO]/main/update.ino.bin`

### RFID Read Issues
- Ensure badge is close to device (within 2-3 cm)
- Check Serial Monitor (115200 baud) for UID readings
- Verify badge UID matches configured values
- Allow debounce time between scans (2 seconds default)

## 📊 Serial Monitor

Connect at **115200 baud** for debug output:
- WiFi connection status with IP address
- RFID card UIDs as they're scanned
- API request/response logs
- Loot transaction confirmations
- OTA update progress
- Error messages and stack traces

## 🔄 Version Control

Current firmware version defined in `config.h`:
```cpp
#define FIRMWARE_VERSION "BuyStation 25.1.0"
```

Version history managed through GitHub releases. Use the OTA system to deploy updates to all devices simultaneously.

## 📝 License

[Add your license here]

## 👥 Contributors

- [Your Name/Team]

## 🤝 Contributing

[Add contribution guidelines if applicable]

## 📧 Support

For issues and questions:
- Open an issue on GitHub
- [Add contact information]

## 🙏 Acknowledgments

- Built with M5Stack Dial hardware
- ESP32-S3 platform
- Arduino framework
- M5Dial library by M5Stack

---

**Current Version**: BuyStation 25.3.1
**Last Updated**: December 30, 2025  
**Maintained by**: mawalton910
