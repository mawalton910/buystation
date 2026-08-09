// config.h - Configuration file for M5 Dial Loot System
#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// FIRMWARE VERSION
// ============================================
#define FIRMWARE_VERSION "BuyStation 26.2.0"

// ============================================
// SECURE WEBSOCKET / UNIVERSAL DIAL TRANSPORT
// ============================================
// Keeps the existing buy-station visuals/state machine but uses the authenticated
// /ws Universal Dial action channel for Buy Station 2 scans and claims.
#define USE_SECURE_BUYSTATION_WS 1
#define SECURE_BUYSTATION_WS_HOST "iot.rke.world"
#define SECURE_BUYSTATION_WS_PORT 443
#define SECURE_BUYSTATION_WS_PATH "/ws"
#define SECURE_BUYSTATION_RECONNECT_MS 2000UL
#define SECURE_BUYSTATION_REQUEST_TIMEOUT_MS 7000UL

// ============================================
// ERROR MESSAGE MAPPING
// ============================================
// Map server error messages to display messages
// Add your custom error mappings here
struct ErrorMessageMap {
    String serverMessage;    // Text to look for in server response
    String displayMessage;   // Message to show on screen
};

const ErrorMessageMap ERROR_MESSAGES[] = {
    {"IOT device not found", "IOT NOT\nFOUND"},
    {"Invalid UUID", "INVALID\nBADGE"},
    {"Database error", "SERVER\nERROR"},
    {"Unauthorized", "NOT\nAUTHORIZED"},
    {"Timeout", "REQUEST\nTIMEOUT"},
    {"Player not found", "PLAYER NOT\nFOUND"},
    {"Item not found", "ITEM NOT\nFOUND"},
    {"already claimed", "GREEDY!\nALREADY\nCLAIMED"},
    // Add more mappings as needed
};
const int ERROR_MESSAGES_COUNT = sizeof(ERROR_MESSAGES) / sizeof(ERROR_MESSAGES[0]);

// ============================================
// NFC CONFIGURATION
// ============================================
// Special NFC tag UIDs for WiFi retry (add your actual tag UIDs here)
// Any card with faction "ORANGE" from server will also trigger WiFi retry
const String WIFI_RETRY_TAGS[] = {
    "04B3452A0A1290",      // Example tag 1
    "0FA31A64"        // Example tag 2 - add more as needed
};
const int WIFI_RETRY_TAGS_COUNT = sizeof(WIFI_RETRY_TAGS) / sizeof(WIFI_RETRY_TAGS[0]);

// Admin badge configuration
// Multiple admin badges supported
const String ADMIN_BADGE_UIDS[] = {
    "23859FFE",
    "BF1D2A40",
    "4F51D264",
    "43D6ACFE",
    "CDC0AA82",
    "F35D2503",
    "13F02F04",
    "D1AC67A9",
    "D3571FFF",
    "1F69D964",
    "63102FFF",
    "CBF51555",
    "041207D75F6180",
    "6E64595B"
};
const int ADMIN_BADGE_UIDS_COUNT = sizeof(ADMIN_BADGE_UIDS) / sizeof(ADMIN_BADGE_UIDS[0]);

// Card read debounce time in milliseconds
#define CARD_READ_DEBOUNCE 2000

// Maximum loot items the firmware can hold in a local cart/session.
// Creator/backend backpack settings should enforce the player-facing limit.
#define MAX_LOOT_CART_ITEMS 50

// ============================================
// ADMIN MODE CONFIGURATION
// ============================================
// Device operational modes
enum OperationalMode {
    MODE_BUY_STATION,    // Normal player/loot transaction mode
    MODE_BUY_STATION_2,  // New loot backpack system with multiple claims
    MODE_BACKPACK,       // Backpack mode - claim loot with session capacity
    MODE_LOOT_TRANSFER,  // Loot Transfer mode - transfer inventory between factions
    MODE_RELAY,          // Badge scanning load mode for game integration
    MODE_PHOTOBOOTH,     // Photobooth reader mode - posts scans to IoT last_uuid
    MODE_MAP             // Map display mode - shows terrain map with current location
};

