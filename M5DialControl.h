// M5DialControl.h
#ifndef M5DIALCONTROL_H
#define M5DIALCONTROL_H

#include <M5Dial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "Faction.h"
#include "secrets.h"
#include "AdminWiFi.h"
#include "AdminLootLock.h"
#include "AdminDeviceInfo.h"
#include "Badge.h"
#include "WiFiManager.h"
#include "SecureBuyStationClient.h"
#include "FactionCache.h"
#include "LegacyFactions.h"

// Forward declarations for tone functions
void playAcceptTone();
void playErrorTone();
void playScanTone();
void playSoftAcceptTone();
void playSoftErrorTone();
void playPurchaseCompleteTone();
void playReturnToScanTone();
void playEncoderClick();
void playBuzzTone();

// Forward declaration for whatIsIt
bool whatIsIt(String uuid, StaticJsonDocument<1024>& doc);

// External declarations for Buy Station 2 variables
extern bool bs2DevMode;
extern String bs2PlayerClass;
extern int bs2ClassMultiplier;

// Loot Transfer Mode struct definition (shared with LootTransferMode.h)
struct TransferInventoryItem {
    String itemId;
    String itemName;
    int amount;
    bool canTransfer;  // false if item has "Scored" keyword
};

// External declarations for Loot Transfer Mode variables (from LootTransferMode.h)
extern String ltFactionA_BadgeUuid;
extern String ltFactionA_BadgeName;
extern String ltFactionA_FactionName;
extern String ltFactionA_FactionId;
extern TransferInventoryItem ltFactionA_Items[];
extern int ltFactionA_ItemCount;
extern int ltFactionA_SelectedIndex;
extern int ltFactionA_SelectedAmount;
extern String ltFactionB_BadgeUuid;
extern String ltFactionB_BadgeName;
extern String ltFactionB_FactionName;
extern String ltFactionB_FactionId;
extern TransferInventoryItem ltFactionB_Items[];
extern int ltFactionB_ItemCount;
extern int ltFactionB_SelectedIndex;
extern int ltFactionB_SelectedAmount;

// Initialize the M5Dial display
void initM5Display() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextSize(2);
}

// Draw WiFi status indicator circle in top-right corner
void drawWiFiIndicator() {
    int x = M5Dial.Display.width() - WIFI_INDICATOR_X_OFFSET;
    int y = WIFI_INDICATOR_Y_OFFSET;
    int radius = WIFI_INDICATOR_RADIUS;
    
    bool connected = networkState.connected || WiFi.status() == WL_CONNECTED;
    uint32_t color = connected ? WIFI_CONNECTED_COLOR : WIFI_DISCONNECTED_COLOR;
    if (networkState.httpInFlight) {
        color = THEME_ACCENT_COLOR;
    }
    M5Dial.Display.fillCircle(x, y, radius, color);
    M5Dial.Display.drawCircle(x, y, radius + WIFI_INDICATOR_OUTLINE_WIDTH, WIFI_INDICATOR_OUTLINE);
}

// Lightweight network spinner (small top-left arc)
bool networkSpinnerActive = false;
unsigned long networkSpinnerLastFrame = 0;
int networkSpinnerFrame = 0;
String networkSpinnerLabel = "NET";

void startNetworkSpinner(const String &label = "NET") {
    networkSpinnerActive = true;
    networkSpinnerLabel = label;
    networkSpinnerFrame = 0;
    networkSpinnerLastFrame = 0;
}

void stopNetworkSpinner() {
    networkSpinnerActive = false;
}

void tickNetworkSpinner() {
    if (!networkSpinnerActive) return;
    unsigned long now = millis();
    if (now - networkSpinnerLastFrame < 80) return;
    networkSpinnerLastFrame = now;
    networkSpinnerFrame = (networkSpinnerFrame + 1) % 12;

    int x = WIFI_INDICATOR_RADIUS + 10;
    int y = WIFI_INDICATOR_RADIUS + 8;
    int rOuter = WIFI_INDICATOR_RADIUS + 6;

    // Precomputed simple spinner offsets (clock positions)
    const int8_t offsets[12][2] = {
        {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
        {0, -1}, {1, -1}, {1, 0}, {1, 1}
    };

    // Clear small region before redraw
    M5Dial.Display.fillRect(0, 0, x + rOuter + 8, y + rOuter + 8, BLACK);

    int dotX = x + offsets[networkSpinnerFrame][0] * (WIFI_INDICATOR_RADIUS + 2);
    int dotY = y + offsets[networkSpinnerFrame][1] * (WIFI_INDICATOR_RADIUS + 2);
    M5Dial.Display.fillCircle(dotX, dotY, 3, THEME_ACCENT_COLOR);

    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextDatum(top_left);
    M5Dial.Display.drawString(networkSpinnerLabel, x + rOuter - 2, y - 6);
}

// Set screen to a solid color
void setScreenColor(uint32_t color) {
    M5Dial.Display.fillScreen(color);
}

// Set screen to random colors (for visual effect)
void setRandomScreenColors() {
    uint32_t color = M5Dial.Display.color565(rand() % 256, rand() % 256, rand() % 256);
    M5Dial.Display.fillScreen(color);
}

// Set screen color based on a Faction object
void setScreenColor(Faction* faction) {
    String colorCode = faction->colorCode; // Get the color code from the faction object
    int red = strtol(colorCode.substring(1, 3).c_str(), NULL, 16);
    int green = strtol(colorCode.substring(3, 5).c_str(), NULL, 16);
    int blue = strtol(colorCode.substring(5, 7).c_str(), NULL, 16);
    
    uint32_t color = M5Dial.Display.color565(red, green, blue);
    M5Dial.Display.fillScreen(color);
}

// Turn off screen (set to black)
void setScreenOff() {
    M5Dial.Display.fillScreen(BLACK);
}

// Set the screen to white
void setScreenWhite() {
    M5Dial.Display.fillScreen(WHITE);
}

// Set the screen to red
void setScreenRed() {
    M5Dial.Display.fillScreen(RED);
}

// Set the screen to yellow
void setScreenYellow() {
    M5Dial.Display.fillScreen(YELLOW);
}

// Set the screen to green
void setScreenGreen() {
    M5Dial.Display.fillScreen(GREEN);
}

// Success blink pattern (green blink)
void displaySuccessBlink() {
    for (int i = 0; i < SUCCESS_BLINK_COUNT; i++) {
        setScreenOff();
        delay(SUCCESS_BLINK_OFF_DELAY);
        setScreenGreen();
        delay(SUCCESS_BLINK_ON_DELAY);
    }
    setScreenOff();
}

// ============================================
// TONE GENERATION
// ============================================
// All tone function definitions have been moved to the main .ino file
// to resolve linker errors. Declarations remain above.

// ============================================
// DISPLAY & CONTROL
// ============================================

// Failure blink pattern (red blink)
void displayFailBlink() {
    for (int i = 0; i < FAIL_BLINK_COUNT; i++) {
        setScreenOff();
        delay(FAIL_BLINK_OFF_DELAY);
        setScreenRed();
        delay(FAIL_BLINK_ON_DELAY);
    }
    setScreenOff();
}

// Strobe effect for a new round (multi-color blink)
void newRoundBlink() {
    for (int i = 0; i < BOOT_COLOR_CYCLE_COUNT; i++) {
        setRandomScreenColors();
        delay(BOOT_COLOR_CYCLE_DELAY);
        setScreenOff();
        delay(BOOT_COLOR_OFF_DELAY);
    }
}

// Badge swipe loading animation (replaces color flash)
void badgeSwipeBlink() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    int radius = 25;
    int prevX = -1, prevY = -1;
    
    // Draw static elements once
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("Loading...", centerX, centerY + 35);
    
    // Quick loading animation - spinning circle
    for (int i = 0; i < 20; i++) {
        int angle = i * 18; // Full rotation in 20 frames
        
        // Erase previous circle
        if (prevX >= 0 && prevY >= 0) {
            M5Dial.Display.fillCircle(prevX, prevY, 9, BLACK);
        }
        
        // Draw rotating loading circle
        int x = centerX + radius * cos(radians(angle));
        int y = centerY + radius * sin(radians(angle));
        M5Dial.Display.fillCircle(x, y, 8, LOADING_TEXT_COLOR);
        
        prevX = x;
        prevY = y;
        delay(25);
    }
}

// Loot swipe animation with spinning globe
void lootSwipeBlink() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    int globeRadius = 30;
    
    // Draw static elements once
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("Loading...", centerX, centerY + 45);
    
    // Spinning globe animation
    for (int rotation = 0; rotation < 360; rotation += 6) {
        // Clear previous frame
        M5Dial.Display.fillCircle(centerX, centerY - 5, globeRadius + 2, BLACK);
        
        // Draw globe outline
        M5Dial.Display.drawCircle(centerX, centerY - 5, globeRadius, LOADING_TEXT_COLOR);
        
        // Draw horizontal latitude lines
        for (int lat = -2; lat <= 2; lat++) {
            int y = centerY - 5 + (lat * 10);
            if (abs(lat * 10) < globeRadius) {
                int lineWidth = sqrt(globeRadius * globeRadius - (lat * 10) * (lat * 10));
                M5Dial.Display.drawLine(centerX - lineWidth, y, centerX + lineWidth, y, 0x4208); // Dim cyan
            }
        }
        
        // Draw rotating vertical longitude lines
        for (int lon = 0; lon < 6; lon++) {
            int angle = rotation + (lon * 60);
            int offset = globeRadius * cos(radians(angle));
            
            // Draw vertical ellipse for longitude
            for (int y = -globeRadius; y <= globeRadius; y += 2) {
                int ellipseWidth = abs(offset) * sqrt(1.0 - (float)(y * y) / (globeRadius * globeRadius));
                int x = centerX + ellipseWidth;
                int py = centerY - 5 + y;
                
                if (abs(offset) > 2) {
                    M5Dial.Display.drawPixel(x, py, 0x4208); // Dim cyan
                }
            }
        }
        
        // Draw equator highlight
        M5Dial.Display.drawLine(centerX - globeRadius, centerY - 5, centerX + globeRadius, centerY - 5, LOADING_TEXT_COLOR);
        
        delay(30);
    }
}

