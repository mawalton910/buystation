#ifndef BACKPACKMODEDISPLAY_H
#define BACKPACKMODEDISPLAY_H

#include <M5Dial.h>
#include "config.h"

int backpackTextWidthForSize(const String& text, int textSize) {
    M5Dial.Display.setTextSize(textSize);
    return (int)M5Dial.Display.textWidth(text.length() > 0 ? text : " ");
}

String backpackEllipsizeToWidth(String text, int textSize, int maxWidth) {
    if (backpackTextWidthForSize(text, textSize) <= maxWidth) return text;
    while (text.length() > 0) {
        String candidate = text + "...";
        if (backpackTextWidthForSize(candidate, textSize) <= maxWidth) {
            return candidate;
        }
        text.remove(text.length() - 1);
    }
    return "...";
}

void drawBackpackAdaptiveName(const String& name, int centerX, int centerY, int maxWidth) {
    int nameSize = 2;
    if (backpackTextWidthForSize(name, nameSize) > maxWidth) {
        nameSize = 1;
    }
    String displayName = backpackEllipsizeToWidth(name, nameSize, maxWidth);
    M5Dial.Display.setTextSize(nameSize);
    M5Dial.Display.drawString(displayName, centerX, centerY);
}

void drawBackpackWrappedItemName(const String& itemName, int centerX, int centerY, int maxWidth) {
    M5Dial.Display.setTextSize(1);
    if (backpackTextWidthForSize(itemName, 1) <= maxWidth) {
        M5Dial.Display.drawString(itemName, centerX, centerY);
        return;
    }

    int splitPos = itemName.length() / 2;
    int bestSplit = splitPos;
    int bestScore = itemName.length();
    for (int i = 1; i < itemName.length() - 1; i++) {
        if (itemName.charAt(i) != ' ') continue;
        String left = itemName.substring(0, i);
        String right = itemName.substring(i + 1);
        left.trim();
        right.trim();
        int wLeft = backpackTextWidthForSize(left, 1);
        int wRight = backpackTextWidthForSize(right, 1);
        int score = abs(wLeft - wRight);
        if (wLeft <= maxWidth && wRight <= maxWidth && score < bestScore) {
            bestScore = score;
            bestSplit = i;
        }
    }

    String line1 = itemName.substring(0, bestSplit);
    String line2 = itemName.substring(bestSplit + 1);
    line1.trim();
    line2.trim();

    if (line1.length() == 0 || line2.length() == 0) {
        String single = backpackEllipsizeToWidth(itemName, 1, maxWidth);
        M5Dial.Display.drawString(single, centerX, centerY);
        return;
    }

    line1 = backpackEllipsizeToWidth(line1, 1, maxWidth);
    line2 = backpackEllipsizeToWidth(line2, 1, maxWidth);
    M5Dial.Display.drawString(line1, centerX, centerY - 7);
    M5Dial.Display.drawString(line2, centerX, centerY + 7);
}

// Show READY screen after badge scan in Backpack mode
void displayBackpackReady(const String& badgeName, int lootLimit) {
    M5Dial.Display.fillScreen(BLACK);
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    // READY
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("READY", centerX, centerY - 30);
    // Badge name
    M5Dial.Display.setTextColor(WHITE);
    drawBackpackAdaptiveName(badgeName, centerX, centerY, M5Dial.Display.width() - 20);
    // Prompt
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(3);
    M5Dial.Display.drawString("SCAN LOOT", centerX, centerY + 40);
    // Slots
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(0x7BEF);
    M5Dial.Display.drawString("Slots: 0/" + String(lootLimit), centerX, M5Dial.Display.height() - 20);
}

void displayLootAlreadyClaimedThisRound(const String& itemName, const String& roundTimeLeft) {
    M5Dial.Display.fillScreen(BLACK);
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;

    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.fillCircle(centerX, 38, 18, RED);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("!", centerX, 38);

    M5Dial.Display.setTextColor(RED);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("ALREADY", centerX, centerY - 48);
    M5Dial.Display.drawString("CLAIMED", centerX, centerY - 24);

    M5Dial.Display.setTextColor(WHITE);
    drawBackpackWrappedItemName(itemName.length() ? itemName : "This loot", centerX, centerY + 10, M5Dial.Display.width() - 22);

    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("THIS ROUND", centerX, centerY + 48);

    if (roundTimeLeft.length() > 0) {
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.drawString(roundTimeLeft, centerX, centerY + 66);
    }
}
// BackpackModeDisplay.h - Custom display functions for Backpack Mode

// Display scan screen for Backpack Mode (copy of displayBuyStation2Scan with wording changes)
void displayBackpackScan() {
    M5Dial.Display.fillScreen(BLACK);
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    // Title
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("GURU GAMES", centerX, 30);
    // Subtitle
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.drawString("CLAIM Station", centerX, 55);
    // Icon - RFID card with person (same as Buy Station 2)
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

// Animated lock helper retained for manual/admin lock workflows.
void displayBackpackUnlocking(const String& itemName, int frame) {
    M5Dial.Display.fillScreen(BLACK);
    int centerX = M5Dial.Display.width() / 2;
    int centerY = M5Dial.Display.height() / 2;
    // Title
    M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(top_center);
    M5Dial.Display.drawString("UNLOCKING", centerX, 20);
    // Item name
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);
    drawBackpackWrappedItemName(itemName, centerX, centerY - 40, M5Dial.Display.width() - 20);
    // Draw lock body
    int lockY = centerY + 10;
    M5Dial.Display.fillRect(centerX - 20, lockY, 40, 30, 0x7BEF); // blue lock body
    M5Dial.Display.drawRect(centerX - 20, lockY, 40, 30, WHITE);
    // Draw shackle (animated)
    int shackleY = lockY - 25;
    int shackleOpen = frame * 8; // frame 0 = closed, frame 3 = open
    int shackleStart = centerX - 12 + shackleOpen;
    int shackleEnd = centerX + 12 - shackleOpen;
    M5Dial.Display.drawArc(centerX, shackleY + 20, 12, 8, 180, 360, WHITE);
    if (frame < 3) {
        // Draw closed shackle
        M5Dial.Display.drawLine(shackleStart, shackleY + 20, shackleStart, shackleY + 8, WHITE);
        M5Dial.Display.drawLine(shackleEnd, shackleY + 20, shackleEnd, shackleY + 8, WHITE);
    } else {
        // Draw open shackle (rotated)
        M5Dial.Display.drawLine(centerX - 12, shackleY + 20, centerX - 20, shackleY + 5, WHITE);
        M5Dial.Display.drawLine(centerX + 12, shackleY + 20, centerX + 20, shackleY + 5, WHITE);
    }
    // Progress dots
    for (int i = 0; i < 4; i++) {
        int dotColor = (i <= frame) ? GREEN : 0x39C7;
        M5Dial.Display.fillCircle(centerX - 18 + i * 12, lockY + 40, 4, dotColor);
    }
}

#endif // BACKPACKMODEDISPLAY_H
