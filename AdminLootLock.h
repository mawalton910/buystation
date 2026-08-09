// AdminLootLock.h - Loot Lock Admin Functions
#ifndef ADMIN_LOOT_LOCK_H
#define ADMIN_LOOT_LOCK_H

#include <M5Dial.h>
#include "config.h"

// External variables
extern String serialNum;

// Display loot lock/unlock mode
void displayLootFlipMode(String lastUuid = "", String lastStatus = "") {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("LOOT LOCK", centerX, 5);
    M5Dial.Display.drawString("MANAGER", centerX, 25);
    
    // Serial number badge
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(serialNum, centerX, 47);
    
    M5Dial.Display.drawLine(20, 62, M5Dial.Display.width() - 20, 62, THEME_ACCENT_COLOR);
    
    // Instructions
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("Scan loot badge", centerX, 80);
    M5Dial.Display.drawString("to toggle lock", centerX, 95);
    
    // Last action results
    if (lastUuid.length() > 0) {
        M5Dial.Display.setTextColor(0x7BEF); // Gray
        M5Dial.Display.setTextDatum(middle_left);
        M5Dial.Display.drawString("UUID:", 20, 110);
        
        // Truncate UUID if too long
        String displayUuid = lastUuid;
        if (displayUuid.length() > 16) {
            displayUuid = displayUuid.substring(0, 14) + "..";
        }
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.drawString(displayUuid, 20, 125);
        
        // Status - Large and prominent
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("LOCK STATUS:", centerX, 150);
        
        // Color code status with larger text
        uint16_t statusColor = WHITE;
        String statusIcon = "";
        if (lastStatus == "LOCKED") {
            statusColor = RED;
            statusIcon = "[LOCKED]";
        } else if (lastStatus == "UNLOCKED") {
            statusColor = GREEN;
            statusIcon = "[UNLOCKED]";
        } else if (lastStatus == "NOT FOUND") {
            statusColor = YELLOW;
            statusIcon = "[NOT FOUND]";
        } else {
            statusColor = RED;
            statusIcon = "[ERROR]";
        }
        
        M5Dial.Display.setTextColor(statusColor);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString(statusIcon, centerX, 175);
    } else {
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("Ready to scan...", centerX, 135);
    }
    
    // Exit instruction
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press: Menu | Admin: Exit", centerX, M5Dial.Display.height() - 5);
}

// Display Backpack mode - Claim and lock loot
void displayBackpackMode(String lastUuid = "", String lastStatus = "") {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("BACKPACK", centerX, 5);
    M5Dial.Display.drawString("MODE", centerX, 25);
    
    // Serial number badge
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(serialNum, centerX, 47);
    
    M5Dial.Display.drawLine(20, 62, M5Dial.Display.width() - 20, 62, THEME_ACCENT_COLOR);
    
    // Instructions
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("Scan loot to", centerX, 80);
    M5Dial.Display.drawString("claim loot", centerX, 95);
    
    // Last action results
    if (lastUuid.length() > 0) {
        M5Dial.Display.setTextColor(0x7BEF); // Gray
        M5Dial.Display.setTextDatum(middle_left);
        M5Dial.Display.drawString("UUID:", 20, 115);
        
        // Truncate UUID if too long
        String displayUuid = lastUuid;
        if (displayUuid.length() > 16) {
            displayUuid = displayUuid.substring(0, 14) + "..";
        }
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.drawString(displayUuid, 20, 130);
        
        // Status - Large and prominent
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("STATUS:", centerX, 155);
        
        // Color code status with larger text
        uint16_t statusColor = WHITE;
        String statusIcon = "";
        if (lastStatus == "SUCCESS") {
            statusColor = GREEN;
            statusIcon = "[CLAIMED+UNLOCKED]";
        } else if (lastStatus == "NOT FOUND") {
            statusColor = YELLOW;
            statusIcon = "[NOT FOUND]";
        } else if (lastStatus == "ALREADY CLAIMED") {
            statusColor = ORANGE;
            statusIcon = "[CLAIMED]";
        } else {
            statusColor = RED;
            statusIcon = "[ERROR]";
        }
        
        M5Dial.Display.setTextColor(statusColor);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString(statusIcon, centerX, 180);
    } else {
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("Ready to scan...", centerX, 140);
    }
    
    // Exit instruction
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press: Menu | Admin: Exit", centerX, M5Dial.Display.height() - 5);
}

#endif // ADMIN_LOOT_LOCK_H
