/*
 * ========================================
 * ADMIN DISPLAY FUNCTIONS
 * For M5 Dial (240x240 circular TFT)
 * ========================================
 * 
 * All admin-related display functions extracted from M5DialControl.h
 * These functions render the admin UI on the M5 Dial circular screen
 * 
 * FUNCTIONS INCLUDED:
 * 1. displayDeviceInfo() - Device serial, MAC, mode
 * 2. displayAdminMenu() - 8-item menu with mode indicator
 * 3. displayWiFiStatus() - Connection status and details
 * 4. displayWiFiReconnecting() - Reconnection animation
 * 5. displayRelayMode() - Tag loading interface
 * 6. displayLootFlipMode() - Lock/unlock display
 * 7. displayAdminAccessGranted() - Entry animation
 * 8. displayWiFiScanning() - Network scan animation
 * 9. displayWiFiNetworkList() - Scrollable network list
 * 10. displayPasswordEntry() - Character input interface
 * 11. displayWiFiConnecting() - Connection progress
 * 12. displayWiFiConnectionResult() - Success/failure screen
 * 
 * DEPENDENCIES:
 * - M5Dial.h library
 * - WiFi.h library
 * - Theme colors (THEME_PRIMARY_COLOR, THEME_ACCENT_COLOR, etc.)
 * 
 * ========================================
 */

#ifndef M5DIALCONTROL_ADMIN_H
#define M5DIALCONTROL_ADMIN_H

#include <M5Dial.h>
#include <WiFi.h>

// Theme colors - customize for your project
#define THEME_PRIMARY_COLOR   0x39C5  // Teal
#define THEME_ACCENT_COLOR    0xFC00  // Orange
#define THEME_SUCCESS_COLOR   GREEN
#define THEME_ERROR_COLOR     RED
#define THEME_WARNING_COLOR   YELLOW
#define THEME_TEXT_COLOR      WHITE
#define THEME_BG_COLOR        BLACK

// Admin page indicator
void drawAdminPageIndicator(int currentPage, int totalPages) {
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_left);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    String pageText = String(currentPage) + "/" + String(totalPages);
    M5Dial.Display.drawString(pageText, 5, 5);
}


// ============================================
// 1. DEVICE INFO DISPLAY
// ============================================

