/*
 * ========================================
 * ADMIN MODE CODE EXTRACTION
 * For M5 Dial NFC Device
 * ========================================
 * 
 * This file contains all the admin mode functionality
 * extracted from the relay2_M5 project.
 * 
 * FEATURES INCLUDED:
 * - Device Info Display
 * - WiFi Status & Diagnostics
 * - WiFi Reconnect
 * - WiFi Scan & Connect (with password entry)
 * - Loot Lock/Unlock
 * - Operational Mode Switching (Buy Station/Relay)
 * - Admin Badge Access System
 * - Rotary Encoder Navigation
 * - Full Admin Menu System
 * 
 * HARDWARE: M5 Dial (ESP32-S3, 1.28" circular TFT, NFC, Rotary Encoder)
 * 
 * ========================================
 */

// ============================================
// 1. CONFIG.H - ADMIN CONFIGURATION
// ============================================

// Admin badge configuration
#define ADMIN_BADGE_UID "43D6ACFE"  // Replace with your actual admin badge UID

// Device operational modes
enum OperationalMode {
    MODE_BUY_STATION,    // Normal player/loot transaction mode
    MODE_RELAY           // Badge scanning load mode for game integration
};

// Admin menu navigation
#define ADMIN_MENU_ITEMS 8              // Number of menu items
#define ADMIN_MENU_SCROLL_DELAY 200     // Delay between menu items when scrolling
#define ADMIN_EXIT_TIMEOUT 30000        // Auto-exit admin after 30s of inactivity

// Card read debounce
#define CARD_READ_DEBOUNCE 2000

// WiFi reconnect delay
#define WIFI_RECONNECT_DELAY 1000


// ============================================
// 2. STATE MACHINE - ADMIN STATES
// ============================================

enum ScanState {
    WAIT_FOR_PLAYER,
    WAIT_FOR_LOOT,
    CONFIRM_TRANSACTION,
    ADMIN_MODE,            // Main admin menu
    ADMIN_WIFI_STATUS,     // WiFi diagnostics screen
    ADMIN_LOOT_FLIP,       // Loot lock/unlock screen
    ADMIN_WIFI_SCAN,       // WiFi network list screen
    ADMIN_WIFI_PASSWORD    // WiFi password entry screen
};


// ============================================
// 3. GLOBAL VARIABLES FOR ADMIN MODE
// ============================================

// Admin mode variables
int adminMenuSelection = 0;
long encoderPosition = 0;
String relayLastUuid = "";
String relayLastResponse = "";
bool relayLastSuccess = false;
OperationalMode deviceMode = MODE_BUY_STATION;  // Default to buy station mode
String lootFlipLastUuid = "";
String lootFlipLastStatus = "";

// WiFi scan and connect variables
int wifiNetworkCount = 0;
int selectedNetworkIndex = 0;
String scannedSSIDs[20];  // Store up to 20 networks
int scannedRSSI[20];
bool scannedEncrypted[20];
String enteredPassword = "";
int passwordCursorPos = 0;
const char charSet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%&*-_+=.,?<DEL><DONE>";
int charSetSize = 76;  // Total characters including <DEL> and <DONE>
int currentCharIndex = 0;


// ============================================
// 4. ADMIN HELPER FUNCTIONS
// ============================================

// Check if scanned UUID is the admin badge
bool isAdminBadge(String uuid) {
    return uuid == String(ADMIN_BADGE_UID);
}

// WiFi reconnect function
void reconnectWiFi() {
    Serial.println("WiFi retry requested...");
    displayWiFiReconnecting();
    WiFi.disconnect();
    delay(WIFI_RECONNECT_DELAY);
    initializeWiFi();  // Call your WiFi initialization function
}


// ============================================
// 5. MAIN LOOP - ADMIN STATE HANDLING
// ============================================

/*
 * Place this code in your main loop() function
 * Use switch/case to handle different admin states
 */