// Startup animation - Spinning circle
void startupAnimationSpinningCircle() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    int radius = 30;
    int prevX = -1, prevY = -1;
    
    // Draw static elements once
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("GURU", centerX, centerY + 40);
    M5Dial.Display.drawString("GAMES", centerX, centerY + 60);
    
    for (int angle = 0; angle < 360; angle += 6) {
        // Erase previous circle by drawing over it in black
        if (prevX >= 0 && prevY >= 0) {
            M5Dial.Display.fillCircle(prevX, prevY, 9, BLACK);
        }
        
        // Draw new circle
        int x = centerX + radius * cos(radians(angle));
        int y = centerY + radius * sin(radians(angle));
        M5Dial.Display.fillCircle(x, y, 8, STARTUP_ANIMATION_COLOR);
        
        prevX = x;
        prevY = y;
        delay(STARTUP_ANIMATION_SPEED);
    }
}

// Startup animation - Pulsing circle
void startupAnimationPulsing() {
    M5Dial.Display.fillScreen(BLACK);
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    for (int pulse = 0; pulse < 3; pulse++) {
        // Expand
        for (int r = 5; r < 50; r += 3) {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(TEXT_INITIALIZING, centerX, centerY + 60);
            M5Dial.Display.drawCircle(centerX, centerY, r, STARTUP_ANIMATION_COLOR);
            delay(STARTUP_ANIMATION_SPEED);
        }
        // Contract
        for (int r = 50; r > 5; r -= 3) {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(TEXT_INITIALIZING, centerX, centerY + 60);
            M5Dial.Display.fillCircle(centerX, centerY, r, STARTUP_ANIMATION_COLOR);
            delay(STARTUP_ANIMATION_SPEED);
        }
    }
}

// Startup animation - Rainbow cycle
void startupAnimationRainbow() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    for (int hue = 0; hue < 360; hue += 5) {
        // Convert HSV to RGB
        float h = hue / 60.0;
        float s = 1.0;
        float v = 1.0;
        int i = floor(h);
        float f = h - i;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1 - f));
        
        float r, g, b;
        switch(i % 6) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
        
        uint32_t color = M5Dial.Display.color565((int)(r * 255), (int)(g * 255), (int)(b * 255));
        M5Dial.Display.fillScreen(color);
        
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString(TEXT_STARTING, centerX, centerY);
        
        delay(STARTUP_ANIMATION_SPEED);
    }
    M5Dial.Display.fillScreen(BLACK);
}

// Draw custom logo shape - CUSTOMIZE THIS WITH YOUR LOGO!
void drawCustomLogo(int centerX, int centerY, int size, float rotation, uint32_t color) {
    // Example: Drawing a hexagon - replace this with your logo design
    // You can draw multiple shapes to create your logo
    
    int numPoints = 6; // Hexagon
    int x1, y1, x2, y2;
    
    for (int i = 0; i < numPoints; i++) {
        float angle1 = (i * 360.0 / numPoints) + rotation;
        float angle2 = ((i + 1) * 360.0 / numPoints) + rotation;
        
        x1 = centerX + size * cos(radians(angle1));
        y1 = centerY + size * sin(radians(angle1));
        x2 = centerX + size * cos(radians(angle2));
        y2 = centerY + size * sin(radians(angle2));
        
        // Draw outline
        M5Dial.Display.drawLine(x1, y1, x2, y2, color);
        M5Dial.Display.drawLine(x1 + 1, y1, x2 + 1, y2, color);
        M5Dial.Display.drawLine(x1, y1 + 1, x2, y2 + 1, color);
    }
    
    // Add center circle for style
    M5Dial.Display.fillCircle(centerX, centerY, size / 3, color);
    
    // Optional: Add your initials or text in the center
    // M5Dial.Display.setTextColor(BLACK);
    // M5Dial.Display.setTextSize(2);
    // M5Dial.Display.setTextDatum(middle_center);
    // M5Dial.Display.drawString("MW", centerX, centerY);
}

// Alternative: Draw logo from bitmap data
// Uncomment and use this if you have your logo as a bitmap array
/*
void drawCustomLogoBitmap(int centerX, int centerY, float rotation, uint32_t color) {
    // Example bitmap - replace with your logo
    // You can use an online tool to convert your image to a byte array
    // Recommended size: 64x64 pixels or smaller
    
    // const unsigned char myLogo[] = { ...your bitmap data here... };
    // M5Dial.Display.pushImage(centerX - 32, centerY - 32, 64, 64, myLogo);
}
*/

// Startup animation - Custom logo spinning
void startupAnimationCustomLogo() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Complete 2 full rotations
    for (int angle = 0; angle < 720; angle += 10) {
        M5Dial.Display.fillScreen(BLACK);
        
        // Draw your custom logo with rotation
        drawCustomLogo(centerX, centerY - 10, STARTUP_LOGO_SIZE, angle, STARTUP_LOGO_COLOR);
        
        // Optional: Add text below logo
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString(TEXT_INITIALIZING, centerX, centerY + 50);
        
        delay(STARTUP_ANIMATION_SPEED);
    }
}

// Draw text with GTA-style outline effect
void drawGTAText(String text, int x, int y, int textSize, uint32_t mainColor, uint32_t outlineColor) {
    M5Dial.Display.setTextSize(textSize);
    M5Dial.Display.setTextDatum(middle_center);
    
    // Draw outline by drawing text multiple times offset
    M5Dial.Display.setTextColor(outlineColor);
    for (int ox = -2; ox <= 2; ox++) {
        for (int oy = -2; oy <= 2; oy++) {
            if (ox != 0 || oy != 0) {
                M5Dial.Display.drawString(text, x + ox, y + oy);
            }
        }
    }
    
    // Draw main text
    M5Dial.Display.setTextColor(mainColor);
    M5Dial.Display.drawString(text, x, y);
}

// Draw Vice City style text with gradient (pink to cyan)
void drawViceCityText(String text, int x, int y, int textSize) {
    M5Dial.Display.setTextSize(textSize);
    M5Dial.Display.setTextDatum(middle_center);
    
    // Draw thick black outline (4px thick for bold GTA look)
    M5Dial.Display.setTextColor(BLACK);
    for (int ox = -3; ox <= 3; ox++) {
        for (int oy = -3; oy <= 3; oy++) {
            if (ox != 0 || oy != 0) {
                M5Dial.Display.drawString(text, x + ox, y + oy);
            }
        }
    }
    
    // Draw gradient text - simulate by drawing with alternating colors
    // Top half: Pink/Magenta
    M5Dial.Display.setTextColor(GTA_TEXT_COLOR_TOP);
    M5Dial.Display.drawString(text, x, y - 1);
    
    // Middle: Purple blend
    M5Dial.Display.setTextColor(0x781F); // Purple
    M5Dial.Display.drawString(text, x, y);
    
    // Bottom: Cyan
    M5Dial.Display.setTextColor(GTA_TEXT_COLOR_BOTTOM);
    M5Dial.Display.drawString(text, x, y + 1);
}