// Admin menu navigation
#define ADMIN_MENU_ITEMS 15             // Number of menu items (Device Info, WiFi Status, WiFi Reconnect, WiFi Scan/Connect, Loot Lock, Backpack, Buy Station 2, Loot Transfer, Relay Mode, Photobooth Mode, Map Mode, Dev Mode, View Variables, Clear Variables, Exit)
#define ADMIN_MENU_SCROLL_DELAY 200     // Delay between menu items when scrolling
#define ADMIN_EXIT_TIMEOUT 30000        // Auto-exit admin after 30s of inactivity

// Relay mode settings
#define RELAY_MODE_TIMEOUT 60000        // Exit relay mode after 60s of inactivity
#define RELAY_SCAN_DEBOUNCE 200         // Fast debounce for relay mode (200ms)
#define RELAY_DUPLICATE_SUPPRESS_MS 3000 // Ignore the same UUID from the same reader for 3s

// ============================================
// MAP MODE CONFIGURATION
// ============================================
// Define location presets that can be triggered by NFC tags
struct MapLocation {
    String tagUid;    // NFC tag UID
    String name;      // Location name
    double lat;       // Latitude
    double lon;       // Longitude
    int zoom;         // Zoom level
};

// Add your location NFC tags here
const MapLocation MAP_LOCATIONS[] = {
    {"LOCATION1TAG", "Base Camp", 41.6573364, -86.7609855, 14},
    {"LOCATION2TAG", "Outpost Alpha", 41.6600000, -86.7700000, 15},
    {"LOCATION3TAG", "Supply Depot", 41.6550000, -86.7550000, 16},
    // Add more locations as needed
};
const int MAP_LOCATIONS_COUNT = sizeof(MAP_LOCATIONS) / sizeof(MAP_LOCATIONS[0]);

// ============================================
// TIMING CONFIGURATION
// ============================================
// Startup animation settings
#define STARTUP_ANIMATION_ENABLED true  // Set to false to disable startup animation
#define STARTUP_ANIMATION_TYPE 1        // 1=Spinning Circle, 2=Pulsing, 3=Rainbow Cycle, 4=Custom Logo Spin, 5=GTA Style Text
#define STARTUP_ANIMATION_SPEED 30      // Animation frame delay (ms) - lower is faster, 20-30ms recommended for smooth motion
// Note: STARTUP_ANIMATION_COLOR is now defined below as THEME_ACCENT_COLOR after theme selection
#define STARTUP_LOGO_SIZE 40            // Size of the custom logo shape
#define STARTUP_LOGO_COLOR 0x07FF       // Cyan - change to your brand color
#define INIT_ANIMATION_FRAMES 200       // Total frames for entire init animation sequence

// GTA-Style text animation colors (Vice City themed)
#define GTA_TEXT_COLOR_TOP 0xF81F      // Hot Pink/Magenta (top of letters)
#define GTA_TEXT_COLOR_BOTTOM 0x07FF   // Cyan/Blue (bottom of letters)
#define GTA_OUTLINE_COLOR 0x0000       // Black outline
#define GTA_BACKGROUND_COLOR WHITE     // White background

// Boot sequence animation settings
#define BOOT_COLOR_CYCLE_COUNT 20       // Number of color cycles on boot
#define BOOT_COLOR_CYCLE_DELAY 800      // Delay between colors (ms)
#define BOOT_COLOR_OFF_DELAY 50         // Delay when screen is off (ms)

// Success blink animation
#define SUCCESS_BLINK_COUNT 4           // Number of blinks
#define SUCCESS_BLINK_ON_DELAY 250      // On duration (ms)
#define SUCCESS_BLINK_OFF_DELAY 250     // Off duration (ms)

// Failure blink animation
#define FAIL_BLINK_COUNT 3              // Number of blinks
#define FAIL_BLINK_ON_DELAY 500         // On duration (ms)
#define FAIL_BLINK_OFF_DELAY 500        // Off duration (ms)

