// AdminDeviceInfo.h - Device Info Admin Functions
#ifndef ADMIN_DEVICE_INFO_H
#define ADMIN_DEVICE_INFO_H

#include <M5Dial.h>
#include <WiFi.h>
#include "config.h"

// External variables
extern String macAddress;

// Display device info screen
void displayDeviceInfo(String serial, OperationalMode mode) {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("DEVICE", centerX, 10);
    M5Dial.Display.drawString("INFO", centerX, 30);
    
    M5Dial.Display.drawLine(20, 55, M5Dial.Display.width() - 20, 55, THEME_ACCENT_COLOR);
    
    // Device info
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_left);
    
    // Serial Number
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Serial:", 20, 70);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    // Truncate if too long
    String displaySerial = serial;
    if (displaySerial.length() > 18) {
        displaySerial = displaySerial.substring(0, 16) + "..";
    }
    M5Dial.Display.drawString(displaySerial, 20, 85);
    
    // Mode
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Mode:", 20, 105);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    String modeText = "";
    if (mode == MODE_BUY_STATION) {
        modeText = "Buy Station";
    } else if (mode == MODE_BUY_STATION_2) {
        modeText = "Buy Station";
    } else if (mode == MODE_BACKPACK) {
        modeText = "Backpack";
    } else if (mode == MODE_LOOT_TRANSFER) {
        modeText = "Loot Transfer";
    } else if (mode == MODE_PHOTOBOOTH) {
        modeText = "Photobooth";
    } else if (mode == MODE_MAP) {
        modeText = "Map";
    } else {
        modeText = "Relay";
    }
    M5Dial.Display.drawString(modeText, 90, 105);
    
    // Firmware Version
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Version:", 20, 125);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString(FIRMWARE_VERSION, 20, 140);
    
    // MAC Address
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("MAC:", 20, 160);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString(macAddress, 20, 175);
    
    // Back instruction
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press: Menu | Admin: Exit", centerX, M5Dial.Display.height() - 5);
}

#endif // ADMIN_DEVICE_INFO_H