// GTA-style startup animation - Mission Passed style for GURU GAMES
void startupAnimationGTA() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Phase 1: Green screen fade in (no white flash)
    for (int fade = 0; fade < 25; fade++) {
        int greenValue = fade * 10;
        uint32_t fadeColor = M5Dial.Display.color565(0, greenValue, 0);
        M5Dial.Display.fillScreen(fadeColor);
        delay(STARTUP_ANIMATION_SPEED / 2);
    }
    
    // Fill with full green
    M5Dial.Display.fillScreen(GREEN);
    delay(200);
    
    // Phase 3: "GURU" appears with zoom
    int topY = centerY - 30;
    
    for (int zoom = 1; zoom <= 3; zoom++) {
        M5Dial.Display.fillScreen(GREEN);
        
        // Draw black outline for GURU
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(zoom);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                if (dx != 0 || dy != 0) {
                    M5Dial.Display.drawString("GURU", centerX + dx, topY + dy);
                }
            }
        }
        
        // Draw gold/yellow text for GURU
        uint16_t goldColor = M5Dial.Display.color565(218, 165, 32);
        M5Dial.Display.setTextColor(goldColor);
        M5Dial.Display.drawString("GURU", centerX, topY);
        
        delay(150);
    }
    
    delay(300);
    
    // Phase 4: "GAMES" appears with zoom
    int bottomY = centerY + 25;
    
    for (int zoom = 1; zoom <= 3; zoom++) {
        M5Dial.Display.fillScreen(GREEN);
        
        // Redraw GURU (static)
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(3);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                if (dx != 0 || dy != 0) {
                    M5Dial.Display.drawString("GURU", centerX + dx, topY + dy);
                }
            }
        }
        uint16_t goldColor = M5Dial.Display.color565(218, 165, 32);
        M5Dial.Display.setTextColor(goldColor);
        M5Dial.Display.drawString("GURU", centerX, topY);
        
        // Draw black outline for GAMES
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(zoom);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                if (dx != 0 || dy != 0) {
                    M5Dial.Display.drawString("GAMES", centerX + dx, bottomY + dy);
                }
            }
        }
        
        // Draw white text for GAMES
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("GAMES", centerX, bottomY);
        
        delay(150);
    }
    
    // Phase 5: Hold the screen with subtle pulse
    for (int pulse = 0; pulse < 25; pulse++) {
        M5Dial.Display.fillScreen(GREEN);
        
        // Draw GURU with black outline
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(3);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                if (dx != 0 || dy != 0) {
                    M5Dial.Display.drawString("GURU", centerX + dx, topY + dy);
                }
            }
        }
        
        // Pulsing gold color
        int pulseBrightness = 32 + (pulse % 10);
        uint16_t goldColor = M5Dial.Display.color565(218 + pulseBrightness, 165 + pulseBrightness, 32 + pulseBrightness);
        M5Dial.Display.setTextColor(goldColor);
        M5Dial.Display.drawString("GURU", centerX, topY);
        
        // Draw GAMES with black outline
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(3);
        for (int dx = -3; dx <= 3; dx++) {
            for (int dy = -3; dy <= 3; dy++) {
                if (dx != 0 || dy != 0) {
                    M5Dial.Display.drawString("GAMES", centerX + dx, bottomY + dy);
                }
            }
        }
        
        // Draw white text for GAMES
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("GAMES", centerX, bottomY);
        
        delay(STARTUP_ANIMATION_SPEED * 2);
    }
    
    // Phase 6: Fade out
    for (int fade = 20; fade >= 0; fade--) {
        int greenValue = fade * 12;
        uint32_t fadeColor = M5Dial.Display.color565(0, greenValue, 0);
        M5Dial.Display.fillScreen(fadeColor);
        
        if (fade > 5) {
            // Draw text during fade
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.setTextColor(BLACK);
            M5Dial.Display.setTextSize(3);
            for (int dx = -3; dx <= 3; dx++) {
                for (int dy = -3; dy <= 3; dy++) {
                    if (dx != 0 || dy != 0) {
                        M5Dial.Display.drawString("GURU", centerX + dx, topY + dy);
                        M5Dial.Display.drawString("GAMES", centerX + dx, bottomY + dy);
                    }
                }
            }
            uint16_t goldColor = M5Dial.Display.color565(218, 165, 32);
            M5Dial.Display.setTextColor(goldColor);
            M5Dial.Display.drawString("GURU", centerX, topY);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.drawString("GAMES", centerX, bottomY);
        }
        
        delay(STARTUP_ANIMATION_SPEED);
    }
    
    M5Dial.Display.fillScreen(BLACK);
}

// Main startup animation dispatcher
void playStartupAnimation() {
    if (!STARTUP_ANIMATION_ENABLED) {
        return;
    }
    
    switch(STARTUP_ANIMATION_TYPE) {
        case 1:
            startupAnimationSpinningCircle();
            break;
        case 2:
            startupAnimationPulsing();
            break;
        case 3:
            startupAnimationRainbow();
            break;
        case 4:
            startupAnimationCustomLogo();
            break;
        case 5:
            startupAnimationGTA();
            break;
        default:
            startupAnimationSpinningCircle();
            break;
    }
}

// Draw a single animation frame with text - used during initialization
// Static variables to track previous frame positions
static int prevAnimX = -1;
static int prevAnimY = -1;
static bool animInitialized = false;

void drawAnimationFrame(int frameNumber, String statusText) {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    if (!STARTUP_ANIMATION_ENABLED) {
        M5Dial.Display.fillScreen(BLACK);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString(statusText, centerX, centerY);
        return;
    }
    
    // Initialize static elements only on first frame
    if (!animInitialized || frameNumber == 0) {
        M5Dial.Display.fillScreen(BLACK);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("GURU", centerX, centerY + 40);
        M5Dial.Display.drawString("GAMES", centerX, centerY + 60);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString(statusText, centerX, centerY + 90);
        prevAnimX = -1;
        prevAnimY = -1;
        animInitialized = true;
    }
    
    switch(STARTUP_ANIMATION_TYPE) {
        case 1: { // Spinning circle
            int angle = (frameNumber * 6) % 360;
            int radius = 30;
            
            // Erase previous circle
            if (prevAnimX >= 0 && prevAnimY >= 0) {
                M5Dial.Display.fillCircle(prevAnimX, prevAnimY, 9, BLACK);
            }
            
            // Draw new circle
            int x = centerX + radius * cos(radians(angle));
            int y = centerY + radius * sin(radians(angle)) - 10;
            M5Dial.Display.fillCircle(x, y, 8, STARTUP_ANIMATION_COLOR);
            
            prevAnimX = x;
            prevAnimY = y;
            break;
        }
        case 2: { // Pulsing
            // Erase previous circle
            if (prevAnimX >= 0) {
                M5Dial.Display.fillCircle(centerX, centerY - 10, prevAnimX, BLACK);
            }
            int r = 10 + (abs(frameNumber % 40 - 20) * 2);
            M5Dial.Display.fillCircle(centerX, centerY - 10, r, STARTUP_ANIMATION_COLOR);
            prevAnimX = r;
            break;
        }
        case 3: { // Rainbow
            int hue = (frameNumber * 5) % 360;
            float h = hue / 60.0;
            int i = floor(h);
            float f = h - i;
            float p = 0;
            float q = 1.0 - f;
            float t = f;
            
            float r, g, b;
            switch(i % 6) {
                case 0: r = 1; g = t; b = p; break;
                case 1: r = q; g = 1; b = p; break;
                case 2: r = p; g = 1; b = t; break;
                case 3: r = p; g = q; b = 1; break;
                case 4: r = t; g = p; b = 1; break;
                case 5: r = 1; g = p; b = q; break;
            }
            uint32_t color = M5Dial.Display.color565((int)(r * 255), (int)(g * 255), (int)(b * 255));
            // Erase previous circle
            if (prevAnimX >= 0) {
                M5Dial.Display.fillCircle(centerX, centerY - 10, 31, BLACK);
            }
            M5Dial.Display.fillCircle(centerX, centerY - 10, 30, color);
            prevAnimX = 30;
            break;
        }
        case 4: { // Custom logo
            int angle = (frameNumber * 10) % 360;
            // Full redraw needed for rotating logo
            M5Dial.Display.fillCircle(centerX, centerY - 10, STARTUP_LOGO_SIZE + 5, BLACK);
            drawCustomLogo(centerX, centerY - 10, STARTUP_LOGO_SIZE, angle, STARTUP_LOGO_COLOR);
            break;
        }
        case 5: { // GTA-style text pulse (Vice City colors)
            // This mode requires full redraws due to white background
            M5Dial.Display.fillScreen(WHITE);
            
            // Rotating neon circle
            int neonAngle = (frameNumber * 12) % 360;
            int neonX = centerX + 45 * cos(radians(neonAngle));
            int neonY = centerY + 45 * sin(radians(neonAngle));
            uint32_t neonColor = (frameNumber % 4 < 2) ? GTA_TEXT_COLOR_TOP : GTA_TEXT_COLOR_BOTTOM;
            M5Dial.Display.fillCircle(neonX, neonY, 3, neonColor);
            
            drawViceCityText("GURU", centerX, centerY - 20, 2);
            drawViceCityText("GAMES", centerX, centerY + 15, 1);
            break;
        }
    }
    
    // Draw status text below animation if provided
    if (statusText.length() > 0) {
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString(statusText, centerX, M5Dial.Display.height() - 20);
    }
}

// Fill the screen with faction colors in sections (circular segments)
void setScreenFactionColors(LinkedList<Faction*> factions) {
    // Ensure there is at least one faction to display
    if (factions.size() == 0) {
        setScreenOff();
        return;
    }

    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    int radius = min(centerX, centerY);

    // Calculate angle per faction
    float anglePerFaction = 360.0 / factions.size();

    for (int i = 0; i < factions.size(); i++) {
        Faction* faction = factions.get(i);

        // Parse the faction's color from the color code
        int red = strtol(faction->colorCode.substring(1, 3).c_str(), NULL, 16);
        int green = strtol(faction->colorCode.substring(3, 5).c_str(), NULL, 16);
        int blue = strtol(faction->colorCode.substring(5, 7).c_str(), NULL, 16);
        
        uint32_t color = M5Dial.Display.color565(red, green, blue);

        // Draw a filled arc/segment for this faction
        float startAngle = i * anglePerFaction;
        float endAngle = (i + 1) * anglePerFaction;
        
        // Draw the segment (you may need to implement arc drawing)
        // For simplicity, draw a triangle from center
        M5Dial.Display.fillTriangle(
            centerX, centerY,
            centerX + radius * cos(radians(startAngle)), 
            centerY + radius * sin(radians(startAngle)),
            centerX + radius * cos(radians(endAngle)), 
            centerY + radius * sin(radians(endAngle)),
            color
        );
    }
}

// Set the screen to red, white, and blue segments
void setScreenRedWhiteBlue() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    int radius = min(centerX, centerY);

    // Red section (0-120 degrees)
    M5Dial.Display.fillCircle(centerX, centerY, radius, RED);
    
    // White section (120-240 degrees) - draw over
    M5Dial.Display.fillTriangle(
        centerX, centerY,
        centerX + radius * cos(radians(120)), 
        centerY + radius * sin(radians(120)),
        centerX + radius * cos(radians(240)), 
        centerY + radius * sin(radians(240)),
        WHITE
    );
    
    // Blue section (240-360 degrees)
    M5Dial.Display.fillTriangle(
        centerX, centerY,
        centerX + radius * cos(radians(240)), 
        centerY + radius * sin(radians(240)),
        centerX + radius * cos(radians(0)), 
        centerY + radius * sin(radians(0)),
        0x001F  // Blue
    );
}