void loop() {
    M5Dial.update();
    
    // ... your other state handling ...
    
    switch (currentState) {
        
        // ========== ADMIN MODE MENU ==========
        case ADMIN_MODE: {
            // Admin menu navigation with rotary encoder
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                int delta = (newPosition - encoderPosition) / 4; // Divide by 4 for smoother scrolling
                if (delta != 0) {
                    adminMenuSelection += delta;
                    if (adminMenuSelection < 0) adminMenuSelection = 0;
                    if (adminMenuSelection > 7) adminMenuSelection = 7; // 0-7: 8 menu items
                    displayAdminMenu(adminMenuSelection, deviceMode);
                    lastActivityTime = millis();
                    encoderPosition = newPosition;
                }
            }
            
            // Button press to select menu item
            if (M5Dial.BtnA.wasPressed()) {
                lastActivityTime = millis();
                
                switch(adminMenuSelection) {
                    case 0: { // Device Info
                        Serial.println("Admin: Device Info");
                        displayDeviceInfo(serialNum, deviceMode);
                        // Wait for button press to return to menu OR admin badge to exit
                        bool deviceInfoActive = true;
                        while (deviceInfoActive) {
                            M5Dial.update();
                            // Button press returns to menu
                            if (M5Dial.BtnA.wasPressed()) {
                                deviceInfoActive = false;
                            }
                            // Admin badge exits admin completely
                            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
                                String scannedUuid = sanitizeUuid(readNFCCardUID());
                                if (isAdminBadge(scannedUuid)) {
                                    Serial.println("Admin badge - exiting admin mode");
                                    if (deviceMode == MODE_BUY_STATION) {
                                        currentState = WAIT_FOR_PLAYER;
                                        displayBlackMarketScan();  // Your main screen
                                    } else {
                                        relayLastUuid = "";
                                        relayLastResponse = "";
                                        relayLastSuccess = false;
                                        displayRelayMode();
                                        currentState = WAIT_FOR_PLAYER;
                                    }
                                    return; // Exit loop() entirely
                                }
                                delay(500);
                            }
                            delay(50);
                        }
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    }
                    
                    case 1: // WiFi Status
                        Serial.println("Admin: WiFi Status");
                        currentState = ADMIN_WIFI_STATUS;
                        displayWiFiStatus();
                        break;
                    
                    case 2: // WiFi Reconnect
                        Serial.println("Admin: WiFi Reconnect");
                        reconnectWiFi();
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    
                    case 3: // WiFi Scan/Connect
                        Serial.println("Admin: WiFi Scan & Connect");
                        currentState = ADMIN_WIFI_SCAN;
                        // Start WiFi scan
                        displayWiFiScanning();
                        wifiNetworkCount = WiFi.scanNetworks();
                        // Store scan results
                        for (int i = 0; i < wifiNetworkCount && i < 20; i++) {
                            scannedSSIDs[i] = WiFi.SSID(i);
                            scannedRSSI[i] = WiFi.RSSI(i);
                            scannedEncrypted[i] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                        }
                        selectedNetworkIndex = 0;
                        encoderPosition = M5Dial.Encoder.read();
                        displayWiFiNetworkList(scannedSSIDs, scannedRSSI, scannedEncrypted, wifiNetworkCount, selectedNetworkIndex);
                        break;
                    
                    case 4: // Loot Lock/Unlock
                        Serial.println("Admin: Loot Lock Mode");
                        currentState = ADMIN_LOOT_FLIP;
                        lootFlipLastUuid = "";
                        lootFlipLastStatus = "";
                        displayLootFlipMode();
                        break;
                    
                    case 5: // Buy Station Mode
                        Serial.println("Admin: Switching to Buy Station Mode");
                        deviceMode = MODE_BUY_STATION;
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    
                    case 6: // Relay Mode
                        Serial.println("Admin: Switching to Relay Mode");
                        deviceMode = MODE_RELAY;
                        // Immediately exit to relay mode
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER;
                        break;
                    
                    case 7: // Exit Admin
                        Serial.println("Admin: Exiting");
                        if (deviceMode == MODE_BUY_STATION) {
                            currentState = WAIT_FOR_PLAYER;
                            displayBlackMarketScan();  // Your main screen
                        } else {
                            relayLastUuid = "";
                            relayLastResponse = "";
                            relayLastSuccess = false;
                            displayRelayMode();
                        }
                        break;
                }
                lastCardReadTime = millis();
            }
            
            // Check for admin badge scan to exit
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    if (deviceMode == MODE_BUY_STATION) {
                        currentState = WAIT_FOR_PLAYER;
                        displayBlackMarketScan();
                    } else {
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                    }
                }
                lastCardReadTime = millis();
            }
            break;
        }
        
        // ========== ADMIN WIFI STATUS ==========
        case ADMIN_WIFI_STATUS:
            // Button returns to menu
            if (M5Dial.BtnA.wasPressed()) {
                currentState = ADMIN_MODE;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastCardReadTime = millis();
            }
            
            // Admin badge exits admin completely
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    if (deviceMode == MODE_BUY_STATION) {
                        currentState = WAIT_FOR_PLAYER;
                        displayBlackMarketScan();
                    } else {
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER;
                    }
                }
                lastCardReadTime = millis();
            }
            break;
        
        // ========== ADMIN LOOT FLIP ==========
        case ADMIN_LOOT_FLIP:
            // Button returns to menu
            if (M5Dial.BtnA.wasPressed()) {
                currentState = ADMIN_MODE;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastCardReadTime = millis();
            }
            
            // Scan loot badge to flip lock status
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                // Check if admin badge - exit completely
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    if (deviceMode == MODE_BUY_STATION) {
                        currentState = WAIT_FOR_PLAYER;
                        displayBlackMarketScan();
                    } else {
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER;
                    }
                } else {
                    // Regular badge - flip loot status
                    lootFlipLastUuid = scannedUuid;
                    Serial.println("Flipping loot: " + scannedUuid);
                    
                    // Show loading
                    displayCenteredText("FLIPPING...", THEME_ACCENT_COLOR, BLACK, 2);
                    
                    // Call flip function (implement your own API call)
                    lootFlipLastStatus = flipLootByUUID(scannedUuid);
                    
                    // Update display
                    displayLootFlipMode(lootFlipLastUuid, lootFlipLastStatus);
                }
                
                lastCardReadTime = millis();
            }
            break;
        
        // ========== ADMIN WIFI SCAN ==========
        case ADMIN_WIFI_SCAN: {
            // Handle encoder rotation to select network
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                int delta = (newPosition - encoderPosition) / 4;
                selectedNetworkIndex += delta;
                selectedNetworkIndex = constrain(selectedNetworkIndex, 0, wifiNetworkCount - 1);
                encoderPosition = newPosition;
                displayWiFiNetworkList(scannedSSIDs, scannedRSSI, scannedEncrypted, wifiNetworkCount, selectedNetworkIndex);
            }
            
            // Button to select network and enter password
            if (M5Dial.BtnA.wasPressed()) {
                if (wifiNetworkCount > 0) {
                    String selectedSSID = scannedSSIDs[selectedNetworkIndex];
                    bool isEncrypted = scannedEncrypted[selectedNetworkIndex];
                    
                    Serial.println("Selected network: " + selectedSSID);
                    
                    if (isEncrypted) {
                        // Move to password entry
                        currentState = ADMIN_WIFI_PASSWORD;
                        enteredPassword = "";
                        currentCharIndex = 0;
                        encoderPosition = M5Dial.Encoder.read();
                        displayPasswordEntry(selectedSSID, enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
                    } else {
                        // Connect to open network
                        displayWiFiConnecting(selectedSSID);
                        WiFi.disconnect();
                        delay(500);
                        WiFi.begin(selectedSSID.c_str());
                        
                        // Wait for connection
                        int attempts = 0;
                        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                            delay(500);
                            attempts++;
                        }
                        
                        bool success = (WiFi.status() == WL_CONNECTED);
                        String message = success ? "IP: " + WiFi.localIP().toString() : "Connection timeout";
                        displayWiFiConnectionResult(success, message);
                        
                        // Wait for button to continue
                        while (!M5Dial.BtnA.wasPressed()) {
                            M5Dial.update();
                            delay(10);
                        }
                        
                        currentState = ADMIN_MODE;
                        displayAdminMenu(adminMenuSelection, deviceMode);
                    }
                }
                lastCardReadTime = millis();
            }
            
            // Admin badge exits admin completely
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    if (deviceMode == MODE_BUY_STATION) {
                        currentState = WAIT_FOR_PLAYER;
                        displayBlackMarketScan();
                    } else {
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER;
                    }
                }
                lastCardReadTime = millis();
            }
            break;
        }
        
        // ========== ADMIN WIFI PASSWORD ENTRY ==========
        case ADMIN_WIFI_PASSWORD: {
            String selectedSSID = scannedSSIDs[selectedNetworkIndex];
            
            // Handle encoder rotation to scroll through characters
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                int delta = (newPosition - encoderPosition) / 4;
                currentCharIndex += delta;
                currentCharIndex = (currentCharIndex + charSetSize) % charSetSize;
                encoderPosition = newPosition;
                displayPasswordEntry(selectedSSID, enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
            }
            
            // Button to select character
            if (M5Dial.BtnA.wasPressed()) {
                if (currentCharIndex == charSetSize - 2) {
                    // DEL - delete last character
                    if (enteredPassword.length() > 0) {
                        enteredPassword.remove(enteredPassword.length() - 1);
                        displayPasswordEntry(selectedSSID, enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
                    }
                } else if (currentCharIndex == charSetSize - 1) {
                    // DONE - attempt connection
                    Serial.println("Connecting to: " + selectedSSID);
                    Serial.println("Password length: " + String(enteredPassword.length()));
                    
                    displayWiFiConnecting(selectedSSID);
                    WiFi.disconnect();
                    delay(500);
                    WiFi.begin(selectedSSID.c_str(), enteredPassword.c_str());
                    
                    // Wait for connection
                    int attempts = 0;
                    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                        delay(500);
                        attempts++;
                    }
                    
                    bool success = (WiFi.status() == WL_CONNECTED);
                    String message = success ? "IP: " + WiFi.localIP().toString() : "Check password";
                    displayWiFiConnectionResult(success, message);
                    
                    // Wait for button to continue
                    while (!M5Dial.BtnA.wasPressed()) {
                        M5Dial.update();
                        delay(10);
                    }
                    
                    currentState = ADMIN_MODE;
                    displayAdminMenu(adminMenuSelection, deviceMode);
                } else {
                    // Regular character - add to password
                    enteredPassword += charSet[currentCharIndex];
                    displayPasswordEntry(selectedSSID, enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
                }
                lastCardReadTime = millis();
            }
            
            // Admin badge exits admin completely
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    if (deviceMode == MODE_BUY_STATION) {
                        currentState = WAIT_FOR_PLAYER;
                        displayBlackMarketScan();
                    } else {
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER;
                    }
                }
                lastCardReadTime = millis();
            }
            break;
        }
    }
}


