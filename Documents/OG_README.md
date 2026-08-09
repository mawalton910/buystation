# Buy Station M5 - Mutant Warfare LARP System

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

1. Clone this repository:
   ```bash
   git clone https://github.com/mawalton910/buystation.git
   ```

2. Open `buyStation_M5v3.ino` in Arduino IDE

3. Configure your settings in `secrets.h`:
   ```cpp
   const char* ssid = "YourWiFiSSID";
   const char* password = "YourWiFiPassword";
   String serialNum = "your_device_serial";
   String macAddress = "your_mac_address";
   ```

4. Update API endpoints in `config.h` if needed

5. Select **M5Stack Dial** as your board

6. Upload to your M5Stack Dial device

## 📡 OTA Updates

### Triggering an Update

1. Scan RFID card: **`04B6C5D65F6180`**
2. Device connects to update WiFi
3. Downloads firmware from: `https://raw.githubusercontent.com/mawalton910/buystation/main/buyStation_M5.ino.bin`
4. Installs and reboots automatically

### Deploying New Firmware

1. **Export Compiled Binary**:
   - Arduino IDE → Sketch → Export Compiled Binary
   
2. **Rename the file** to: `buyStation_M5.ino.bin`

3. **Upload to GitHub**:
   - Place file in the **root** of the `main` branch
   - Commit and push changes

4. **Deploy**:
   - Scan the OTA trigger card on any device
   - All devices will update to the new version

### OTA Configuration

Configure in `OTAUpdate.h`:
```cpp
// OTA Trigger Card
const String OTA_TRIGGER_UIDS[] = {
    "04B6C5D65F6180"  // Add more trigger cards as needed
};

// Update WiFi (can be different from normal WiFi)
const char* OTA_WIFI_SSID = "YourUpdateWiFi";
const char* OTA_WIFI_PASSWORD = "YourPassword";

// GitHub Settings
const char* GITHUB_USER = "mawalton910";
const char* GITHUB_REPO = "buystation";
const char* FIRMWARE_FILENAME = "buyStation_M5.ino.bin";
String targetBranch = "main";
```

## 🎯 Usage

### Admin Access

Scan any admin badge to enter admin mode:
- `23859FFE`
- `BF1D2A40`
- `4F51D264`
- `43D6ACFE`

### Device Modes

Navigate admin menu using rotary encoder:
- **Rotate**: Scroll through options
- **Press**: Select option

Available modes:
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

API endpoints configured in `ApiClient.h`.

## 🐛 Troubleshooting

### WiFi Connection Issues
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check SSID and password in `secrets.h`
- Scan admin badge → WiFi Status to diagnose
- Use "WiFi Reconnect" option in admin menu

### OTA Update Failures
- **"WIFI FAILED"**: Check OTA WiFi credentials in `OTAUpdate.h`
- **"UPDATE FAILED"**: Verify `.bin` file exists at GitHub URL
- **"NO UPDATE AVAILABLE"**: Check branch name and filename
- Test URL in browser: Should download the `.bin` file

### RFID Read Issues
- Ensure badge is close to device
- Check Serial Monitor (115200 baud) for UID readings
- Verify badge UID matches configured values
- Allow debounce time between scans (2 seconds default)

## 📊 Serial Monitor

Connect at **115200 baud** for debug output:
- WiFi connection status
- RFID card UIDs
- API request/response logs
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

**Current Version**: BuyStation 25.3.0  
**Last Updated**: December 30, 2025  
**Maintained by**: mawalton910