// Display text on screen with optional color
void displayText(String text, uint32_t textColor = WHITE, uint32_t bgColor = BLACK) {
    M5Dial.Display.fillScreen(bgColor);
    M5Dial.Display.setTextColor(textColor);
    M5Dial.Display.setCursor(10, M5Dial.Display.height() / 2 - 10);
    M5Dial.Display.println(text);
}

// Display centered text with larger font
void displayCenteredText(String text, uint32_t textColor = WHITE, uint32_t bgColor = BLACK, int textSize = 2) {
    M5Dial.Display.fillScreen(bgColor);
    M5Dial.Display.setTextColor(textColor);
    M5Dial.Display.setTextSize(textSize);
    M5Dial.Display.setTextDatum(middle_center);
    
    // M5Dial display specs: 240x240 circular display
    int displayWidth = M5Dial.Display.width();
    int displayHeight = M5Dial.Display.height();
    int centerX = displayWidth / 2;
    int centerY = displayHeight / 2;
    int radius = centerX - 10; // Account for edge margin
    
    // First pass: estimate number of lines needed for initial Y position calculation
    int lineHeight = 8 * textSize;
    int estimatedLines = (text.length() * 6 * textSize) / (displayWidth - 40) + 1;
    if (estimatedLines > 10) estimatedLines = 10;
    int estimatedTotalHeight = estimatedLines * lineHeight;
    int estimatedStartY = (displayHeight - estimatedTotalHeight) / 2 + lineHeight / 2;
    
    // Calculate available width at the first line's Y position (most restrictive)
    // For circular display: width = 2 * sqrt(radius² - (y - centerY)²)
    int firstLineY = estimatedStartY;
    int yOffset = abs(firstLineY - centerY);
    int availableWidth = displayWidth; // Default to full width
    
    if (yOffset < radius) {
        // Calculate width at this Y position on the circle
        float widthAtY = 2.0 * sqrt(radius * radius - yOffset * yOffset);
        availableWidth = (int)widthAtY - 20; // 20px margin total
    } else {
        availableWidth = displayWidth - 40; // Fallback with margins
    }
    
    // Calculate max characters per line based on the narrowest point (first line)
    int maxCharsPerLine = availableWidth / (6 * textSize); // 6 pixels per char base width
    if (maxCharsPerLine < 5) maxCharsPerLine = 5; // Minimum 5 chars
    
    // Split text into lines based on the calculated max width
    String lines[10]; // Support up to 10 lines
    int lineCount = 0;
    String currentLine = "";
    int textLength = text.length();
    
    for (int i = 0; i < textLength && lineCount < 10; i++) {
        char c = text.charAt(i);
        
        // Check if adding this character would exceed the line width
        if (currentLine.length() + 1 > maxCharsPerLine) {
            // Try to break at last space
            int lastSpace = currentLine.lastIndexOf(' ');
            if (lastSpace > 0 && lastSpace > maxCharsPerLine * 0.5) {
                // Break at space
                lines[lineCount++] = currentLine.substring(0, lastSpace);
                currentLine = currentLine.substring(lastSpace + 1) + c;
            } else {
                // No good break point, force break
                lines[lineCount++] = currentLine;
                currentLine = String(c);
            }
        } else {
            currentLine += c;
        }
    }
    
    // Add the last line
    if (currentLine.length() > 0 && lineCount < 10) {
        lines[lineCount++] = currentLine;
    }
    
    // Recalculate vertical spacing with actual line count
    int totalHeight = lineCount * lineHeight;
    int startY = (displayHeight - totalHeight) / 2 + lineHeight / 2;
    
    // Draw each line centered
    for (int i = 0; i < lineCount; i++) {
        int y = startY + (i * lineHeight);
        M5Dial.Display.drawString(lines[i], centerX, y);
    }
}

// Display black market themed scan badge screen
void displayBlackMarketScan() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(PROMPT_BG_COLOR);
    
    // Draw corner brackets (cyberpunk style)
    int bracketSize = 15;
    int margin = 20;
    uint32_t accentColor = THEME_ACCENT_COLOR;  // Use theme accent color
    
    // Top-left corner
    M5Dial.Display.drawLine(margin, margin, margin + bracketSize, margin, accentColor);
    M5Dial.Display.drawLine(margin, margin, margin, margin + bracketSize, accentColor);
    
    // Top-right corner
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, margin, M5Dial.Display.width() - margin - bracketSize, margin, accentColor);
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, margin, M5Dial.Display.width() - margin, margin + bracketSize, accentColor);
    
    // Bottom-left corner
    M5Dial.Display.drawLine(margin, M5Dial.Display.height() - margin, margin + bracketSize, M5Dial.Display.height() - margin, accentColor);
    M5Dial.Display.drawLine(margin, M5Dial.Display.height() - margin, margin, M5Dial.Display.height() - margin - bracketSize, accentColor);
    
    // Bottom-right corner
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, M5Dial.Display.height() - margin, M5Dial.Display.width() - margin - bracketSize, M5Dial.Display.height() - margin, accentColor);
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, M5Dial.Display.height() - margin, M5Dial.Display.width() - margin, M5Dial.Display.height() - margin - bracketSize, accentColor);
    
    // Draw scanning lines effect
    for (int i = 0; i < 3; i++) {
        int y = margin + 30 + (i * 20);
        uint32_t dimAccent = M5Dial.Display.color565(
            (accentColor >> 11) * 0.3,
            ((accentColor >> 5) & 0x3F) * 0.3,
            (accentColor & 0x1F) * 0.3
        );
        M5Dial.Display.drawLine(margin + 5, y, M5Dial.Display.width() - margin - 5, y, dimAccent);
    }
    
    // Top header text
    M5Dial.Display.setTextColor(accentColor);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("[SECURE TERMINAL]", centerX, margin + 5);
    
    // Main instruction - cryptic/atmospheric
    M5Dial.Display.setTextColor(PROMPT_TEXT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("IDENTIFY", centerX, centerY - 15);
    M5Dial.Display.drawString("YOURSELF", centerX, centerY + 10);
    
    // Bottom status text
    M5Dial.Display.setTextColor(accentColor);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("> AWAITING CREDENTIALS <", centerX, M5Dial.Display.height() - margin - 5);
    
    // Blinking cursor/indicator
    if (millis() % 1000 < 500) {
        M5Dial.Display.fillCircle(centerX, centerY + 40, 3, accentColor);
    }
}

// Display black market themed scan loot screen
void displayBlackMarketScanLoot() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    M5Dial.Display.fillScreen(PROMPT_BG_COLOR);
    
    // Draw corner brackets
    int bracketSize = 15;
    int margin = 20;
    uint32_t accentColor = THEME_ACCENT_COLOR;
    
    // Top-left corner
    M5Dial.Display.drawLine(margin, margin, margin + bracketSize, margin, accentColor);
    M5Dial.Display.drawLine(margin, margin, margin, margin + bracketSize, accentColor);
    
    // Top-right corner
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, margin, M5Dial.Display.width() - margin - bracketSize, margin, accentColor);
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, margin, M5Dial.Display.width() - margin, margin + bracketSize, accentColor);
    
    // Bottom-left corner
    M5Dial.Display.drawLine(margin, M5Dial.Display.height() - margin, margin + bracketSize, M5Dial.Display.height() - margin, accentColor);
    M5Dial.Display.drawLine(margin, M5Dial.Display.height() - margin, margin, M5Dial.Display.height() - margin - bracketSize, accentColor);
    
    // Bottom-right corner
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, M5Dial.Display.height() - margin, M5Dial.Display.width() - margin - bracketSize, M5Dial.Display.height() - margin, accentColor);
    M5Dial.Display.drawLine(M5Dial.Display.width() - margin, M5Dial.Display.height() - margin, M5Dial.Display.width() - margin, M5Dial.Display.height() - margin - bracketSize, accentColor);
    
    // Draw globe icon for loot
    int globeSize = 20;
    M5Dial.Display.drawCircle(centerX, centerY - 25, globeSize, accentColor);
    // Horizontal lines
    M5Dial.Display.drawLine(centerX - globeSize, centerY - 25, centerX + globeSize, centerY - 25, accentColor);
    // Vertical line
    M5Dial.Display.drawLine(centerX, centerY - 25 - globeSize, centerX, centerY - 25 + globeSize, accentColor);
    
    // Top header
    M5Dial.Display.setTextColor(accentColor);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("[SECURE TERMINAL]", centerX, margin + 5);
    
    // Main instruction
    M5Dial.Display.setTextColor(SCAN_LOOT_TEXT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("SCAN", centerX, centerY + 10);
    M5Dial.Display.drawString("PACKAGE", centerX, centerY + 35);
    
    // Bottom status
    M5Dial.Display.setTextColor(accentColor);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("> AWAITING CARGO DATA <", centerX, M5Dial.Display.height() - margin - 5);
    
    // Blinking indicator
    if (millis() % 1000 < 500) {
        M5Dial.Display.fillCircle(centerX, centerY + 55, 3, accentColor);
    }
}