void displayDeviceInfo(const String& serialNum, OperationalMode currentMode) {
    M5Dial.Display.clear();
    drawAdminPageIndicator(1, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("DEVICE INFO", 120, 30);
    
    // Serial number (truncate if too long)
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Serial:", 120, 70);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    String displaySerial = serialNum;
    if (displaySerial.length() > 16) {
        displaySerial = displaySerial.substring(0, 16) + "...";
    }
    M5Dial.Display.drawString(displaySerial, 120, 90);
    
    // MAC address (last 11 chars for readability)
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("MAC:", 120, 110);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    String shortMac = macAddress.substring(macAddress.length() - 11);  // Last 5 bytes (AA:BB:CC:DD:EE)
    M5Dial.Display.drawString(shortMac, 120, 130);
    
    // Firmware Version
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Firmware:", 120, 150);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.drawString(FIRMWARE_VERSION, 120, 170);
    
    // Operational Mode
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Mode:", 120, 190);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    String modeStr = (currentMode == MODE_BUY_STATION) ? "Buy Station" : "Relay";
    M5Dial.Display.drawString(modeStr, 120, 210);
    
    // Exit instructions
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Press to return", 120, 220);
}


// ============================================
// 2. ADMIN MENU - VISUAL ICON-BASED
// ============================================

// Draw icon for each menu item
void drawMenuIcon(int iconType, int centerX, int centerY, uint16_t color) {
    switch(iconType) {
        case 0: // Device Info - Monitor/Screen icon
            M5Dial.Display.drawRect(centerX - 20, centerY - 15, 40, 30, color);
            M5Dial.Display.drawRect(centerX - 18, centerY - 13, 36, 26, color);
            M5Dial.Display.fillRect(centerX - 5, centerY + 15, 10, 5, color);
            M5Dial.Display.fillRect(centerX - 8, centerY + 20, 16, 3, color);
            break;
            
        case 1: // WiFi Status - WiFi icon
            M5Dial.Display.fillCircle(centerX, centerY + 10, 3, color);
            M5Dial.Display.drawArc(centerX, centerY + 10, 8, 6, 200, 340, color);
            M5Dial.Display.drawArc(centerX, centerY + 10, 14, 12, 200, 340, color);
            M5Dial.Display.drawArc(centerX, centerY + 10, 20, 18, 200, 340, color);
            break;
            
        case 2: // WiFi Reconnect - Refresh icon
            M5Dial.Display.drawArc(centerX, centerY, 18, 15, 90, 270, color);
            M5Dial.Display.fillTriangle(centerX - 15, centerY - 20, centerX - 10, centerY - 25, centerX - 20, centerY - 25, color);
            M5Dial.Display.drawArc(centerX, centerY, 18, 15, 270, 90, color);
            M5Dial.Display.fillTriangle(centerX + 15, centerY + 20, centerX + 10, centerY + 25, centerX + 20, centerY + 25, color);
            break;
            
        case 3: // WiFi Scan - Magnifying glass
            M5Dial.Display.drawCircle(centerX - 5, centerY - 5, 12, color);
            M5Dial.Display.drawCircle(centerX - 5, centerY - 5, 10, color);
            M5Dial.Display.drawLine(centerX + 5, centerY + 5, centerX + 15, centerY + 15, color);
            M5Dial.Display.drawLine(centerX + 4, centerY + 6, centerX + 14, centerY + 16, color);
            M5Dial.Display.drawLine(centerX + 6, centerY + 4, centerX + 16, centerY + 14, color);
            break;
            
        case 4: // Loot Lock - Padlock icon
            M5Dial.Display.drawRect(centerX - 12, centerY, 24, 20, color);
            M5Dial.Display.fillRect(centerX - 10, centerY + 2, 20, 16, color);
            M5Dial.Display.drawArc(centerX, centerY, 10, 8, 180, 360, color);
            M5Dial.Display.drawArc(centerX, centerY, 8, 6, 180, 360, BLACK);
            M5Dial.Display.fillCircle(centerX, centerY + 10, 2, BLACK);
            break;
            
        case 5: // Backpack Claim+Lock - Army rucksack/duffle bag
            // Main duffle bag body (rounded rectangle)
            M5Dial.Display.fillRoundRect(centerX - 14, centerY - 6, 28, 18, 4, color);
            M5Dial.Display.drawRoundRect(centerX - 14, centerY - 6, 28, 18, 4, color);
            
            // Top flap/closure
            M5Dial.Display.fillRect(centerX - 12, centerY - 10, 24, 4, color);
            M5Dial.Display.drawRect(centerX - 12, centerY - 10, 24, 4, color);
            
            // Zipper line across top
            for (int i = -10; i < 10; i += 3) {
                M5Dial.Display.drawPixel(centerX + i, centerY - 8, BLACK);
            }
            
            // Straps/handles on sides
            M5Dial.Display.drawLine(centerX - 16, centerY - 2, centerX - 16, centerY + 6, color);
            M5Dial.Display.drawLine(centerX + 16, centerY - 2, centerX + 16, centerY + 6, color);
            M5Dial.Display.drawArc(centerX - 16, centerY - 2, 3, 2, 180, 360, color);
            M5Dial.Display.drawArc(centerX + 16, centerY - 2, 3, 2, 180, 360, color);
            
            // Pocket detail on front
            M5Dial.Display.drawRect(centerX - 8, centerY - 2, 16, 8, BLACK);
            
            // Small lock overlay at bottom
            M5Dial.Display.fillRect(centerX - 4, centerY + 8, 8, 5, BLACK);
            M5Dial.Display.drawRect(centerX - 4, centerY + 8, 8, 5, color);
            M5Dial.Display.drawArc(centerX, centerY + 8, 3, 2, 180, 360, color);
            break;
            
        case 6: // Buy Station - Shopping cart icon (bold)
            // Basket
            M5Dial.Display.drawRect(centerX - 10, centerY - 6, 22, 12, color);
            M5Dial.Display.drawLine(centerX - 10, centerY - 6, centerX - 14, centerY - 12, color);
            M5Dial.Display.drawLine(centerX - 14, centerY - 12, centerX - 6, centerY - 12, color);
            // Wheels
            M5Dial.Display.fillCircle(centerX - 6, centerY + 10, 2, color);
            M5Dial.Display.fillCircle(centerX + 6, centerY + 10, 2, color);
            // Basket details
            M5Dial.Display.drawLine(centerX - 6, centerY - 4, centerX + 8, centerY - 4, color);
            M5Dial.Display.drawLine(centerX - 6, centerY - 1, centerX + 8, centerY - 1, color);
            break;
            
        case 7: // Relay - Antenna/Tower icon
            M5Dial.Display.fillRect(centerX - 2, centerY - 20, 4, 40, color);
            M5Dial.Display.fillTriangle(centerX, centerY - 25, centerX - 10, centerY - 15, centerX + 10, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 15, centerY - 10, centerX - 25, centerY - 15, color);
            M5Dial.Display.drawLine(centerX + 15, centerY - 10, centerX + 25, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 15, centerY, centerX - 25, centerY - 5, color);
            M5Dial.Display.drawLine(centerX + 15, centerY, centerX + 25, centerY - 5, color);
            break;
            
        case 8: // Exit - Door with arrow icon
            M5Dial.Display.drawRect(centerX - 15, centerY - 18, 30, 36, color);
            M5Dial.Display.fillCircle(centerX - 8, centerY, 2, color);
            M5Dial.Display.drawLine(centerX + 5, centerY, centerX + 15, centerY, color);
            M5Dial.Display.fillTriangle(centerX + 12, centerY - 5, centerX + 12, centerY + 5, centerX + 18, centerY, color);
            break;
        case 11: // View Variables - List icon
            M5Dial.Display.drawLine(centerX - 18, centerY - 15, centerX + 18, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 18, centerY - 5, centerX + 18, centerY - 5, color);
            M5Dial.Display.drawLine(centerX - 18, centerY + 5, centerX + 18, centerY + 5, color);
            M5Dial.Display.drawLine(centerX - 18, centerY + 15, centerX + 18, centerY + 15, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY - 15, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY - 5, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY + 5, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY + 15, 2, color);
            break;
        case 12: // Clear Variables - Trash/Eraser icon
            M5Dial.Display.drawRect(centerX - 12, centerY - 15, 24, 28, color);
            M5Dial.Display.drawLine(centerX - 15, centerY - 17, centerX + 15, centerY - 17, color);
            M5Dial.Display.drawLine(centerX - 8, centerY - 20, centerX - 8, centerY - 17, color);
            M5Dial.Display.drawLine(centerX, centerY - 20, centerX, centerY - 17, color);
            M5Dial.Display.drawLine(centerX + 8, centerY - 20, centerX + 8, centerY - 17, color);
            M5Dial.Display.drawLine(centerX - 5, centerY - 10, centerX - 5, centerY + 8, color);
            M5Dial.Display.drawLine(centerX, centerY - 10, centerX, centerY + 8, color);
            M5Dial.Display.drawLine(centerX + 5, centerY - 10, centerX + 5, centerY + 8, color);
            break;

        case 13: // Exit - Door with arrow icon
            M5Dial.Display.drawRect(centerX - 15, centerY - 18, 30, 36, color);
            M5Dial.Display.fillCircle(centerX - 8, centerY, 2, color);
            M5Dial.Display.drawLine(centerX + 5, centerY, centerX + 15, centerY, color);
            M5Dial.Display.fillTriangle(centerX + 12, centerY - 5, centerX + 12, centerY + 5, centerX + 18, centerY, color);
            break;
    }
}

void displayAdminMenu(int selectedItem, OperationalMode currentMode) {
    M5Dial.Display.clear();
    
    int centerX = 120;
    int iconCenterY = 90;
    
    // Draw large icon in center
    uint16_t iconColor = THEME_ACCENT_COLOR;
    drawMenuIcon(selectedItem, centerX, iconCenterY, iconColor);
    
    // Mode indicator badge in top-right
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_right);
    String modeText;
    if (currentMode == MODE_BUY_STATION) {
        modeText = "BUY";
    } else if (currentMode == MODE_BUY_STATION_2) {
        modeText = "BS2";
    } else if (currentMode == MODE_BACKPACK) {
        modeText = "BP";
    } else if (currentMode == MODE_LOOT_TRANSFER) {
        modeText = "LT";
    } else if (currentMode == MODE_RELAY) {
        modeText = "RELAY";
    } else if (currentMode == MODE_MAP) {
        modeText = "MAP";
    } else {
        modeText = "???";
    }
    uint16_t modeColor = (currentMode == MODE_BUY_STATION || currentMode == MODE_BUY_STATION_2 || currentMode == MODE_BACKPACK || currentMode == MODE_LOOT_TRANSFER) ? GREEN : THEME_ACCENT_COLOR;
    M5Dial.Display.setTextColor(modeColor, BLACK);
    M5Dial.Display.drawString(modeText, 235, 5);

    // Page indicator
    drawAdminPageIndicator(selectedItem + 1, ADMIN_MENU_ITEMS);
    
    // Menu item names
    const char* menuNames[] = {
        "Device Info",
        "WiFi Status", 
        "WiFi Reconnect",
        "WiFi Scan",
        "Loot Lock",
        "Backpack",
        "Buy Station",
        "Loot Transfer",
        "Relay Mode",
        "Map Mode",
        "Dev Mode",
        "View Variables",
        "Clear Variables",
        "Exit Admin"
    };
    
    // Display menu name at bottom
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextColor(WHITE, BLACK);
    M5Dial.Display.drawString(menuNames[selectedItem], centerX, 185);
    
    // Navigation hints
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    M5Dial.Display.drawString("Rotate: Navigate | Press: Select", centerX, 220);
}


