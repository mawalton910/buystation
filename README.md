# Guru Games - Buy Station
**Current Version**: BuyStation 26.1.0

A specialized IoT device system built on the M5Stack Dial (ESP32-S3) for managing player transactions, loot distribution, and game mechanics in live-action role-playing events.

## 🎮 What It Does

The Buy Station is an RFID-enabled device that revolutionizes LARP gameplay by providing:

### For Players
- **Purchase & Claim Loot**: Scan your badge to browse and claim available items
- **Loot Browsing**: Use the rotary dial to scroll through faction-specific rewards
- **Multi-Claim Sessions**: Claim multiple items in one session before timeout
- **Inventory Transfer**: Transfer loot between players seamlessly
- **Interactive Maps**: View terrain maps with current location tracking
- **Quick Badge Updates**: Fast badge scanning for relay mode integration

### For Game Masters
- **WiFi Management**: Configure network settings and scan for connections
- **Device Monitoring**: View real-time device information and status
- **Loot Control**: Lock/unlock loot pools for game flow management
- **Mode Switching**: Toggle between different operational modes
- **Variable Management**: View and clear stored game variables
- **Remote Updates**: Push firmware updates to all devices simultaneously

## 🔧 Hardware

- **M5Stack Dial** (ESP32-S3) with built-in RFID reader
- 240x240 round touchscreen display
- Rotary encoder with button
- WiFi connectivity for server integration

## 📦 Operational Modes

### 1. Buy Station 2 Mode (Default)
Multi-claim loot system where players can:
- Scan badge to load available loot
- Browse items using rotary encoder
- Claim multiple items in one session
- Auto-timeout protection with session management

### 2. Backpack Mode
Simple claim and unlock system for loot items with badge-based inventory management.

### 3. Loot Transfer Mode
Transfer inventory items between different player factions with validation and tracking.

### 4. Relay Mode
Quick badge scanning mode for game integration and badge status updates.

### 5. Map Mode
Display interactive terrain maps with location markers for navigation during gameplay.

## 🎯 How Players Use It

1. **Scan Your Badge** → System identifies you and loads your available loot
2. **Browse Items** → Rotate the dial to see what's available
3. **Claim Items** → Press the button to claim the item you want
4. **Repeat** → Continue claiming items until you're done
5. **Auto-Save** → Session ends automatically, all claims are saved

## 🛠️ Administrative Features

Game masters can access admin mode by scanning an authorized badge:
- Monitor device health and WiFi status
- Switch between operational modes
- Control loot availability
- Update firmware remotely
- View system logs and variables

## 📡 Over-the-Air Updates

Devices support remote firmware updates for seamless version management:
- Scan a designated trigger card to start update
- Device automatically downloads and installs new firmware
- Progress displayed with percentage and download statistics
- Automatic reboot after successful installation
- All devices can be updated simultaneously

## 🎮 Game Integration

The system integrates with backend servers to:
- Track player badge data in real-time
- Manage faction-based loot distribution
- Record transaction history
- Synchronize game state across all devices
- Provide analytics for game balance

## 🔐 Security

- Physical RFID badge required for all transactions
- Faction-based access control
- Session timeout protection
- Encrypted communication with backend servers
- Admin-only access for critical functions

## 🙏 Acknowledgments

Built with M5Stack Dial hardware, ESP32-S3 platform, and the Arduino framework.

---

**Current Version**: BuyStation 26.1.0
**Last Updated**: December 30, 2025