// ============================================
// 6. ADMIN BADGE ACCESS TRIGGER
// ============================================

/*
 * Place this code in your WAIT_FOR_PLAYER and WAIT_FOR_LOOT states
 * to allow admin badge access from anywhere in the application
 */

// Example: In your player/loot scan code
if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
    String scannedUuid = sanitizeUuid(readNFCCardUID());
    
    // Check for admin badge FIRST before anything else
    if (isAdminBadge(scannedUuid)) {
        Serial.println("Admin badge detected!");
        displayAdminAccessGranted();  // Show access animation
        currentState = ADMIN_MODE;
        adminMenuSelection = 0;
        encoderPosition = M5Dial.Encoder.read();
        displayAdminMenu(adminMenuSelection, deviceMode);
        lastActivityTime = millis();
        lastCardReadTime = millis();
        return;  // Skip rest of badge processing
    }
    
    // ... rest of your normal badge processing code ...
}


// ============================================
// 7. DISPLAY FUNCTIONS (M5DialControl.h)
// ============================================

// See attached file M5DialControl_ADMIN_DISPLAYS.h
// This contains all 12 display functions:
// - displayDeviceInfo()
// - displayAdminMenu()
// - displayWiFiStatus()
// - displayWiFiReconnecting()
// - displayRelayMode()
// - displayLootFlipMode()
// - displayAdminAccessGranted()
// - displayWiFiScanning()
// - displayWiFiNetworkList()
// - displayPasswordEntry()
// - displayWiFiConnecting()
// - displayWiFiConnectionResult()


