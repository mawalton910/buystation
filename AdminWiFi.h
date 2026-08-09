// AdminWiFi.h - WiFi Admin Functions
#ifndef ADMIN_WIFI_H
#define ADMIN_WIFI_H

#include <M5Dial.h>
#include <WiFi.h>
#include "config.h"
#include "WiFiManager.h"

// External variables
extern String macAddress;

// Display WiFi status screen
void displayWiFiStatus() {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("WIFI", centerX, 10);
    M5Dial.Display.drawString("STATUS", centerX, 30);
    
    M5Dial.Display.drawLine(20, 55, M5Dial.Display.width() - 20, 55, THEME_ACCENT_COLOR);
    
    // Connection status
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_left);
    
    bool connected = WiFi.status() == WL_CONNECTED;
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Status:", 20, 75);
    M5Dial.Display.setTextColor(connected ? GREEN : RED);
    M5Dial.Display.drawString(connected ? "OK" : "Down", 90, 75);
    
    if (connected) {
        // SSID
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("SSID:", 20, 95);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.drawString(WiFi.SSID(), 90, 95);

        // Reconnect count
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Reconnects:", 20, 115);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.drawString(String(networkState.reconnectCount), 140, 115);
        
        // IP Address
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("IP:", 20, 135);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        String ipStr = WiFi.localIP().toString();
        M5Dial.Display.drawString(ipStr, 90, 135);
        
        // Signal strength
        int rssi = WiFi.RSSI();
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Signal:", 20, 155);
        M5Dial.Display.setTextColor(rssi > -70 ? GREEN : (rssi > -80 ? YELLOW : RED));
        M5Dial.Display.drawString(String(rssi) + " dBm", 90, 155);

        // Last reconnect time (seconds ago)
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Last Reconn:", 20, 155);
        M5Dial.Display.setTextColor(0x7BEF);
        unsigned long secondsSinceReconnect = (millis() - networkState.lastReconnectMs) / 1000;
        M5Dial.Display.drawString(String(secondsSinceReconnect) + "s", 120, 155);

        // RTT
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("RTT:", 20, 175);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.drawString(String(networkState.lastRttMs) + " ms", 90, 175);

        // TLS/API status
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("TLS Ready:", 20, 195);
        M5Dial.Display.setTextColor(networkState.tlsReady ? GREEN : YELLOW);
        M5Dial.Display.drawString(networkState.tlsReady ? "YES" : "WAIT", 120, 195);

        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("API:", 20, 215);
        M5Dial.Display.setTextColor(networkState.apiStatus.startsWith("20") ? GREEN : YELLOW);
        M5Dial.Display.drawString(networkState.apiStatus, 90, 215);
        
        // MAC Address (truncated for display)
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("MAC:", 20, 155);
        M5Dial.Display.setTextColor(0x7BEF);
        String macAddr = macAddress;
        // Show last 8 chars (last 4 bytes)
        if (macAddr.length() > 11) {
            macAddr = ".." + macAddr.substring(macAddr.length() - 11);
        }
        M5Dial.Display.drawString(macAddr, 20, 170);
    }
    
    // Back instruction
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press: Menu | Admin: Exit", centerX, M5Dial.Display.height() - 5);
}

// Display WiFi reconnecting screen
void displayWiFiReconnecting() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("WIFI", centerX, centerY - 20);
    M5Dial.Display.drawString("RECONNECTING", centerX, centerY + 5);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Please wait...", centerX, centerY + 35);
}

// Display WiFi scanning screen
void displayWiFiScanning() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("SCANNING", centerX, centerY - 10);
    M5Dial.Display.drawString("NETWORKS", centerX, centerY + 15);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Please wait...", centerX, centerY + 45);
}

// Display WiFi network list
void displayWiFiNetworkList(String ssids[], int rssi[], bool encrypted[], int count, int selectedIndex) {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("NETWORKS", centerX, 5);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString(String(count) + " found", centerX, 28);
    
    M5Dial.Display.drawLine(20, 42, M5Dial.Display.width() - 20, 42, THEME_ACCENT_COLOR);
    
    // Show networks (3-5 at a time)
    int startIdx = max(0, selectedIndex - 1);
    int endIdx = min(count, startIdx + 4);
    int yPos = 50;
    
    M5Dial.Display.setTextDatum(middle_left);
    
    for (int i = startIdx; i < endIdx; i++) {
        bool isSelected = (i == selectedIndex);
        
        // Selection indicator
        if (isSelected) {
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.drawString(">", 5, yPos);
        }
        
        // SSID
        M5Dial.Display.setTextColor(isSelected ? WHITE : 0x7BEF);
        String displaySSID = ssids[i];
        if (displaySSID.length() > 18) {
            displaySSID = displaySSID.substring(0, 16) + "..";
        }
        M5Dial.Display.drawString(displaySSID, 20, yPos);
        
        // Signal strength indicator
        int signalBars = 0;
        if (rssi[i] > -60) signalBars = 4;
        else if (rssi[i] > -70) signalBars = 3;
        else if (rssi[i] > -80) signalBars = 2;
        else signalBars = 1;
        
        int barX = 200;
        for (int b = 0; b < 4; b++) {
            uint16_t barColor = (b < signalBars) ? GREEN : 0x2104;
            int barHeight = 4 + (b * 2);
            M5Dial.Display.fillRect(barX + (b * 4), yPos - barHeight/2, 3, barHeight, barColor);
        }
        
        // Lock icon if encrypted
        if (encrypted[i]) {
            M5Dial.Display.setTextColor(YELLOW);
            M5Dial.Display.drawString("L", 220, yPos);
        }
        
        yPos += 18;
    }
    
    // Instructions
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Rotate: Select | Press: Connect", centerX, M5Dial.Display.height() - 5);
}