// Display multi-line centered text
void displayMultiLineText(String line1, String line2, uint32_t textColor = WHITE, uint32_t bgColor = BLACK) {
    M5Dial.Display.fillScreen(bgColor);
    M5Dial.Display.setTextColor(textColor);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(line1, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
    M5Dial.Display.drawString(line2, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
}

// Display confirmation prompt
void displayConfirmPrompt(String message = "Press to\nConfirm") {
    M5Dial.Display.fillScreen(GREEN);
    M5Dial.Display.setTextColor(BLACK);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("Press to", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
    M5Dial.Display.drawString("Confirm", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
}

// Show loading animation
void showLoadingAnimation() {
    for (int i = 0; i < 360; i += 10) {
        M5Dial.Display.fillScreen(BLACK);
        int centerX = M5Dial.Display.width() / 2;
        int centerY = M5Dial.Display.height() / 2;
        int x = centerX + 30 * cos(radians(i));
        int y = centerY + 30 * sin(radians(i));
        M5Dial.Display.fillCircle(x, y, 10, WHITE);
        delay(50);
    }
}

// Show loading with text
void displayLoading(String text = "Loading...") {
    M5Dial.Display.fillScreen(LOADING_BG_COLOR);
    M5Dial.Display.setTextColor(LOADING_TEXT_COLOR);
    M5Dial.Display.setTextSize(DEFAULT_TEXT_SIZE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(text, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
}

int textWidthForSize(const String& text, int textSize) {
    M5Dial.Display.setTextSize(textSize);
    return (int)M5Dial.Display.textWidth(text.length() > 0 ? text : " ");
}

int textHeightForSize(const String& text, int textSize) {
    (void)text;
    M5Dial.Display.setTextSize(textSize);
    return (int)M5Dial.Display.fontHeight();
}

void trimTextToWidthWithEllipsis(String& text, int textSize, int maxWidth) {
    if (textWidthForSize(text, textSize) <= maxWidth) return;
    while (text.length() > 0) {
        String candidate = text + "...";
        if (textWidthForSize(candidate, textSize) <= maxWidth) {
            text = candidate;
            return;
        }
        text.remove(text.length() - 1);
    }
    text = "...";
}

void wrapTextForWidth(const String& source, int textSize, int maxWidth, int maxLines, String lines[], int& lineCount, bool& truncated) {
    lineCount = 0;
    truncated = false;

    String text = source;
    text.replace("\n", " ");
    text.trim();

    if (text.length() == 0) {
        lines[0] = "";
        lineCount = 1;
        return;
    }

    int cursor = 0;
    while (cursor < text.length()) {
        while (cursor < text.length() && text[cursor] == ' ') cursor++;
        if (cursor >= text.length()) break;

        int nextSpace = text.indexOf(' ', cursor);
        String word = (nextSpace < 0) ? text.substring(cursor) : text.substring(cursor, nextSpace);
        cursor = (nextSpace < 0) ? text.length() : nextSpace + 1;

        bool pendingWord = true;
        while (pendingWord) {
            if (lineCount >= maxLines) {
                truncated = true;
                return;
            }

            String current = lines[lineCount];
            String candidate = current.length() > 0 ? current + " " + word : word;

            if (textWidthForSize(candidate, textSize) <= maxWidth) {
                lines[lineCount] = candidate;
                pendingWord = false;
            } else if (current.length() == 0) {
                // Force-break very long tokens character-by-character.
                String piece = "";
                int splitPos = 0;
                while (splitPos < word.length()) {
                    String tryPiece = piece + word[splitPos];
                    if (textWidthForSize(tryPiece, textSize) <= maxWidth) {
                        piece = tryPiece;
                        splitPos++;
                    } else {
                        break;
                    }
                }

                if (piece.length() == 0) {
                    piece = word.substring(0, 1);
                    splitPos = 1;
                }

                lines[lineCount] = piece;
                word = word.substring(splitPos);
                pendingWord = word.length() > 0;
                lineCount++;
            } else {
                lineCount++;
            }
        }
    }

    if (lineCount < maxLines && lines[lineCount].length() > 0) {
        lineCount++;
    }

    if (lineCount == 0) {
        lines[0] = source;
        lineCount = 1;
    }
}

void drawAdaptiveCenteredTextBlock(const String& text, int centerX, int centerY, int maxWidth, int maxLines, int preferredSize, int minSize, uint32_t color) {
    String bestLines[4];
    int bestLineCount = 0;
    int bestSize = minSize;

    for (int size = preferredSize; size >= minSize; size--) {
        String lines[4] = {"", "", "", ""};
        int lineCount = 0;
        bool truncated = false;
        wrapTextForWidth(text, size, maxWidth, maxLines, lines, lineCount, truncated);
        if (!truncated && lineCount <= maxLines) {
            bestSize = size;
            bestLineCount = lineCount;
            for (int i = 0; i < lineCount; i++) bestLines[i] = lines[i];
            break;
        }
        if (size == minSize) {
            bestLineCount = min(lineCount, maxLines);
            for (int i = 0; i < bestLineCount; i++) bestLines[i] = lines[i];
            if (bestLineCount > 0) {
                trimTextToWidthWithEllipsis(bestLines[bestLineCount - 1], size, maxWidth);
            }
        }
    }

    M5Dial.Display.setTextColor(color);
    M5Dial.Display.setTextSize(bestSize);
    M5Dial.Display.setTextDatum(middle_center);

    int lineHeight = textHeightForSize("Ag", bestSize) + 4;
    int blockHeight = bestLineCount * lineHeight;
    int firstY = centerY - (blockHeight / 2) + (lineHeight / 2);

    for (int i = 0; i < bestLineCount; i++) {
        M5Dial.Display.drawString(bestLines[i], centerX, firstY + (i * lineHeight));
    }
}

void drawConfirmCountdownRing(unsigned long elapsedMs, unsigned long totalMs) {
    const int centerX = M5Dial.Display.width() / 2;
    const int centerY = M5Dial.Display.height() / 2;
    const int outerRadius = (min(M5Dial.Display.width(), M5Dial.Display.height()) / 2) - 3;
    const int innerRadius = outerRadius - 8;
    const int startAngle = 270; // top
    const uint16_t ringBaseColor = 0x39E7; // soft gray

    float progress = 0.0f;
    if (totalMs > 0) {
        progress = (float)elapsedMs / (float)totalMs;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
    }

    int sweepDeg = (int)(progress * 360.0f + 0.5f);
    if (sweepDeg < 0) sweepDeg = 0;
    if (sweepDeg > 360) sweepDeg = 360;

    // Draw full base ring first (remaining time color).
    M5Dial.Display.drawArc(centerX, centerY, outerRadius, innerRadius, 0, 360, ringBaseColor);

    // Overlay elapsed section (countdown progress).
    if (sweepDeg > 0) {
        int endAngle = startAngle + sweepDeg;
        if (endAngle <= 360) {
            M5Dial.Display.drawArc(centerX, centerY, outerRadius, innerRadius, startAngle, endAngle, CONFIRM_TEXT_COLOR);
        } else {
            M5Dial.Display.drawArc(centerX, centerY, outerRadius, innerRadius, startAngle, 360, CONFIRM_TEXT_COLOR);
            M5Dial.Display.drawArc(centerX, centerY, outerRadius, innerRadius, 0, endAngle - 360, CONFIRM_TEXT_COLOR);
        }
    }
}

// Display player info (name and faction)
void displayPlayerInfo(String playerName, String additional = "") {
    M5Dial.Display.fillScreen(PLAYER_INFO_BG_COLOR);
    M5Dial.Display.setTextColor(PLAYER_INFO_TEXT_COLOR);
    M5Dial.Display.setTextSize(PLAYER_INFO_LABEL_SIZE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(TEXT_PLAYER_LABEL, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
    drawAdaptiveCenteredTextBlock(playerName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 8, M5Dial.Display.width() - 24, 2, PLAYER_INFO_NAME_SIZE, 1, PLAYER_INFO_TEXT_COLOR);
    if (additional.length() > 0) {
        drawAdaptiveCenteredTextBlock(additional, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 46, M5Dial.Display.width() - 20, 1, PLAYER_INFO_FACTION_SIZE, 1, PLAYER_INFO_TEXT_COLOR);
    }
}

// Display item info
void displayItemInfo(String itemName, String additional = "") {
    M5Dial.Display.fillScreen(LOOT_INFO_BG_COLOR);
    M5Dial.Display.setTextColor(LOOT_INFO_TEXT_COLOR);
    M5Dial.Display.setTextSize(LOOT_INFO_LABEL_SIZE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(TEXT_ITEM_LABEL, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
    drawAdaptiveCenteredTextBlock(itemName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 8, M5Dial.Display.width() - 24, 2, LOOT_INFO_NAME_SIZE, 1, LOOT_INFO_TEXT_COLOR);
    if (additional.length() > 0) {
        drawAdaptiveCenteredTextBlock(additional, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 46, M5Dial.Display.width() - 20, 1, LOOT_INFO_TYPE_SIZE, 1, LOOT_INFO_TEXT_COLOR);
    }
}

// Display transaction summary
void displayTransactionSummary(String playerName, String itemName) {
    M5Dial.Display.fillScreen(CONFIRM_BG_COLOR);
    M5Dial.Display.setTextColor(CONFIRM_TEXT_COLOR);
    M5Dial.Display.setTextSize(CONFIRM_SUMMARY_SIZE);
    M5Dial.Display.setTextDatum(middle_center);
    drawAdaptiveCenteredTextBlock(playerName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 46, M5Dial.Display.width() - 24, 2, 2, 1, CONFIRM_TEXT_COLOR);
    M5Dial.Display.drawString(TEXT_GETS_LABEL, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
    drawAdaptiveCenteredTextBlock(itemName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 2, M5Dial.Display.width() - 24, 2, 2, 1, CONFIRM_TEXT_COLOR);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("TAP SCREEN OR PRESS", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 42);
    M5Dial.Display.drawString("AUTO CONFIRM IN 30s", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 56);
    drawConfirmCountdownRing(0, CONFIRM_AUTO_CONFIRM_MS);
}

// ============================================
// ADMIN MODE FUNCTIONS
// ============================================
// Admin functions now in separate header files:
// - AdminDeviceInfo.h - Device info display
// - AdminWiFi.h - WiFi status, scan, connect functions
// - AdminLootLock.h - Loot lock/unlock management

// Display admin menu
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
            
        case 6: // Buy Station 2 - Backpack icon
            M5Dial.Display.drawRect(centerX - 12, centerY - 8, 24, 20, color);
            M5Dial.Display.fillRect(centerX - 10, centerY - 6, 20, 16, color);
            M5Dial.Display.drawLine(centerX - 8, centerY - 8, centerX - 8, centerY - 15, color);
            M5Dial.Display.drawLine(centerX + 8, centerY - 8, centerX + 8, centerY - 15, color);
            M5Dial.Display.drawArc(centerX - 8, centerY - 15, 3, 2, 180, 360, color);
            M5Dial.Display.drawArc(centerX + 8, centerY - 15, 3, 2, 180, 360, color);
            M5Dial.Display.fillRect(centerX - 5, centerY - 2, 10, 3, BLACK);
            break;
            
        case 7: // Loot Transfer - Two badges with arrow icon
            // Source badge (left)
            M5Dial.Display.drawRoundRect(centerX - 20, centerY - 12, 14, 20, 2, color);
            M5Dial.Display.fillCircle(centerX - 13, centerY - 6, 3, color);
            M5Dial.Display.fillRect(centerX - 16, centerY, 6, 8, color);
            // Arrow
            M5Dial.Display.fillTriangle(centerX + 2, centerY, centerX + 8, centerY - 4, centerX + 8, centerY + 4, YELLOW);
            // Target badge (right)
            M5Dial.Display.drawRoundRect(centerX + 10, centerY - 12, 14, 20, 2, GREEN);
            M5Dial.Display.fillCircle(centerX + 17, centerY - 6, 3, GREEN);
            M5Dial.Display.fillRect(centerX + 14, centerY, 6, 8, GREEN);
            break;
            
        case 8: // Relay - Antenna/Tower icon
            M5Dial.Display.fillRect(centerX - 2, centerY - 20, 4, 40, color);
            M5Dial.Display.fillTriangle(centerX, centerY - 25, centerX - 10, centerY - 15, centerX + 10, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 15, centerY - 10, centerX - 25, centerY - 15, color);
            M5Dial.Display.drawLine(centerX + 15, centerY - 10, centerX + 25, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 15, centerY, centerX - 25, centerY - 5, color);
            M5Dial.Display.drawLine(centerX + 15, centerY, centerX + 25, centerY - 5, color);
            break;
            
        case 9: // Photobooth - Camera icon
            M5Dial.Display.drawRoundRect(centerX - 20, centerY - 13, 40, 28, 4, color);
            M5Dial.Display.fillRect(centerX - 12, centerY - 19, 24, 8, color);
            M5Dial.Display.drawCircle(centerX, centerY + 1, 9, color);
            M5Dial.Display.fillCircle(centerX, centerY + 1, 4, color);
            M5Dial.Display.fillCircle(centerX + 13, centerY - 7, 2, color);
            break;
            
        case 10: // Map Mode - Map icon
            // Map outline
            M5Dial.Display.drawRect(centerX - 18, centerY - 12, 36, 24, color);
            // Folding lines
            M5Dial.Display.drawLine(centerX - 6, centerY - 12, centerX - 6, centerY + 12, color);
            M5Dial.Display.drawLine(centerX + 6, centerY - 12, centerX + 6, centerY + 12, color);
            // Location marker
            M5Dial.Display.fillCircle(centerX, centerY - 3, 3, RED);
            M5Dial.Display.fillTriangle(centerX, centerY + 1, centerX - 3, centerY - 1, centerX + 3, centerY - 1, RED);
            break;
            
        case 11: // Dev Mode - Gear/wrench icon
            // Gear
            M5Dial.Display.drawCircle(centerX - 5, centerY - 5, 8, color);
            M5Dial.Display.fillCircle(centerX - 5, centerY - 5, 4, color);
            for (int i = 0; i < 4; i++) {
                float angle = i * PI / 2;
                int x1 = centerX - 5 + cos(angle) * 6;
                int y1 = centerY - 5 + sin(angle) * 6;
                int x2 = centerX - 5 + cos(angle) * 10;
                int y2 = centerY - 5 + sin(angle) * 10;
                M5Dial.Display.drawLine(x1, y1, x2, y2, color);
                M5Dial.Display.drawLine(x1+1, y1, x2+1, y2, color);
            }
            // Wrench
            M5Dial.Display.drawLine(centerX + 8, centerY + 8, centerX + 15, centerY + 15, color);
            M5Dial.Display.drawLine(centerX + 7, centerY + 9, centerX + 14, centerY + 16, color);
            M5Dial.Display.drawCircle(centerX + 6, centerY + 6, 2, color);
            break;
            
        case 12: // View Variables - List icon
            M5Dial.Display.drawLine(centerX - 18, centerY - 15, centerX + 18, centerY - 15, color);
            M5Dial.Display.drawLine(centerX - 18, centerY - 5, centerX + 18, centerY - 5, color);
            M5Dial.Display.drawLine(centerX - 18, centerY + 5, centerX + 18, centerY + 5, color);
            M5Dial.Display.drawLine(centerX - 18, centerY + 15, centerX + 18, centerY + 15, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY - 15, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY - 5, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY + 5, 2, color);
            M5Dial.Display.fillCircle(centerX - 15, centerY + 15, 2, color);
            break;

        case 13: // Clear Variables - Eraser/trash icon
            M5Dial.Display.drawRoundRect(centerX - 15, centerY - 10, 30, 24, 3, color);
            M5Dial.Display.drawLine(centerX - 11, centerY - 16, centerX + 11, centerY - 16, color);
            M5Dial.Display.drawLine(centerX - 7, centerY - 20, centerX + 7, centerY - 20, color);
            M5Dial.Display.drawLine(centerX - 7, centerY - 4, centerX + 7, centerY + 10, color);
            M5Dial.Display.drawLine(centerX + 7, centerY - 4, centerX - 7, centerY + 10, color);
            break;

        case 14: // Exit - Door with arrow icon
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
    String modeText = "";
    if (currentMode == MODE_BUY_STATION) {
        modeText = "BUY";
    } else if (currentMode == MODE_BUY_STATION_2) {
        modeText = "BUY2";
    } else if (currentMode == MODE_BACKPACK) {
        modeText = "BP";
    } else if (currentMode == MODE_LOOT_TRANSFER) {
        modeText = "LT";
    } else if (currentMode == MODE_RELAY) {
        modeText = "RELAY";
    } else if (currentMode == MODE_PHOTOBOOTH) {
        modeText = "PHOTO";
    } else if (currentMode == MODE_MAP) {
        modeText = "MAP";
    } else {
        modeText = "???";
    }
    uint16_t modeColor = (currentMode == MODE_BUY_STATION || currentMode == MODE_BUY_STATION_2 || currentMode == MODE_BACKPACK || currentMode == MODE_LOOT_TRANSFER) ? GREEN : THEME_ACCENT_COLOR;
    M5Dial.Display.setTextColor(modeColor, BLACK);
    M5Dial.Display.drawString(modeText, 235, 5);
    
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
        "Photobooth",
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
    
    // Show Dev Mode status indicator if on Dev Mode item
    if (selectedItem == 11) {
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(bs2DevMode ? GREEN : 0x7BEF, BLACK);
        M5Dial.Display.drawString(bs2DevMode ? "ENABLED" : "DISABLED", centerX, 205);
    }
    
    // Navigation hints
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF, BLACK);
    M5Dial.Display.drawString("Rotate: Navigate | Press: Select", centerX, 220);
}

// ===================================================================
// ADMIN WIFI FUNCTIONS - Moved to AdminWiFi.h
// displayWiFiStatus, displayWiFiReconnecting, displayWiFiScanning,
// displayWiFiNetworkList, displayPasswordEntry, displayWiFiConnecting,
// displayWiFiConnectionResult
// ===================================================================

// ===================================================================
// ADMIN LOOT LOCK FUNCTION - Moved to AdminLootLock.h
// displayLootFlipMode
// ==================================================================

// Display relay-style scan posting screen
void displayRelayPostingScreen(const char* headerLine1, const char* headerLine2, const char* instructionLine1, const char* instructionLine2, String lastUuid = "", String lastResponse = "", bool success = false) {
    int centerX = M5Dial.Display.width() / 2;
    
    M5Dial.Display.fillScreen(BLACK);
    
    // Header
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(headerLine1, centerX, 5);
    M5Dial.Display.drawString(headerLine2, centerX, 25);
    
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
    M5Dial.Display.drawString(instructionLine1, centerX, 80);
    M5Dial.Display.drawString(instructionLine2, centerX, 95);
    
    // Last scan results
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
        
        // Status
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Status:", 20, 150);
        M5Dial.Display.setTextColor(success ? GREEN : RED);
        M5Dial.Display.drawString(success ? "POSTED" : "FAILED", 90, 150);
        
        // Response code/message
        if (lastResponse.length() > 0) {
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Code:", 20, 170);
            M5Dial.Display.setTextColor(success ? GREEN : RED);
            M5Dial.Display.drawString(lastResponse, 90, 170);
        }
    } else {
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("Ready to scan...", centerX, 135);
    }
    
    // Exit instruction
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Scan admin for menu", centerX, M5Dial.Display.height() - 5);
}

// Display relay mode screen
void displayRelayMode(String lastUuid = "", String lastResponse = "", bool success = false) {
    displayRelayPostingScreen("LOAD TAG", "MODE", "Scan badge to load", "into game", lastUuid, lastResponse, success);
}

// Display photobooth mode screen
void displayPhotoboothMode(String lastUuid = "", String lastResponse = "", bool success = false) {
    displayRelayPostingScreen("PHOTOBOOTH", "READER", "Scan badge for", "photo booth", lastUuid, lastResponse, success);
}

// Display admin access granted animation
void displayAdminAccessGranted() {
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Flash effect
    for (int i = 0; i < 3; i++) {
        M5Dial.Display.fillScreen(BLACK);
        delay(100);
        M5Dial.Display.fillScreen(THEME_ACCENT_COLOR);
        M5Dial.Display.setTextColor(BLACK);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("ADMIN", centerX, centerY - 15);
        M5Dial.Display.drawString("ACCESS", centerX, centerY + 15);
        delay(100);
    }
    
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("ACCESS", centerX, centerY - 10);
    M5Dial.Display.drawString("GRANTED", centerX, centerY + 15);
    delay(800);
}

// ============================================
// BUY STATION 2 DISPLAYS
// ============================================

// Display Buy Station 2 scan prompt
void displayBuyStation2Scan() {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Title
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("BUY STATION", centerX, 20);
    
    // Subtitle
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Buy Station", centerX, 45);
    
    // Icon - RFID card with person
    // Draw card background
    M5Dial.Display.fillRoundRect(centerX - 35, centerY - 25, 70, 45, 5, 0x2965); // Dark blue card
    M5Dial.Display.drawRoundRect(centerX - 35, centerY - 25, 70, 45, 5, THEME_ACCENT_COLOR); // Card border
    
    // Draw person icon on card (left side)
    // Head
    M5Dial.Display.fillCircle(centerX - 15, centerY - 8, 8, WHITE);
    // Body
    M5Dial.Display.fillRect(centerX - 20, centerY + 2, 10, 12, WHITE);
    // Arms
    M5Dial.Display.fillTriangle(centerX - 20, centerY + 3, centerX - 25, centerY + 10, centerX - 20, centerY + 10, WHITE);
    M5Dial.Display.fillTriangle(centerX - 10, centerY + 3, centerX - 5, centerY + 10, centerX - 10, centerY + 10, WHITE);
    
    // Draw card details (lines representing text on right side)
    M5Dial.Display.drawLine(centerX + 2, centerY - 12, centerX + 28, centerY - 12, THEME_ACCENT_COLOR);
    M5Dial.Display.drawLine(centerX + 2, centerY - 5, centerX + 28, centerY - 5, 0x7BEF);
    M5Dial.Display.drawLine(centerX + 2, centerY + 2, centerX + 20, centerY + 2, 0x7BEF);
    M5Dial.Display.drawLine(centerX + 2, centerY + 9, centerX + 25, centerY + 9, 0x7BEF);
    
    // Instruction
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("SCAN PLAYER", centerX, centerY + 60);
}

// Display badge info with backpack count
void displayBuyStation2BadgeInfo(String badgeName, String faction, int backpackCount, int lootCount) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    
    // Badge name
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(badgeName, centerX, 20);
    
    // Faction
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString(faction, centerX, 45);
    
    // Backpack value
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("$" + String(backpackCount), centerX, 100);
    
    // Loot items count
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(String(lootCount) + " Items", centerX, 140);
    
    // Instruction
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Press to claim", centerX, M5Dial.Display.height() - 5);
}

// Display claiming progress
void displayBuyStation2Claiming(int currentItem, int totalItems, String itemName) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Title
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("CLAIMING", centerX, 20);
    
    // Progress
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(String(currentItem) + "/" + String(totalItems), centerX, centerY - 20);
    
    // Item name
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString(itemName, centerX, centerY + 15);
    
    // Progress bar
    int barWidth = 180;
    int barHeight = 10;
    int barX = centerX - barWidth / 2;
    int barY = centerY + 40;
    
    M5Dial.Display.drawRect(barX, barY, barWidth, barHeight, WHITE);
    int fillWidth = (barWidth * currentItem) / totalItems;
    M5Dial.Display.fillRect(barX + 2, barY + 2, fillWidth - 4, barHeight - 4, GREEN);
}

// Display claiming results
void displayBuyStation2Results(int successCount, int totalCount) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    bool allSuccess = (successCount == totalCount);
    
    // Title
    M5Dial.Display.setTextColor(allSuccess ? GREEN : ORANGE);
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(allSuccess ? "SUCCESS!" : "PARTIAL", centerX, centerY - 30);
    
    // Results
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(String(successCount) + "/" + String(totalCount), centerX, centerY + 10);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("items claimed", centerX, centerY + 35);
    
    // Instruction
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Scan next badge", centerX, M5Dial.Display.height() - 5);
}

// Display loot browser - scroll through collected items
void displayBuyStation2LootBrowser(int currentIndex, int totalItems, String itemName, String playerName, String rewardText) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Player class and multiplier (if present)
    if (bs2PlayerClass.length() > 0 && bs2ClassMultiplier > 1) {
        M5Dial.Display.setTextColor(ORANGE);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(top_center);
        M5Dial.Display.drawString(bs2PlayerClass + " x" + String(bs2ClassMultiplier), centerX, 12);
    }
    
    // Item counter
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("ITEM " + String(currentIndex + 1) + "/" + String(totalItems), centerX, centerY - 60);
    
    // Item name (larger) - split into 2 lines if too long - all items are GREEN (unlocked)
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(2);
    if (itemName.length() > 15) {
        // Find a good split point (space or middle)
        int splitPos = 15;
        for (int i = 14; i >= 8; i--) {
            if (itemName.charAt(i) == ' ') {
                splitPos = i;
                break;
            }
        }
        String line1 = itemName.substring(0, splitPos);
        String line2 = itemName.substring(splitPos);
        line2.trim(); // Remove leading space if split at space
        M5Dial.Display.drawString(line1, centerX, centerY - 30);
        M5Dial.Display.drawString(line2, centerX, centerY - 10);
    } else {
        M5Dial.Display.drawString(itemName, centerX, centerY - 20);
    }
    
    // Rewards (smaller text, below item name)
    if (rewardText.length() > 0) {
        M5Dial.Display.setTextColor(YELLOW);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextDatum(middle_center);
        
        // Split reward text by newlines and display each line
        int yPos = centerY + 15;
        int startPos = 0;
        int endPos = rewardText.indexOf('\n');
        
        while (endPos != -1 || startPos < rewardText.length()) {
            String line;
            if (endPos != -1) {
                line = rewardText.substring(startPos, endPos);
                startPos = endPos + 1;
                endPos = rewardText.indexOf('\n', startPos);
            } else {
                line = rewardText.substring(startPos);
                startPos = rewardText.length();
            }
            
            M5Dial.Display.drawString(line, centerX, yPos);
            yPos += 12;
        }
    }
    
    // Claim button instruction
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("TAP SCREEN OR PRESS", centerX, centerY + 68);
    M5Dial.Display.drawString("TO CLAIM CART", centerX, centerY + 80);
}

// ============================================
// LOOT TRANSFER MODE DISPLAYS
// ============================================

// Display Loot Transfer scan prompt
void displayLootTransferScan() {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Title
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("LOOT TRANSFER", centerX, 35);
    
    // Subtitle
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Faction Inventory", centerX, 55);
    
    // Icon - Two badges with arrow
    // Source badge (left)
    M5Dial.Display.fillRoundRect(centerX - 55, centerY - 20, 35, 50, 3, 0x2965);
    M5Dial.Display.drawRoundRect(centerX - 55, centerY - 20, 35, 50, 3, THEME_ACCENT_COLOR);
    
    // Arrow (middle)
    M5Dial.Display.fillTriangle(centerX - 10, centerY, centerX + 5, centerY - 8, centerX + 5, centerY + 8, YELLOW);
    M5Dial.Display.fillRect(centerX - 10, centerY - 3, 10, 6, YELLOW);
    
    // Target badge (right)
    M5Dial.Display.fillRoundRect(centerX + 20, centerY - 20, 35, 50, 3, 0x2965);
    M5Dial.Display.drawRoundRect(centerX + 20, centerY - 20, 35, 50, 3, GREEN);
    
    // Instruction
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("SCAN BADGE", centerX, centerY + 55);
}

// Display faction inventory
void displayLootTransferInventory(String badgeName, String faction, int inventoryCount) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    
    // Badge name
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(badgeName, centerX, 20);
    
    // Faction
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString(faction, centerX, 45);
    
    // Inventory count
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(String(inventoryCount), centerX, 100);
    
    // Label
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("ITEMS", centerX, 140);
    
    // Instruction
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextDatum(bottom_center);
    M5Dial.Display.drawString("Turn to browse", centerX, M5Dial.Display.height() - 5);
}

// Display item browser for transfer
void displayLootTransferItemBrowser(int currentIndex, int totalItems, String itemName, int amount, bool canTransfer) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Item counter at top
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString(String(currentIndex + 1) + " / " + String(totalItems), centerX, 5);
    
    // Item name (centered, word wrapped)
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    
    if (itemName.length() > 12) {
        // Split into two lines
        int splitPos = itemName.indexOf(' ', itemName.length() / 2);
        if (splitPos == -1) splitPos = itemName.length() / 2;
        M5Dial.Display.drawString(itemName.substring(0, splitPos), centerX, centerY - 30);
        M5Dial.Display.drawString(itemName.substring(splitPos), centerX, centerY - 10);
    } else {
        M5Dial.Display.drawString(itemName, centerX, centerY - 20);
    }
    
    // Amount
    M5Dial.Display.setTextColor(YELLOW);
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.drawString("x" + String(amount), centerX, centerY + 20);
    
    // Transfer status
    if (canTransfer) {
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString("PRESS TO SELECT", centerX, centerY + 65);
    } else {
        M5Dial.Display.setTextColor(RED);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString("CANNOT TRANSFER", centerX, centerY + 65);
        M5Dial.Display.drawString("(Scored Item)", centerX, centerY + 80);
    }
}

// Display waiting for target faction
void displayLootTransferWaitingForTarget(String itemName, int amount) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Title
    M5Dial.Display.setTextColor(YELLOW);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("TRANSFER", centerX, 15);
    
    // Item name
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString(itemName, centerX, 45);
    
    // Amount
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("x" + String(amount), centerX, 65);
    
    // Arrow animation
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("->", centerX, centerY + 10);
    
    // Instruction
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("SCAN TARGET", centerX, centerY + 55);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString("Faction Badge", centerX, centerY + 75);
}

// Display transfer in progress
void displayLootTransferInProgress(String itemName, String targetFaction) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Status
    M5Dial.Display.setTextColor(YELLOW);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("TRANSFERRING", centerX, centerY - 30);
    
    // Item
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString(itemName, centerX, centerY);
    
    // Target
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString("TO", centerX, centerY + 20);
    M5Dial.Display.drawString(targetFaction, centerX, centerY + 35);
}