// ============================================
// 3. WIFI STATUS DISPLAY
// ============================================

void displayWiFiStatus() {
    M5Dial.Display.clear();
    drawAdminPageIndicator(2, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("WiFi Status", 120, 20);
    
    M5Dial.Display.setTextSize(1);
    
    if (WiFi.status() == WL_CONNECTED) {
        // Connected - show details
        M5Dial.Display.setTextColor(THEME_SUCCESS_COLOR, BLACK);
        M5Dial.Display.drawString("CONNECTED", 120, 50);
        
        // SSID
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString("Network:", 120, 80);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        String ssid = WiFi.SSID();
        if (ssid.length() > 18) ssid = ssid.substring(0, 18) + "...";
        M5Dial.Display.drawString(ssid, 120, 100);
        
        // IP Address
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString("IP Address:", 120, 130);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        M5Dial.Display.drawString(WiFi.localIP().toString(), 120, 150);
        
        // Signal strength with color coding
        int rssi = WiFi.RSSI();
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString("Signal:", 120, 180);
        
        // Color based on strength
        uint16_t signalColor = THEME_SUCCESS_COLOR;
        if (rssi < -80) signalColor = THEME_ERROR_COLOR;
        else if (rssi < -70) signalColor = THEME_WARNING_COLOR;
        
        M5Dial.Display.setTextColor(signalColor, BLACK);
        M5Dial.Display.drawString(String(rssi) + " dBm", 120, 200);
        
    } else {
        // Not connected
        M5Dial.Display.setTextColor(THEME_ERROR_COLOR, BLACK);
        M5Dial.Display.drawString("DISCONNECTED", 120, 50);
        
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        M5Dial.Display.drawString("No WiFi connection", 120, 100);
        M5Dial.Display.drawString("Use WiFi Reconnect", 120, 120);
        M5Dial.Display.drawString("or WiFi Scan", 120, 140);
    }
    
    // Exit instructions
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Press to return", 120, 220);
}


// ============================================
// 4. WIFI RECONNECTING
// ============================================

void displayWiFiReconnecting() {
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("WiFi", 120, 100);
    M5Dial.Display.drawString("Reconnecting...", 120, 130);
}


// ============================================
// 5. RELAY MODE DISPLAY
// ============================================

void displayRelayMode(const String& lastUuid = "", const String& lastResponse = "", bool lastSuccess = false) {
    M5Dial.Display.clear();
    M5Dial.Display.setTextDatum(middle_center);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("RELAY MODE", 120, 30);
    
    // Serial Number (small, at top)
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    String displaySerial = serialNum;
    if (displaySerial.length() > 20) {
        displaySerial = displaySerial.substring(0, 18) + "..";
    }
    M5Dial.Display.drawString(displaySerial, 120, 55);
    
    // Instructions
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.drawString("Scan badge", 120, 75);
    M5Dial.Display.drawString("to load UUID", 120, 95);
    
    // Show last result if available
    if (lastUuid.length() > 0) {
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString("Last UUID:", 120, 130);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        String shortUuid = lastUuid;
        if (shortUuid.length() > 12) {
            shortUuid = shortUuid.substring(0, 12) + "...";
        }
        M5Dial.Display.drawString(shortUuid, 120, 150);
        
        // Response
        uint16_t responseColor = lastSuccess ? THEME_SUCCESS_COLOR : THEME_ERROR_COLOR;
        M5Dial.Display.setTextColor(responseColor, BLACK);
        M5Dial.Display.drawString(lastResponse, 120, 180);
    }
    
    // Admin exit hint
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Admin badge to exit", 120, 220);
}


// ============================================
// 6. LOOT FLIP MODE DISPLAY
// ============================================

void displayLootFlipMode(const String& lastUuid = "", const String& lastStatus = "") {
    M5Dial.Display.clear();
    drawAdminPageIndicator(5, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextDatum(middle_center);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("LOOT LOCK", 120, 30);
    
    // Serial Number (small, at top)
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    String displaySerial = serialNum;
    if (displaySerial.length() > 20) {
        displaySerial = displaySerial.substring(0, 18) + "..";
    }
    M5Dial.Display.drawString(displaySerial, 120, 55);
    
    // Instructions
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.drawString("Scan badge", 120, 75);
    M5Dial.Display.drawString("to flip lock", 120, 95);
    
    // Show last result if available
    if (lastUuid.length() > 0) {
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString("UUID:", 120, 130);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        String shortUuid = lastUuid;
        if (shortUuid.length() > 12) {
            shortUuid = shortUuid.substring(0, 12) + "...";
        }
        M5Dial.Display.drawString(shortUuid, 120, 150);
        
        // Status with color coding
        uint16_t statusColor = THEME_TEXT_COLOR;
        if (lastStatus == "LOCKED") statusColor = THEME_ERROR_COLOR;
        else if (lastStatus == "UNLOCKED") statusColor = THEME_SUCCESS_COLOR;
        else if (lastStatus == "NOT FOUND") statusColor = THEME_WARNING_COLOR;
        
        M5Dial.Display.setTextColor(statusColor, BLACK);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString(lastStatus, 120, 180);
    }
    
    // Exit hint
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Press to return", 120, 220);
}


// ============================================
// 7. ADMIN ACCESS GRANTED ANIMATION
// ============================================

void displayAdminAccessGranted() {
    M5Dial.Display.clear();
    M5Dial.Display.setTextDatum(middle_center);
    
    // Flash animation
    for (int i = 0; i < 3; i++) {
        M5Dial.Display.fillScreen(THEME_ACCENT_COLOR);
        M5Dial.Display.setTextColor(BLACK, THEME_ACCENT_COLOR);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString("ADMIN", 120, 100);
        M5Dial.Display.drawString("ACCESS", 120, 130);
        delay(150);
        
        M5Dial.Display.fillScreen(BLACK);
        delay(150);
    }
    
    // Final display
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.drawString("ADMIN", 120, 100);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("ACCESS", 120, 140);
    delay(500);
}


// ============================================
// 8. WIFI SCANNING ANIMATION
// ============================================

void displayWiFiScanning() {
    M5Dial.Display.clear();
    drawAdminPageIndicator(4, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("Scanning", 120, 100);
    M5Dial.Display.drawString("WiFi...", 120, 130);
    
    // Animated dots
    for (int i = 0; i < 3; i++) {
        M5Dial.Display.fillCircle(100 + (i * 20), 170, 3, THEME_ACCENT_COLOR);
        delay(200);
    }
}


// ============================================
// 9. WIFI NETWORK LIST (SCROLLABLE)
// ============================================

void displayWiFiNetworkList(String ssids[], int rssi[], bool encrypted[], int count, int selectedIndex) {
    M5Dial.Display.clear();
    drawAdminPageIndicator(4, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextDatum(top_left);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("WiFi Networks", 120, 15);
    
    if (count == 0) {
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        M5Dial.Display.drawString("No networks found", 120, 120);
        return;
    }
    
    // Display up to 5 networks at a time (scrollable list)
    int startY = 45;
    int itemHeight = 35;
    int maxVisible = 5;
    
    // Calculate visible window
    int startIdx = max(0, selectedIndex - 2);
    int endIdx = min(count, startIdx + maxVisible);
    
    for (int i = startIdx; i < endIdx; i++) {
        int y = startY + ((i - startIdx) * itemHeight);
        
        // Highlight selected
        if (i == selectedIndex) {
            M5Dial.Display.fillRect(5, y - 2, 230, itemHeight - 5, THEME_ACCENT_COLOR);
            M5Dial.Display.setTextColor(BLACK, THEME_ACCENT_COLOR);
        } else {
            M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        }
        
        // SSID (truncate if too long)
        String displaySSID = ssids[i];
        if (displaySSID.length() > 16) {
            displaySSID = displaySSID.substring(0, 16) + "...";
        }
        
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(top_left);
        M5Dial.Display.drawString(displaySSID, 10, y);
        
        // Signal strength bars
        int bars = 1;
        if (rssi[i] > -80) bars = 2;
        if (rssi[i] > -70) bars = 3;
        if (rssi[i] > -60) bars = 4;
        
        uint16_t barColor = (i == selectedIndex) ? BLACK : THEME_TEXT_COLOR;
        for (int b = 0; b < bars; b++) {
            int barHeight = 3 + (b * 2);
            M5Dial.Display.fillRect(170 + (b * 4), y + 10 - barHeight, 3, barHeight, barColor);
        }
        
        // Lock icon for encrypted networks
        if (encrypted[i]) {
            uint16_t lockColor = (i == selectedIndex) ? BLACK : THEME_WARNING_COLOR;
            M5Dial.Display.setTextColor(lockColor, (i == selectedIndex) ? THEME_ACCENT_COLOR : BLACK);
            M5Dial.Display.drawString("🔒", 200, y);
        }
    }
    
    // Scroll indicators
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextSize(1);
    if (startIdx > 0) {
        M5Dial.Display.drawString("▲", 120, 35);
    }
    if (endIdx < count) {
        M5Dial.Display.drawString("▼", 120, 225);
    }
}


// ============================================
// 10. PASSWORD ENTRY (CHARACTER PICKER)
// ============================================

void displayPasswordEntry(const String& ssid, const String& password, int cursorPos, const char* charset, int charsetSize, int currentCharIdx) {
    M5Dial.Display.clear();
    drawAdminPageIndicator(4, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextDatum(middle_center);
    
    // SSID header
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    String displaySSID = ssid;
    if (displaySSID.length() > 18) {
        displaySSID = displaySSID.substring(0, 18) + "...";
    }
    M5Dial.Display.drawString(displaySSID, 120, 20);
    
    // Password display box
    M5Dial.Display.drawRect(20, 40, 200, 30, THEME_ACCENT_COLOR);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    String displayPassword = password;
    if (displayPassword.length() > 20) {
        displayPassword = displayPassword.substring(password.length() - 20);
    }
    M5Dial.Display.drawString(displayPassword, 120, 55);
    
    // Cursor line
    int cursorX = 120 + (password.length() * 6) - (displayPassword.length() * 6);
    M5Dial.Display.drawLine(cursorX, 65, cursorX + 5, 65, THEME_ACCENT_COLOR);
    
    // Character selection - show current, previous, and next
    M5Dial.Display.setTextSize(4);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    
    // Determine display characters based on position
    String currentChar = "";
    if (currentCharIdx == charsetSize - 2) {
        currentChar = "DEL";
        M5Dial.Display.setTextSize(2);
    } else if (currentCharIdx == charsetSize - 1) {
        currentChar = "DONE";
        M5Dial.Display.setTextSize(2);
    } else {
        currentChar = String(charset[currentCharIdx]);
    }
    
    M5Dial.Display.drawString(currentChar, 120, 120);
    
    // Previous and next characters (smaller)
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    
    // Previous char
    int prevIdx = (currentCharIdx - 1 + charsetSize) % charsetSize;
    String prevChar = "";
    if (prevIdx == charsetSize - 2) prevChar = "DEL";
    else if (prevIdx == charsetSize - 1) prevChar = "DONE";
    else prevChar = String(charset[prevIdx]);
    M5Dial.Display.drawString(prevChar, 60, 120);
    
    // Next char
    int nextIdx = (currentCharIdx + 1) % charsetSize;
    String nextChar = "";
    if (nextIdx == charsetSize - 2) nextChar = "DEL";
    else if (nextIdx == charsetSize - 1) nextChar = "DONE";
    else nextChar = String(charset[nextIdx]);
    M5Dial.Display.drawString(nextChar, 180, 120);
    
    // Instructions
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Rotate to scroll", 120, 180);
    M5Dial.Display.drawString("Press to select", 120, 200);
}


// ============================================
// 11. WIFI CONNECTING
// ============================================

void displayWiFiConnecting(const String& ssid) {
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("Connecting", 120, 80);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    String displaySSID = ssid;
    if (displaySSID.length() > 18) {
        displaySSID = displaySSID.substring(0, 18) + "...";
    }
    M5Dial.Display.drawString(displaySSID, 120, 120);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Please wait...", 120, 160);
}


// ============================================
// 12. WIFI CONNECTION RESULT
// ============================================

void displayWiFiConnectionResult(bool success, const String& message) {
    M5Dial.Display.clear();
    M5Dial.Display.setTextDatum(middle_center);
    
    if (success) {
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextColor(THEME_SUCCESS_COLOR, BLACK);
        M5Dial.Display.drawString("SUCCESS", 120, 80);
        
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        M5Dial.Display.drawString("Connected!", 120, 120);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString(message, 120, 145);
    } else {
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextColor(THEME_ERROR_COLOR, BLACK);
        M5Dial.Display.drawString("FAILED", 120, 80);
        
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
        M5Dial.Display.drawString("Connection failed", 120, 120);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString(message, 120, 145);
    }
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Press to continue", 120, 210);
}

// ============================================
// 10. VIEW VARIABLES DISPLAY
// ============================================

void displayVariableViewer(int scrollIndex) {
    extern String bs2BadgeUuid, bs2BadgeName, bs2BadgeFaction, bs2BadgeFactionId, bs2PlayerClass;
    extern int bs2ClassMultiplier, bs2BackpackValue, bs2LootLimit, bs2LootCount, bs2ClaimedCount;
    extern String bs2LastUsedBadge;
    extern bool bs2DevMode;
    extern String relayLastUuid, relayLastResponse;
    extern bool relayLastSuccess;
    extern String lootFlipLastUuid, lootFlipLastStatus;
    extern int playerClassCount;
    extern String serialNum, macAddress, gameId;
    extern OperationalMode deviceMode;
    extern unsigned long bs2LastActivityTime;
    
    M5Dial.Display.clear();
    drawAdminPageIndicator(12, ADMIN_MENU_ITEMS);
    M5Dial.Display.setTextColor(THEME_TEXT_COLOR, BLACK);
    M5Dial.Display.setTextDatum(top_left);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Variables", 10, 5);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE, BLACK);
    
    // Build variable list
    String varNames[30];
    String varValues[30];
    int varCount = 0;
    
    // Device Info
    varNames[varCount] = "Serial"; varValues[varCount] = serialNum; varCount++;
    varNames[varCount] = "MAC"; varValues[varCount] = macAddress.substring(0, 17); varCount++;
    varNames[varCount] = "GameId"; varValues[varCount] = gameId.substring(0, 12) + "..."; varCount++;
    
    // Mode
    String modeStr = "Unknown";
    if (deviceMode == MODE_BUY_STATION) modeStr = "BuyStation";
    else if (deviceMode == MODE_BUY_STATION_2) modeStr = "BuyStation2";
    else if (deviceMode == MODE_BACKPACK) modeStr = "Backpack";
    else if (deviceMode == MODE_LOOT_TRANSFER) modeStr = "LootTransfer";
    else if (deviceMode == MODE_RELAY) modeStr = "Relay";
    else if (deviceMode == MODE_MAP) modeStr = "Map";
    varNames[varCount] = "Mode"; varValues[varCount] = modeStr; varCount++;
    
    // Buy Station 2 Variables
    varNames[varCount] = "BS2:Badge"; varValues[varCount] = bs2BadgeUuid.length() > 0 ? bs2BadgeUuid.substring(0, 10) : "(none)"; varCount++;
    varNames[varCount] = "BS2:Name"; varValues[varCount] = bs2BadgeName.length() > 0 ? bs2BadgeName : "(none)"; varCount++;
    varNames[varCount] = "BS2:Class"; varValues[varCount] = bs2PlayerClass.length() > 0 ? bs2PlayerClass : "(none)"; varCount++;
    varNames[varCount] = "BS2:Multiplier"; varValues[varCount] = String(bs2ClassMultiplier) + "x"; varCount++;
    varNames[varCount] = "BS2:LootLimit"; varValues[varCount] = String(bs2LootLimit); varCount++;
    varNames[varCount] = "BS2:LootCount"; varValues[varCount] = String(bs2LootCount); varCount++;
    varNames[varCount] = "BS2:Claimed"; varValues[varCount] = String(bs2ClaimedCount); varCount++;
    varNames[varCount] = "BS2:LastUsed"; varValues[varCount] = bs2LastUsedBadge.length() > 0 ? bs2LastUsedBadge.substring(0, 10) : "(none)"; varCount++;
    varNames[varCount] = "BS2:DevMode"; varValues[varCount] = bs2DevMode ? "YES" : "NO"; varCount++;
    varNames[varCount] = "BS2:Session"; varValues[varCount] = bs2LastActivityTime > 0 ? String((millis() - bs2LastActivityTime) / 1000) + "s ago" : "(none)"; varCount++;
    
    // Player Classes Cache
    varNames[varCount] = "ClassCache"; varValues[varCount] = String(playerClassCount) + " classes"; varCount++;
    
    // Relay Variables
    varNames[varCount] = "Relay:UUID"; varValues[varCount] = relayLastUuid.length() > 0 ? relayLastUuid.substring(0, 10) : "(none)"; varCount++;
    varNames[varCount] = "Relay:Success"; varValues[varCount] = relayLastSuccess ? "YES" : "NO"; varCount++;
    
    // Loot Flip Variables
    varNames[varCount] = "LootFlip:UUID"; varValues[varCount] = lootFlipLastUuid.length() > 0 ? lootFlipLastUuid.substring(0, 10) : "(none)"; varCount++;
    varNames[varCount] = "LootFlip:Status"; varValues[varCount] = lootFlipLastStatus.length() > 0 ? lootFlipLastStatus : "(none)"; varCount++;
    
    // WiFi
    varNames[varCount] = "WiFi"; varValues[varCount] = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"; varCount++;
    varNames[varCount] = "RSSI"; varValues[varCount] = WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) + " dBm" : "N/A"; varCount++;
    
    // Memory
    varNames[varCount] = "FreeHeap"; varValues[varCount] = String(ESP.getFreeHeap() / 1024) + " KB"; varCount++;
    varNames[varCount] = "Uptime"; varValues[varCount] = String(millis() / 60000) + " min"; varCount++;
    
    // Constrain scroll
    int maxScroll = max(0, varCount - 8);  // Show 8 lines at a time
    scrollIndex = constrain(scrollIndex, 0, maxScroll);
    
    // Display variables (8 lines visible)
    int yPos = 30;
    for (int i = scrollIndex; i < min(scrollIndex + 8, varCount); i++) {
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
        M5Dial.Display.drawString(varNames[i] + ":", 10, yPos);
        M5Dial.Display.setTextColor(WHITE, BLACK);
        M5Dial.Display.drawString(varValues[i], 130, yPos);
        yPos += 22;
    }
    
    // Scroll indicator
    if (varCount > 8) {
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(0x7BEF, BLACK);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("Rotate: Scroll (" + String(scrollIndex + 1) + "/" + String(maxScroll + 1) + ")", 120, 220);
    }
    
    // Navigation hint
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press: Back to Menu", 120, 235);
}

#endif // M5DIALCONTROL_ADMIN_H


// ============================================
// USAGE NOTES
// ============================================

/*
 * TO INTEGRATE THESE DISPLAYS:
 * 
 * 1. Include this header in your main sketch
 * 2. Call functions from your state machine
 * 3. Customize theme colors for your project
 * 4. Adjust text sizes for readability
 * 5. Modify layouts to fit your 240x240 circular screen
 * 
 * DISPLAY OPTIMIZATIONS:
 * - All text is centered using setTextDatum()
 * - Long strings are truncated with "..."
 * - Color coding for status (GREEN=good, RED=bad, YELLOW=warning)
 * - Scrollable lists show 5 items at a time
 * - Signal strength uses 4-bar visualization
 * - Password display auto-scrolls if > 20 chars
 * 
 * THEME CUSTOMIZATION:
 * - Change THEME_* colors to match your design
 * - Adjust text sizes for your display
 * - Modify spacing and positioning as needed
 * - Add custom graphics or icons
 * 
 */

// END OF ADMIN DISPLAY FUNCTIONS