// Display password entry screen
void displayPasswordEntry(String ssid, String password, int cursorPos, const char charSet[], int charSetSize, int currentCharIndex) {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("Enter Password", centerX, 5);
    
    // SSID
    M5Dial.Display.setTextColor(WHITE);
    String displaySSID = ssid;
    if (displaySSID.length() > 20) {
        displaySSID = displaySSID.substring(0, 18) + "..";
    }
    M5Dial.Display.drawString(displaySSID, centerX, 20);
    
    M5Dial.Display.drawLine(20, 35, M5Dial.Display.width() - 20, 35, THEME_ACCENT_COLOR);
    
    // Password display
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    
    String displayPassword = password;
    if (displayPassword.length() == 0) {
        displayPassword = "_";
    }
    if (displayPassword.length() > 16) {
        displayPassword = displayPassword.substring(displayPassword.length() - 16);
    }
    M5Dial.Display.drawString(displayPassword, centerX, 70);
    
    // Character selector
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString("Select character:", centerX, 100);
    
    // Current character (large)
    M5Dial.Display.setTextSize(4);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    String currentChar = "";
    if (currentCharIndex == charSetSize - 2) {
        currentChar = "<DEL>";
        M5Dial.Display.setTextSize(2);
    } else if (currentCharIndex == charSetSize - 1) {
        currentChar = "<DONE>";
        M5Dial.Display.setTextSize(2);
    } else {
        currentChar = String(charSet[currentCharIndex]);
    }
    M5Dial.Display.drawString(currentChar, centerX, 140);
    
    // Previous and next characters (small preview)
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(0x7BEF);
    
    int prevIdx = (currentCharIndex - 1 + charSetSize) % charSetSize;
    int nextIdx = (currentCharIndex + 1) % charSetSize;
    
    String prevChar = (prevIdx == charSetSize - 2) ? "DEL" : (prevIdx == charSetSize - 1) ? "OK" : String(charSet[prevIdx]);
    String nextChar = (nextIdx == charSetSize - 2) ? "DEL" : (nextIdx == charSetSize - 1) ? "OK" : String(charSet[nextIdx]);
    
    M5Dial.Display.drawString(prevChar, centerX - 60, 140);
    M5Dial.Display.drawString(nextChar, centerX + 60, 140);
    
    // Instructions
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Rotate: Choose | Press: Add", centerX, M5Dial.Display.height() - 5);
}

// Display WiFi connecting screen
void displayWiFiConnecting(String ssid) {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("CONNECTING", centerX, centerY - 20);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(ssid, centerX, centerY + 10);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString("Please wait...", centerX, centerY + 30);
}

// Display WiFi connection result
void displayWiFiConnectionResult(bool success, String message) {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    if (success) {
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("SUCCESS", centerX, centerY - 10);
        
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Connected!", centerX, centerY + 25);
    } else {
        M5Dial.Display.setTextColor(RED);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("FAILED", centerX, centerY - 20);
        
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(WHITE);
        // Wrap message if needed
        if (message.length() > 20) {
            M5Dial.Display.drawString(message.substring(0, 20), centerX, centerY + 10);
            M5Dial.Display.drawString(message.substring(20), centerX, centerY + 25);
        } else {
            M5Dial.Display.drawString(message, centerX, centerY + 15);
        }
    }
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press button to continue", centerX, M5Dial.Display.height() - 5);
}

// Scan nearby networks and populate arrays (max 20 entries)
void scanWiFiNetworks(String ssids[], int rssi[], bool encrypted[], int &count) {
    displayWiFiScanning();
    count = WiFi.scanNetworks();
    if (count > 20) count = 20;
    for (int i = 0; i < count; i++) {
        ssids[i] = WiFi.SSID(i);
        rssi[i] = WiFi.RSSI(i);
        encrypted[i] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
}

// Connect to selected network with optional password
bool connectToSelectedNetwork(const String &ssid, const String &password) {
    WiFi.disconnect();
    delay(500);
    if (password.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
    } else {
        WiFi.begin(ssid.c_str());
    }

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    return WiFi.status() == WL_CONNECTED;
}

#endif // ADMIN_WIFI_H