// Display transfer result
void displayLootTransferResult(bool success, String itemName, String targetFaction) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    if (success) {
        // Success
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("SUCCESS", centerX, centerY - 35);
        
        // Checkmark
        M5Dial.Display.drawLine(centerX - 15, centerY, centerX - 5, centerY + 15, GREEN);
        M5Dial.Display.drawLine(centerX - 5, centerY + 15, centerX + 20, centerY - 15, GREEN);
        M5Dial.Display.drawLine(centerX - 15, centerY - 1, centerX - 5, centerY + 16, GREEN);
        M5Dial.Display.drawLine(centerX - 5, centerY + 16, centerX + 20, centerY - 14, GREEN);
        
        // Details
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString(itemName, centerX, centerY + 35);
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.drawString("Sent to " + targetFaction, centerX, centerY + 50);
    } else {
        // Failure
        M5Dial.Display.setTextColor(RED);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("FAILED", centerX, centerY - 35);
        
        // X mark
        M5Dial.Display.drawLine(centerX - 15, centerY - 15, centerX + 15, centerY + 15, RED);
        M5Dial.Display.drawLine(centerX + 15, centerY - 15, centerX - 15, centerY + 15, RED);
        M5Dial.Display.drawLine(centerX - 15, centerY - 14, centerX + 15, centerY + 16, RED);
        M5Dial.Display.drawLine(centerX + 15, centerY - 14, centerX - 15, centerY + 16, RED);
        
        // Error message
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString("Transfer Failed", centerX, centerY + 35);
    }
}