// Badge swipe animation
#define BADGE_SWIPE_CYCLE_COUNT 10      // Number of color cycles
#define BADGE_SWIPE_ON_DELAY 50         // On duration (ms)
#define BADGE_SWIPE_OFF_DELAY 25        // Off duration (ms)

// Display delays
#define PLAYER_INFO_DISPLAY_TIME 2000   // Time to show player info (ms)
#define LOOT_INFO_DISPLAY_TIME 2000     // Time to show loot info (ms)
#define PLAYER_LOOT_TOGGLE_TIME 1500    // Time for each when toggling (ms)
#define SUCCESS_MESSAGE_DELAY 2000      // Success message display time (ms)
#define FAIL_MESSAGE_DELAY 2000         // Fail message display time (ms)
#define WIFI_RECONNECT_DELAY 1000       // Delay before WiFi reconnect (ms)
#define WIFI_CONNECTED_DELAY 1000       // Delay after WiFi connects (ms)
#define WIFI_FAILED_DELAY 2000          // Delay after WiFi fails (ms)

// WiFi connection timeout
#define WIFI_CONNECT_ATTEMPTS 2000      // Max attempts (2000 * 30ms = 60 seconds)
#define WIFI_CONNECT_RETRY_DELAY 500    // Delay between attempts (ms) - not used during init, only for runtime reconnects
#define WIFI_RECONNECT_BASE_DELAY 500    // Base delay for reconnect backoff (ms)
#define WIFI_RECONNECT_MAX_DELAY 8000    // Max backoff delay (ms)

// TLS/HTTPS readiness
#define TLS_READY_DELAY_MS 800           // Wait after WiFi connect before first HTTPS (ms)
#define HTTP_REQUEST_TIMEOUT_MS 5000     // Default HTTP timeout for API calls (ms)

// Telemetry / heartbeat
#define TELEMETRY_INTERVAL_MS 10000      // Interval between health heartbeats (ms)
#define TELEMETRY_MIN_RSSI_UPDATE_MS 10000 // Min interval to refresh RSSI (ms)
#define TELEMETRY_DEBUG 0                // Set to 1 for verbose heartbeat logging

// Display power saving
#define SCREEN_TIMEOUT_MS 45000          // Turn off screen after inactivity (ms)

// ============================================
// THEME SYSTEM
// ============================================
// Select your theme by changing ACTIVE_THEME value:
// 1 = BLACK_MARKET (dark with green accents)
// 2 = CYBERPUNK (dark with cyan/magenta accents)
// 3 = MILITARY (dark with amber/orange accents)
// 4 = CLASSIC (black and white, simple)
// 5 = NEON (dark with bright neon colors)
// 6 = GURU_GAMES (black and orange branded theme)
#define ACTIVE_THEME 6

// Theme structure
struct Theme {
    // Scan screen theme
    uint32_t accentColor;           // Primary accent color for borders, highlights
    uint32_t scanTextColor;         // Text color for scan prompts
    uint32_t scanBgColor;           // Background for scan screens
    
    // Info screens
    uint32_t playerInfoBg;          // Player info background
    uint32_t playerInfoText;        // Player info text
    uint32_t lootInfoBg;            // Loot info background
    uint32_t lootInfoText;          // Loot info text
    
    // Confirmation screen
    uint32_t confirmBg;             // Confirmation background
    uint32_t confirmText;           // Confirmation text
    
    // Loading screens
    uint32_t loadingBg;             // Loading background
    uint32_t loadingText;           // Loading text/animation color
    
    // Status colors
    uint32_t sendingText;           // "Sending..." text
    uint32_t successText;           // "Success!" text
    uint32_t failText;              // "Failed" text
    
    // WiFi indicator
    uint32_t wifiConnected;         // WiFi connected indicator
    uint32_t wifiDisconnected;      // WiFi disconnected indicator
    uint32_t wifiOutline;           // WiFi indicator outline
    