// ============================================
// USAGE NOTES
// ============================================

/*
 * TO USE THIS CODE IN YOUR PROJECT:
 * 
 * 1. Copy the configuration section to your config.h
 * 2. Add the state enum values to your existing enum
 * 3. Add the global variables to your main file
 * 4. Copy the admin state handling cases to your loop()
 * 5. Add the admin badge check trigger to your badge scan code
 * 6. Copy all display functions to your display library
 * 7. Update ADMIN_BADGE_UID with your actual admin badge
 * 8. Customize colors and layout for your screen
 * 
 * CONTROL SCHEME:
 * - Rotary Encoder: Navigate menus/scroll characters
 * - Button Press: Select menu item/confirm action
 * - Admin Badge: Exit admin mode completely
 * 
 * FEATURES:
 * - Device Info: Serial, MAC, Mode
 * - WiFi Status: Connection details, signal strength
 * - WiFi Reconnect: Force reconnect
 * - WiFi Scan: Find and connect to networks on the fly
 * - Password Entry: Full character input with rotary encoder
 * - Loot Lock: Toggle lock status via API
 * - Mode Switch: Buy Station ↔ Relay mode
 * 
 * API FUNCTIONS NEEDED:
 * - flipLootByUUID(uuid) - Returns "LOCKED", "UNLOCKED", "NOT FOUND", or "ERROR"
 * - initializeWiFi() - Your WiFi connection initialization
 * - sanitizeUuid(uuid) - Remove spaces and uppercase
 * - displayBlackMarketScan() - Your main screen
 * - displayCenteredText() - Center text display utility
 * 
 */

// END OF ADMIN CODE EXTRACTION