// Display amount selector for loot transfer
void displayLootTransferAmountSelector(String itemName, int currentAmount, int maxAmount, String factionLabel) {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    
    // Title
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString(factionLabel, centerX, 20);
    
    // Item name
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(itemName, centerX, 45);
    
    // Amount selector
    M5Dial.Display.setTextSize(4);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.drawString(String(currentAmount), centerX, centerY);
    
    // Max amount
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString("of " + String(maxAmount), centerX, centerY + 35);
    
    // Instructions
    M5Dial.Display.setTextColor(DARKGREY);
    M5Dial.Display.drawString("Rotate to change", centerX, M5Dial.Display.height() - 40);
    M5Dial.Display.drawString("Click to confirm", centerX, M5Dial.Display.height() - 25);
    
    drawWiFiIndicator();
}

// Display checkout confirmation screen
void displayLootTransferCheckout() {
    M5Dial.Display.fillScreen(BLACK);
    
    int centerX = M5Dial.Display.width() / 2;
    int y = 15;
    
    // Title
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("CHECKOUT", centerX, y);
    y += 25;
    
    // Faction A section
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);  // Gray
    M5Dial.Display.drawString("SENDING:", centerX, y);
    y += 12;
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(ltFactionA_BadgeName, centerX, y);
    y += 15;
    
    if (ltFactionA_SelectedIndex >= 0 && ltFactionA_SelectedAmount > 0) {
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString(ltFactionA_Items[ltFactionA_SelectedIndex].itemName, centerX, y);
        y += 12;
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.drawString("x" + String(ltFactionA_SelectedAmount), centerX, y);
    } else {
        M5Dial.Display.setTextColor(DARKGREY);
        M5Dial.Display.drawString("(nothing)", centerX, y);
        y += 12;
    }
    y += 18;
    
    // Arrow
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.drawString("<  >", centerX, y);
    y += 18;
    
    // Faction B section
    M5Dial.Display.setTextColor(0x7BEF);  // Gray
    M5Dial.Display.drawString("SENDING:", centerX, y);
    y += 12;
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString(ltFactionB_BadgeName, centerX, y);
    y += 15;
    
    if (ltFactionB_SelectedIndex >= 0 && ltFactionB_SelectedAmount > 0) {
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.drawString(ltFactionB_Items[ltFactionB_SelectedIndex].itemName, centerX, y);
        y += 12;
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.drawString("x" + String(ltFactionB_SelectedAmount), centerX, y);
    } else {
        M5Dial.Display.setTextColor(DARKGREY);
        M5Dial.Display.drawString("(nothing)", centerX, y);
        y += 12;
    }
    y += 25;
    
    // Instructions
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("Click to confirm", centerX, y);
    
    drawWiFiIndicator();
}