    // Error screens (typically consistent across themes)
    uint32_t errorText;             // Error message text
    uint32_t errorBg;               // Error background
};

// Theme definitions
const Theme THEME_BLACK_MARKET = {
    0x07E0,  // accentColor - Green
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0x07E0,  // playerInfoText - Green
    0x0000,  // lootInfoBg - Black
    0x07E0,  // lootInfoText - Green
    0x0000,  // confirmBg - Black
    0x07E0,  // confirmText - Green
    0x0000,  // loadingBg - Black
    0x07E0,  // loadingText - Green
    0x07E0,  // sendingText - Green
    0x07E0,  // successText - Green
    0xFFFF,  // failText - White
    0x07E0,  // wifiConnected - Green
    0xF800,  // wifiDisconnected - Red
    0x07E0,  // wifiOutline - Green
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

const Theme THEME_CYBERPUNK = {
    0xF81F,  // accentColor - Magenta
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0x07FF,  // playerInfoText - Cyan
    0x0000,  // lootInfoBg - Black
    0xF81F,  // lootInfoText - Magenta
    0x0000,  // confirmBg - Black
    0xF81F,  // confirmText - Magenta
    0x0000,  // loadingBg - Black
    0x07FF,  // loadingText - Cyan
    0xF81F,  // sendingText - Magenta
    0x07FF,  // successText - Cyan
    0xFFFF,  // failText - White
    0x07FF,  // wifiConnected - Cyan
    0xF800,  // wifiDisconnected - Red
    0xF81F,  // wifiOutline - Magenta
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

const Theme THEME_MILITARY = {
    0xFD20,  // accentColor - Orange/Amber
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0xFD20,  // playerInfoText - Orange
    0x0000,  // lootInfoBg - Black
    0xFD20,  // lootInfoText - Orange
    0x0000,  // confirmBg - Black
    0xFD20,  // confirmText - Orange
    0x0000,  // loadingBg - Black
    0xFD20,  // loadingText - Orange
    0xFD20,  // sendingText - Orange
    0x07E0,  // successText - Green
    0xF800,  // failText - Red
    0x07E0,  // wifiConnected - Green
    0xF800,  // wifiDisconnected - Red
    0xFD20,  // wifiOutline - Orange
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

const Theme THEME_CLASSIC = {
    0xFFFF,  // accentColor - White
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0xFFFF,  // playerInfoText - White
    0x0000,  // lootInfoBg - Black
    0xFFFF,  // lootInfoText - White
    0x0000,  // confirmBg - Black
    0xFFFF,  // confirmText - White
    0x0000,  // loadingBg - Black
    0xFFFF,  // loadingText - White
    0xFFFF,  // sendingText - White
    0xFFFF,  // successText - White
    0xFFFF,  // failText - White
    0x07E0,  // wifiConnected - Green
    0xF800,  // wifiDisconnected - Red
    0xFFFF,  // wifiOutline - White
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

const Theme THEME_NEON = {
    0x07FF,  // accentColor - Cyan
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0xFFE0,  // playerInfoText - Yellow
    0x0000,  // lootInfoBg - Black
    0x07FF,  // lootInfoText - Cyan
    0x0000,  // confirmBg - Black
    0xF81F,  // confirmText - Magenta
    0x0000,  // loadingBg - Black
    0x07FF,  // loadingText - Cyan
    0xFFE0,  // sendingText - Yellow
    0x07E0,  // successText - Green
    0xF800,  // failText - Red
    0x07E0,  // wifiConnected - Green
    0xF800,  // wifiDisconnected - Red
    0x07FF,  // wifiOutline - Cyan
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

const Theme THEME_GURU_GAMES = {
    0xFD20,  // accentColor - Orange (brand color)
    0xFFFF,  // scanTextColor - White
    0x0000,  // scanBgColor - Black
    0x0000,  // playerInfoBg - Black
    0xFD20,  // playerInfoText - Orange
    0x0000,  // lootInfoBg - Black
    0xFD20,  // lootInfoText - Orange
    0x0000,  // confirmBg - Black
    0xFD20,  // confirmText - Orange
    0x0000,  // loadingBg - Black
    0xFD20,  // loadingText - Orange
    0xFD20,  // sendingText - Orange
    0xFD20,  // successText - Orange
    0xFFFF,  // failText - White
    0xFD20,  // wifiConnected - Orange (brand themed)
    0xF800,  // wifiDisconnected - Red
    0xFD20,  // wifiOutline - Orange
    0xF800,  // errorText - Red
    0xFFE0   // errorBg - Yellow
};

// Select active theme
#if ACTIVE_THEME == 1
    const Theme CURRENT_THEME = THEME_BLACK_MARKET;
#elif ACTIVE_THEME == 2
    const Theme CURRENT_THEME = THEME_CYBERPUNK;
#elif ACTIVE_THEME == 3
    const Theme CURRENT_THEME = THEME_MILITARY;
#elif ACTIVE_THEME == 4
    const Theme CURRENT_THEME = THEME_CLASSIC;
#elif ACTIVE_THEME == 5
    const Theme CURRENT_THEME = THEME_NEON;
#elif ACTIVE_THEME == 6
    const Theme CURRENT_THEME = THEME_GURU_GAMES;
#else
    const Theme CURRENT_THEME = THEME_BLACK_MARKET; // Default fallback
#endif

// ============================================
// COLOR CONFIGURATION (Using Active Theme)
// ============================================
// M5Stack 16-bit RGB565 Color Format:
// Use this tool to convert: http://www.barth-dev.de/online/rgb565-color-picker/
// Common colors: BLACK=0x0000, WHITE=0xFFFF, RED=0xF800, GREEN=0x07E0, 
// BLUE=0x001F, CYAN=0x07FF, MAGENTA=0xF81F, YELLOW=0xFFE0, ORANGE=0xFD20

// Player info screen colors (shown when player badge is scanned)
#define PLAYER_INFO_BG_COLOR CURRENT_THEME.playerInfoBg
#define PLAYER_INFO_TEXT_COLOR CURRENT_THEME.playerInfoText

// Loot/Item info screen colors (shown when loot badge is scanned)
#define LOOT_INFO_BG_COLOR CURRENT_THEME.lootInfoBg
#define LOOT_INFO_TEXT_COLOR CURRENT_THEME.lootInfoText

// Confirmation screen colors (shows player + item summary before confirming transaction)
#define CONFIRM_BG_COLOR CURRENT_THEME.confirmBg
#define CONFIRM_TEXT_COLOR CURRENT_THEME.confirmText

// Loading screen colors (shown during badge data fetch from server)
#define LOADING_BG_COLOR CURRENT_THEME.loadingBg
#define LOADING_TEXT_COLOR CURRENT_THEME.loadingText

// WiFi status indicator colors (small circle in top-right corner)
#define WIFI_CONNECTED_COLOR CURRENT_THEME.wifiConnected
#define WIFI_DISCONNECTED_COLOR CURRENT_THEME.wifiDisconnected
#define WIFI_INDICATOR_OUTLINE CURRENT_THEME.wifiOutline

// Prompt screen colors (main waiting screens for "Scan Badge" and "Scan Loot")
#define PROMPT_TEXT_COLOR CURRENT_THEME.scanTextColor
#define PROMPT_BG_COLOR CURRENT_THEME.scanBgColor
#define SCAN_LOOT_TEXT_COLOR CURRENT_THEME.scanTextColor
#define THEME_ACCENT_COLOR CURRENT_THEME.accentColor  // For themed UI elements like borders
#define STARTUP_ANIMATION_COLOR CURRENT_THEME.accentColor  // Startup animation uses theme accent color

// Status message colors (transaction sending/result messages)
#define SENDING_TEXT_COLOR CURRENT_THEME.sendingText
#define SUCCESS_TEXT_COLOR CURRENT_THEME.successText
#define FAIL_TEXT_COLOR CURRENT_THEME.failText
#define ERROR_TEXT_COLOR CURRENT_THEME.errorText
#define ERROR_BG_COLOR CURRENT_THEME.errorBg

// WiFi connection message colors (used during initialization and reconnection)
#define WIFI_CONNECTING_COLOR CURRENT_THEME.scanTextColor
#define WIFI_CONNECTED_COLOR CURRENT_THEME.successText
#define WIFI_FAILED_COLOR CURRENT_THEME.errorText


// ============================================
// TEXT CONFIGURATION
// ============================================
// Screen prompts
#define TEXT_SCAN_PLAYER "Scan \nBadge"
#define TEXT_SCAN_LOOT "Scan \nLoot"
#define TEXT_WIFI_RECONNECTING "WiFi\nReconnecting"
#define TEXT_INITIALIZING "Initializing..."
#define TEXT_STARTING "Starting..."
#define TEXT_CONNECTING_WIFI "Connecting\nWiFi..."
#define TEXT_INIT_NFC "Initializing\nNFC..."

// Loading messages
#define TEXT_LOADING_PLAYER "Loading Player..."
#define TEXT_LOADING_LOOT "Loading Loot..."
#define TEXT_LOADING "Loading..."

// WiFi messages
#define TEXT_WIFI_CONNECTING "WiFi..."
#define TEXT_WIFI_CONNECTED "Connected"
#define TEXT_WIFI_FAILED "WiFi Failed"
#define TEXT_WIFI_NOT_CONNECTED "No WiFi"

// Status messages
#define TEXT_SENDING "Sending..."
#define TEXT_SUCCESS "Success!"
#define TEXT_FAILED "Failed\n"
#define TEXT_ERROR "ERROR"
#define TEXT_READY "Ready"

// Display labels
#define TEXT_PLAYER_LABEL "PLAYER:"
#define TEXT_ITEM_LABEL "ITEM:"
#define TEXT_GETS_LABEL "gets"
#define TEXT_PRESS_TO "PRESS TO"
#define TEXT_CONFIRM "CONFIRM"

// Default names when data not found
#define TEXT_UNKNOWN_PLAYER "Unknown"
#define TEXT_UNKNOWN_ITEM "Unknown"
#define TEXT_PLAYER_PREFIX "Player "
#define TEXT_ITEM_PREFIX "Item "

// ============================================
// FONT/TEXT SIZE CONFIGURATION
// ============================================
#define DEFAULT_TEXT_SIZE 2
#define PLAYER_INFO_LABEL_SIZE 2
#define PLAYER_INFO_NAME_SIZE 2
#define PLAYER_INFO_FACTION_SIZE 1
#define LOOT_INFO_LABEL_SIZE 2
#define LOOT_INFO_NAME_SIZE 2
#define LOOT_INFO_TYPE_SIZE 1
#define CONFIRM_SUMMARY_SIZE 1
#define CONFIRM_BUTTON_SIZE 2

// ============================================
// WIFI INDICATOR CONFIGURATION
// ============================================
#define WIFI_INDICATOR_X_OFFSET 15      // Pixels from right edge
#define WIFI_INDICATOR_Y_OFFSET 15      // Pixels from top edge
#define WIFI_INDICATOR_RADIUS 8         // Circle radius
#define WIFI_INDICATOR_OUTLINE_WIDTH 1  // Outline thickness

// ============================================
// TOUCH BUTTON CONFIGURATION
// ============================================
// Confirm button touch area (relative to screen center)
#define CONFIRM_BUTTON_TOP_OFFSET 10    // Pixels below center
#define CONFIRM_BUTTON_BOTTOM_MARGIN 10 // Pixels from bottom
#define CONFIRM_BUTTON_LEFT_MARGIN 10   // Pixels from left
#define CONFIRM_BUTTON_RIGHT_MARGIN 10  // Pixels from right
#define CONFIRM_AUTO_CONFIRM_MS 30000   // Auto-confirm timeout on confirm screen

#endif // CONFIG_H
