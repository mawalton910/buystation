// BadgeUtils.h - Badge Utility Functions
#ifndef BADGE_UTILS_H
#define BADGE_UTILS_H

#include <Arduino.h>
#include "config.h"

// External variables
extern unsigned long lastCardReadTime;

// Sanitize UUID - remove spaces and convert to uppercase
String sanitizeUuid(const String& uuid) {
    String sanitized = uuid;
    sanitized.trim();
    sanitized.replace(" ", "");
    sanitized.toUpperCase();
    return sanitized;
}

// Check if scanned UUID is an admin badge
bool isAdminBadge(String uuid) {
    for (int i = 0; i < ADMIN_BADGE_UIDS_COUNT; i++) {
        if (uuid == ADMIN_BADGE_UIDS[i]) {
            return true;
        }
    }
    return false;
}

// Check if UUID is in the WiFi retry tags array
bool isWiFiRetryTag(String uuid) {
    for (int i = 0; i < WIFI_RETRY_TAGS_COUNT; i++) {
        if (uuid == WIFI_RETRY_TAGS[i]) {
            return true;
        }
    }
    return false;
}

// Check if touch coordinates are within the confirm button area
bool isTouchInConfirmButton(int x, int y) {
    int screenWidth = M5Dial.Display.width();
    int screenHeight = M5Dial.Display.height();
    int buttonTop = screenHeight / 2 + CONFIRM_BUTTON_TOP_OFFSET;
    int buttonBottom = screenHeight - CONFIRM_BUTTON_BOTTOM_MARGIN;
    int buttonLeft = CONFIRM_BUTTON_LEFT_MARGIN;
    int buttonRight = screenWidth - CONFIRM_BUTTON_RIGHT_MARGIN;
    
    return (x >= buttonLeft && x <= buttonRight && y >= buttonTop && y <= buttonBottom);
}

#endif // BADGE_UTILS_H