// Display variable viewer - scrollable list of all device variables
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
    extern SecureBuyStationClient SecureWs;
    extern unsigned long bs2LastActivityTime;
    
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(WHITE, BLACK);
    M5Dial.Display.setTextDatum(top_left);
    
    // Title
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR, BLACK);
    M5Dial.Display.drawString("Variables", 10, 5);
    
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE, BLACK);
    
    // Build variable list
    String varNames[64];
    String varValues[64];
    int varCount = 0;
    
    // Device Info
    varNames[varCount] = "Serial"; varValues[varCount] = serialNum; varCount++;
    varNames[varCount] = "MAC"; varValues[varCount] = macAddress.substring(0, 17); varCount++;
    varNames[varCount] = "GameId"; varValues[varCount] = gameId.substring(0, 12) + "..."; varCount++;
    varNames[varCount] = "SecureWS"; varValues[varCount] = SecureWs.ready() ? "AUTH OK" : "PENDING"; varCount++;
    if (SecureWs.widgetId().length()) {
        varNames[varCount] = "SecureWidget"; varValues[varCount] = SecureWs.widgetId().substring(0, 10); varCount++;
    }
    if (SecureWs.gameId().length()) {
        varNames[varCount] = "SecureGame"; varValues[varCount] = SecureWs.gameId().substring(0, 12) + "..."; varCount++;
    }
    varNames[varCount] = "SecureModes"; varValues[varCount] = SecureWs.capabilitySummary(); varCount++;
    
    // Mode
    String modeStr = "Unknown";
    if (deviceMode == MODE_BUY_STATION) modeStr = "BuyStation";
    else if (deviceMode == MODE_BUY_STATION_2) modeStr = "BuyStation2";
    else if (deviceMode == MODE_BACKPACK) modeStr = "Backpack";
    else if (deviceMode == MODE_LOOT_TRANSFER) modeStr = "LootTransfer";
    else if (deviceMode == MODE_RELAY) modeStr = "Relay";
    else if (deviceMode == MODE_PHOTOBOOTH) modeStr = "Photobooth";
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

    // Game Faction Cache
    varNames[varCount] = "FactionCache"; varValues[varCount] = String(gameFactionCount) + " " + gameFactionCacheStatus; varCount++;
    if (gameFactionCacheWidgetId.length()) {
        varNames[varCount] = "Widget"; varValues[varCount] = gameFactionCacheWidgetId.substring(0, 10); varCount++;
    }
    for (int i = 0; i < gameFactionCount && varCount < 62; i++) {
        varNames[varCount] = "Faction " + String(i + 1);
        varValues[varCount] = gameFactionCache[i].name;
        if (gameFactionCache[i].colorCode.length()) {
            varValues[varCount] += " " + gameFactionCache[i].colorCode;
        }
        varCount++;

        if (varCount < 62) {
            varNames[varCount] = "FactionId";
            varValues[varCount] = gameFactionCache[i].id.substring(0, 12);
            varCount++;
        }
    }
    if (gameFactionCount == 0 && varCount < 62) {
        varNames[varCount] = "FactionFallback"; varValues[varCount] = "legacy names"; varCount++;
        for (int i = 0; i < FACTION_FALLBACK_COUNT && varCount < 62; i++) {
            varNames[varCount] = "Legacy " + String(i + 1);
            varValues[varCount] = String(FACTION_FALLBACK_NAMES[i]);
            varCount++;
        }
    }
    
    // Relay-style scan posting variables
    varNames[varCount] = "Post:UUID"; varValues[varCount] = relayLastUuid.length() > 0 ? relayLastUuid.substring(0, 10) : "(none)"; varCount++;
    varNames[varCount] = "Post:Success"; varValues[varCount] = relayLastSuccess ? "YES" : "NO"; varCount++;
    
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

#endif // M5DIALCONTROL_H
