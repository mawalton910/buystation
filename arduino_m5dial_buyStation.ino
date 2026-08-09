// relay2_M5.ino - M5 Dial Version
#include <WiFi.h>
#include <Arduino.h>
#include <esp_now.h>
#include <string>
#include <M5Dial.h>

#if __has_include(<M5Dial.h>)
#include <M5Dial.h>
#endif
#include "config.h"
#include "BadgeUtils.h"
#include "ErrorHandler.h"
#include "WiFiManager.h"
#include "ApiClient.h"
#include "SecureBuyStationClient.h"
#include "M5DialControl.h"
#include "BuyStation2.h"
#include "BackpackModeDisplay.h"
#include "BackpackMode.h"
#include "LootTransferMode.h"
#include "AdminLootLock.h"
#include "AdminWiFi.h"
#include "AdminDeviceInfo.h"
#include "initSequence_M5.h"
#include "MapMode.h"
#include "OTAUpdate.h"
#include "HttpUtils.h"

// State machine for scanning workflow
enum ScanState {
    WAIT_FOR_PLAYER,
    WAIT_FOR_LOOT,
    CONFIRM_TRANSACTION,
    ADMIN_MODE,
    ADMIN_WIFI_STATUS,
    ADMIN_LOOT_FLIP,
    ADMIN_WIFI_SCAN,
    ADMIN_WIFI_PASSWORD
};

ScanState currentState = WAIT_FOR_PLAYER;
String playerUuid = "";
String lootUuid = "";
String playerName = "";
String itemName = "";
String playerFaction = "";
String itemType = "";
unsigned long lastCardReadTime = 0;
unsigned long lastActivityTime = 0;
unsigned long confirmStateEnteredMs = 0;
bool bs2FinalConfirmPending = false;

// Admin mode variables
int adminMenuSelection = 0;
long encoderPosition = 0;
long lastEncoderChange = 0;
unsigned long lastButtonPress = 0;
const unsigned long BUTTON_DEBOUNCE_MS = 300;  // 300ms button debounce
long adminEncoderStep = 0;
const int ADMIN_ENCODER_STEP = 4;  // Encoder ticks per detent
String relayLastUuid = "";
String relayLastResponse = "";
bool relayLastSuccess = false;
String relayLastSentUuid = "";
String relayLastSentSource = "";
unsigned long relayLastSentAtMs = 0;
unsigned long relayDisplayClearAtMs = 0;
OperationalMode deviceMode = MODE_BACKPACK;  // Default to Backpack mode
OperationalMode previousDeviceMode = MODE_BACKPACK; // To store mode before entering admin
String lootFlipLastUuid = "";
String lootFlipLastStatus = "";

// Shared HTTPS client for keep-alive
WiFiClientSecure sharedHttpsClient;
SecureBuyStationClient SecureWs;

bool waitForSecureWsAuth(uint32_t timeoutMs) {
    if (WiFi.status() != WL_CONNECTED) return false;
    SecureWs.begin();
    const uint32_t startedAt = millis();
    while (!SecureWs.ready() && static_cast<uint32_t>(millis() - startedAt) < timeoutMs) {
        SecureWs.loop();
        delay(10);
    }
    return SecureWs.ready();
}

bool isRelayPostingMode() {
    return deviceMode == MODE_RELAY || deviceMode == MODE_PHOTOBOOTH;
}

void displayRelayPostingMode(String lastUuid, String lastResponse, bool success) {
    if (deviceMode == MODE_PHOTOBOOTH) {
        displayPhotoboothMode(lastUuid, lastResponse, success);
    } else {
        displayRelayMode(lastUuid, lastResponse, success);
    }
}

void displayRelayPostingMode() {
    displayRelayPostingMode("", "", false);
}

void displayActiveOperationalMode() {
    if (deviceMode == MODE_BUY_STATION || deviceMode == MODE_BUY_STATION_2) displayBuyStation2Scan();
    else if (deviceMode == MODE_BACKPACK) displayBackpackScan();
    else if (deviceMode == MODE_LOOT_TRANSFER) displayLootTransferScan();
    else if (deviceMode == MODE_MAP) displayMapModeStatus("Ready", GREEN);
    else if (isRelayPostingMode()) displayRelayPostingMode();
    else displayBuyStation2Scan();
}

void scheduleRelayDisplayClear() {
    relayDisplayClearAtMs = millis() + 1500;
}

void clearRelayDisplayIfReady() {
    if (!relayDisplayClearAtMs || !isRelayPostingMode()) return;
    if (static_cast<long>(millis() - relayDisplayClearAtMs) < 0) return;
    relayDisplayClearAtMs = 0;
    relayLastUuid = "";
    relayLastResponse = "";
    relayLastSuccess = false;
    displayRelayPostingMode();
}

void handleRelayPostingScan(const String& scannedUuid, const String& sourceLabel) {
    recordUserActivity();
    unsigned long now = millis();
    if (relayLastSentUuid == scannedUuid &&
        relayLastSentSource == sourceLabel &&
        static_cast<unsigned long>(now - relayLastSentAtMs) < RELAY_DUPLICATE_SUPPRESS_MS) {
        Serial.println("Relay duplicate suppressed for " + sourceLabel + ": " + scannedUuid);
        return;
    }

    relayLastUuid = scannedUuid;
    String modeLabel = deviceMode == MODE_PHOTOBOOTH ? "PHOTOBOOTH" : "RELAY";
    Serial.println(modeLabel + " mode " + sourceLabel + " scan: " + scannedUuid);

    int httpCode = sendRelayUpdate(scannedUuid, modeLabel);
    relayLastSuccess = (httpCode == 200 || httpCode == 201);
    relayLastResponse = sourceLabel.length() ? sourceLabel + " " + String(httpCode) : String(httpCode);

    displayRelayPostingMode(relayLastUuid, relayLastResponse, relayLastSuccess);

    if (relayLastSuccess) {
        relayLastSentUuid = scannedUuid;
        relayLastSentSource = sourceLabel;
        relayLastSentAtMs = now;
        relayLastUuid = "";
        scheduleRelayDisplayClear();
    }
}

struct EspNowScanPacket {
    uint8_t type;
    uint32_t sequence;
    char uid[32];
    char senderMac[18];
    char targetLabel[16];
    char effect[16];
};

struct EspNowModeSyncPacket {
    uint8_t type;
    uint32_t sequence;
    char routeMode[24];
    char receiverLabel[16];
};

struct EspNowHelloPacket {
    uint8_t type;
    uint32_t sequence;
    char senderMac[18];
    char senderLabel[16];
    char routeMode[24];
};

volatile bool espNowRelayPending = false;
String espNowRelayUuid = "";
String espNowRelayResponse = "";
bool espNowRelaySuccess = false;
uint32_t espNowModeSequence = 0;
String learnedSenderMac = "";
bool learnedSenderReady = false;

#if __has_include(<M5Dial.h>)
MFRC522 rfid2PortA(RC522_I2C_ADDRESS, &M5.Ex_I2C);
bool rfid2PortAAvailable = false;
#endif

bool hasNativeRfidReader() {
    return true;
}

bool hasPortARfid2Reader() {
#if __has_include(<M5Dial.h>)
    return true;
#else
    return false;
#endif
}

String readPortARfid2Uid() {
#if __has_include(<M5Dial.h>)
    if (!rfid2PortAAvailable) {
        return "";
    }

    uint8_t bufferATQA[2] = {0, 0};
    uint8_t bufferSize = sizeof(bufferATQA);
    uint8_t status = rfid2PortA.PICC_RequestA(bufferATQA, &bufferSize);
    if (status != MFRC522::STATUS_OK && status != MFRC522::STATUS_COLLISION) {
        return "";
    }

    if (rfid2PortA.PICC_ReadCardSerial()) {
        Serial.println("[RFID2] card read from Port A");
        String uidString = "";
        for (byte i = 0; i < rfid2PortA.uid.size; i++) {
            if (rfid2PortA.uid.uidByte[i] < 0x10) {
                uidString += "0";
            }
            uidString += String(rfid2PortA.uid.uidByte[i], HEX);
            if (i != rfid2PortA.uid.size - 1) {
                uidString += " ";
            }
        }
        uidString.toUpperCase();
        rfid2PortA.PICC_HaltA();
        rfid2PortA.PCD_StopCrypto1();
        return sanitizeUuid(uidString);
    }
#endif
    return "";
}

void handlePortARfid2Scan() {
#if __has_include(<M5Dial.h>)
    if (!rfid2PortAAvailable) {
        return;
    }

    if (millis() - lastCardReadTime <= CARD_READ_DEBOUNCE) {
        return;
    }

    String scannedUuid = readPortARfid2Uid();
    if (scannedUuid.length() == 0) {
        return;
    }

    lastCardReadTime = millis();
    Serial.println("[RFID2] Port A badge detected: " + scannedUuid);

    if (isRelayPostingMode()) {
        if (isAdminBadge(scannedUuid)) {
            Serial.println("[RFID2] Admin badge detected!");
            displayAdminAccessGranted();
            previousDeviceMode = deviceMode;
            currentState = ADMIN_MODE;
            adminMenuSelection = 0;
            encoderPosition = M5Dial.Encoder.read();
            adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
            displayAdminMenu(adminMenuSelection, deviceMode);
            lastActivityTime = millis();
            return;
        }
        handleRelayPostingScan(scannedUuid, "RFID2");
    } else if (deviceMode == MODE_BUY_STATION_2 && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
        processBuyStation2BadgeScan(scannedUuid, false);
    } else if (deviceMode == MODE_BACKPACK && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
        processBackpackBadgeScan(scannedUuid, false);
    } else {
        Serial.printf("[RFID2] Scan received while in mode %d: %s\n", (int)deviceMode, scannedUuid.c_str());
    }
#endif
}

bool parseMacAddressString(const String& value, uint8_t outMac[6]) {
    int parsed[6] = {0, 0, 0, 0, 0, 0};
    if (sscanf(value.c_str(), "%x:%x:%x:%x:%x:%x",
               &parsed[0], &parsed[1], &parsed[2], &parsed[3], &parsed[4], &parsed[5]) != 6) {
        return false;
    }
    for (int index = 0; index < 6; index++) outMac[index] = (uint8_t)parsed[index];
    return true;
}

String currentModeRouteLabel() {
    switch (deviceMode) {
        case MODE_BUY_STATION:
        case MODE_BUY_STATION_2: return "BUY";
        case MODE_BACKPACK: return "BACKPACK";
        case MODE_LOOT_TRANSFER: return "TRANSFER";
        case MODE_RELAY: return "RELAY";
        case MODE_PHOTOBOOTH: return "PHOTO";
        case MODE_MAP: return "MAP";
        default: return "BUY";
    }
}

String macBytesToString(const uint8_t* macAddr) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    return String(buffer);
}

void broadcastCurrentModeToSender() {
    String peerMacString = learnedSenderReady ? learnedSenderMac : espnow_peer_mac;
    if (!learnedSenderReady && peerMacString == "FF:FF:FF:FF:FF:FF") {
        Serial.println("[ESPNOW] waiting for sender hello before syncing mode");
        return;
    }
    uint8_t peerMac[6];
    if (!parseMacAddressString(peerMacString, peerMac)) {
        Serial.printf("[ESPNOW] invalid peer mac: %s\n", peerMacString.c_str());
        return;
    }

    if (esp_now_is_peer_exist(peerMac) == false) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, peerMac, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
#if defined(WIFI_IF_STA)
        peerInfo.ifidx = WIFI_IF_STA;
#endif
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("[ESPNOW] mode sync peer add failed");
            return;
        }
    }

    EspNowModeSyncPacket packet = {};
    packet.type = 2;
    packet.sequence = ++espNowModeSequence;
    strlcpy(packet.routeMode, currentModeRouteLabel().c_str(), sizeof(packet.routeMode));
    strlcpy(packet.receiverLabel, espnow_mac.c_str(), sizeof(packet.receiverLabel));

    esp_err_t result = esp_now_send(peerMac, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
    if (result == ESP_OK) {
        Serial.printf("[ESPNOW] mode sync sent route=%s label=%s seq=%lu\n",
                      packet.routeMode, packet.receiverLabel, (unsigned long)packet.sequence);
    } else {
        Serial.printf("[ESPNOW] mode sync failed: %d\n", (int)result);
    }
}

void onEspNowRecv(const esp_now_recv_info* recvInfo, const uint8_t* incomingData, int len) {
    const uint8_t* macAddr = recvInfo->src_addr;
    if (len == (int)sizeof(EspNowHelloPacket)) {
        EspNowHelloPacket packet = {};
        memcpy(&packet, incomingData, sizeof(packet));
        if (packet.type != 3) return;

        learnedSenderMac = String(packet.senderMac);
        learnedSenderMac.trim();
        if (learnedSenderMac.length() == 0) {
            learnedSenderMac = macBytesToString(macAddr);
        }
        learnedSenderReady = true;
        Serial.printf("[ESPNOW] Hello from sender=%s label=%s route=%s\n",
                      learnedSenderMac.c_str(), packet.senderLabel, packet.routeMode);
        broadcastCurrentModeToSender();
        return;
    }

    if (len != (int)sizeof(EspNowScanPacket)) {
        Serial.printf("[ESPNOW] Ignored packet len=%d\n", len);
        return;
    }

    EspNowScanPacket packet = {};
    memcpy(&packet, incomingData, sizeof(packet));

    String target = String(packet.targetLabel);
    target.trim();
    if (target.length() > 0 && target != espnow_mac) {
        Serial.printf("[ESPNOW] Ignored target=%s expected=%s\n", target.c_str(), espnow_mac.c_str());
        return;
    }

    espNowRelayUuid = String(packet.uid);
    espNowRelayResponse = String("ESPNOW ") + packet.senderMac;
    espNowRelaySuccess = true;
    espNowRelayPending = true;

    Serial.printf("[ESPNOW] Rx uid=%s from=%02X:%02X:%02X:%02X:%02X:%02X target=%s seq=%lu\n",
                  packet.uid,
                  macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5],
                  packet.targetLabel,
                  (unsigned long)packet.sequence);
}

void initializePortARfid2() {
#if __has_include(<M5Dial.h>)
    Serial.println("Initializing Port A RFID2 reader...");
    M5.Ex_I2C.begin();
    delay(250);
    rfid2PortA.PCD_Init();
    delay(250);
    uint8_t version = rfid2PortA.PCD_ReadRegister(MFRC522::VersionReg);
    Serial.printf("[RFID2] Port A bus SDA=%d SCL=%d VersionReg=0x%02X\n", M5.Ex_I2C.getSDA(), M5.Ex_I2C.getSCL(), version);
    rfid2PortAAvailable = (version != 0x00 && version != 0xFF);
    if (rfid2PortAAvailable) {
        Serial.println("Port A RFID2 ready");
    } else {
        Serial.println("Port A RFID2 did not respond; check wiring and power");
    }
#else
    Serial.println("Port A RFID2 support disabled: M5Dial RFID support not available");
#endif
}

bool initEspNowReceiver() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESPNOW] receiver init failed");
        return false;
    }
    esp_now_register_recv_cb(onEspNowRecv);
    Serial.println("[ESPNOW] receiver ready");
    return true;
}

void processBuyStation2BadgeScan(const String& scannedUuid, bool fromEspNow) {
    recordUserActivity();

    String normalizedUuid = sanitizeUuid(scannedUuid);
    if (isAdminBadge(normalizedUuid)) {
        Serial.println(fromEspNow ? "[ESPNOW] Admin badge detected" : "Admin badge detected!");
        displayAdminAccessGranted();
        previousDeviceMode = deviceMode;
        currentState = ADMIN_MODE;
        adminMenuSelection = 0;
        encoderPosition = M5Dial.Encoder.read();
        adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
        displayAdminMenu(adminMenuSelection, deviceMode);
        lastActivityTime = millis();
        lastCardReadTime = millis();
        resetBuyStation2State();
        bs2WaitingForConfirm = false;
        return;
    }

    displayCenteredText(fromEspNow ? "ESPNOW CHECKING..." : "CHECKING...", THEME_ACCENT_COLOR, BLACK, 2);
    Serial.println(fromEspNow ? "=== Buy Station: Checking ESP-NOW Badge Type ===" : "=== Buy Station: Checking Badge Type ===");
    Serial.println("UUID: " + normalizedUuid);

    bs2LastActivityTime = millis();
    String badgeType = checkWhatIsIt(normalizedUuid);

    Serial.println("Badge Type: " + badgeType);
    Serial.println("==========================================");

    if (badgeType == "error") {
        Serial.println("HTTP error during badge scan");
        displayCenteredText("NETWORK ERROR  Retry", RED, BLACK, 2);
        delay(2000);
        displayBuyStation2Scan();
        return;
    }

    if (badgeType == "badge") {
        if (!bs2DevMode && bs2LastUsedBadge.length() > 0 && bs2LastUsedBadge == scannedUuid) {
            Serial.println("Badge cooldown active - same badge used back-to-back: " + scannedUuid);
            Serial.println("Another player must complete a transaction first");
            displayCenteredText("WAIT FOR NEXT PLAYER", ORANGE, BLACK, 2);
            delay(2000);
            displayBuyStation2Scan();
            return;
        }

        bs2LootCount = 0;
        bs2WaitingForConfirm = false;
        bs2BrowsingIndex = 0;
        Serial.println("Buy Station: Player badge detected: " + scannedUuid);
        Serial.println("Name: " + bs2BadgeName);
        Serial.println("Loot limit: " + String(bs2LootLimit));
        Serial.println("Player Class: " + (bs2PlayerClass.length() > 0 ? bs2PlayerClass : "None"));
        Serial.println("Loot Multiplier: " + String(bs2ClassMultiplier) + "x");

        M5Dial.Display.fillScreen(BLACK);
        M5Dial.Display.setTextColor(GREEN);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("READY", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(2);
        M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
        M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.setTextColor(0x7BEF);
        M5Dial.Display.drawString("Slots: 0/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        return;
    }

    if (badgeType == "loot") {
        if (bs2LootLimit < 1) bs2LootLimit = 1;
        if (bs2BadgeUuid.length() > 0) {
            Serial.println("Buy Station: Loot item detected: " + scannedUuid);

            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(GREEN);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString("LOOT ADDED", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.drawString(bs2LootItems[bs2LootCount - 1].lootName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString(String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);

            unsigned long startTime = millis();
            while (millis() - startTime < 800) {
                M5Dial.update();
                if (M5Dial.BtnA.wasPressed()) break;
                delay(50);
            }

            Serial.println("Entering browse mode with " + String(bs2LootCount) + " item(s)");
            bs2WaitingForConfirm = true;
            bs2BrowsingIndex = bs2LootCount - 1;
            encoderPosition = M5Dial.Encoder.read();
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
            lastCardReadTime = millis();
            return;
        }

        if (bs2BadgeUuid.length() == 0) {
            displayCenteredText("SCAN BADGE FIRST", ORANGE, BLACK, 2);
            unsigned long startTime = millis();
            while (millis() - startTime < 1500) {
                M5Dial.update();
                if (M5Dial.BtnA.wasPressed()) break;
                delay(50);
            }
            displayBuyStation2Scan();
            return;
        }

        playErrorTone();
        Serial.println("[Limit] bs2LootCount=" + String(bs2LootCount) + " bs2LootLimit=" + String(bs2LootLimit));
        if (bs2LootLimit == 1) {
            displayCenteredText("LOOT LIMIT REACHED  No Backpack", ORANGE, BLACK, 2);
        } else {
            displayCenteredText("LOOT LIMIT REACHED", RED, BLACK, 2);
        }

        unsigned long startTime = millis();
        bool buttonPressed = false;
        while (millis() - startTime < 3000 && !buttonPressed) {
            M5Dial.update();
            if (M5Dial.BtnA.wasPressed()) buttonPressed = true;
            delay(50);
        }

        if (bs2WaitingForConfirm && bs2LootCount > 0) {
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        } else if (bs2BadgeUuid.length() == 0) {
            displayBuyStation2Scan();
        } else {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Slots: " + String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        }
        lastCardReadTime = millis();
        return;
    }

    if (badgeType == "duplicate") {
        displayCenteredText("ALREADY IN CART", ORANGE, BLACK, 2);
        delay(1500);
        if (bs2WaitingForConfirm && bs2LootCount > 0) {
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        } else {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Slots: " + String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        }
        return;
    }

    if (badgeType == "removed") {
        playBuzzTone();
        displayCenteredText("REMOVED FROM CART", ORANGE, BLACK, 2);
        delay(1200);
        if (bs2LootCount > 0) {
            bs2WaitingForConfirm = true;
            if (bs2BrowsingIndex >= bs2LootCount) bs2BrowsingIndex = bs2LootCount - 1;
            if (bs2BrowsingIndex < 0) bs2BrowsingIndex = 0;
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        } else if (bs2BadgeUuid.length() > 0) {
            bs2WaitingForConfirm = false;
            bs2BrowsingIndex = 0;
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Slots: 0/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        } else {
            displayBuyStation2Scan();
        }
        lastCardReadTime = millis();
        return;
    }

    if (badgeType == "locked") {
        int centerX = M5Dial.Display.width() / 2;
        int centerY = M5Dial.Display.height() / 2;
        playBuzzTone();
        M5Dial.Display.fillScreen(BLACK);
        M5Dial.Display.fillCircle(centerX, centerY - 35, 12, RED);
        M5Dial.Display.fillRect(centerX - 15, centerY - 25, 30, 20, RED);
        M5Dial.Display.fillRect(centerX - 10, centerY - 15, 20, 10, BLACK);
        M5Dial.Display.fillCircle(centerX, centerY - 28, 3, BLACK);
        M5Dial.Display.setTextColor(RED);
        M5Dial.Display.setTextSize(3);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.drawString("LOCKED", centerX, centerY + 15);
        M5Dial.Display.setTextColor(WHITE);
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString("Item already claimed", centerX, centerY + 45);
        M5Dial.Display.drawString("Scan again to toggle", centerX, centerY + 65);
        delay(1500);
        if (bs2WaitingForConfirm && bs2LootCount > 0) {
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        } else if (bs2BadgeUuid.length() > 0) {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Slots: " + String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        } else {
            displayBuyStation2Scan();
        }
        lastCardReadTime = millis();
        return;
    }

    if (badgeType == "round_claimed") {
        playBuzzTone();
        displayLootAlreadyClaimedThisRound(bs2LastRejectedLootName, bs2LastRejectedRoundTimeLeft);
        delay(2200);
        if (bs2WaitingForConfirm && bs2LootCount > 0) {
            displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
        } else if (bs2BadgeUuid.length() > 0) {
            M5Dial.Display.fillScreen(BLACK);
            M5Dial.Display.setTextColor(WHITE);
            M5Dial.Display.setTextSize(2);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.drawString(bs2BadgeName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
            M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
            M5Dial.Display.setTextSize(3);
            M5Dial.Display.drawString("SCAN LOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.setTextColor(0x7BEF);
            M5Dial.Display.drawString("Slots: " + String(bs2LootCount) + "/" + String(bs2LootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() - 20);
        } else {
            displayBuyStation2Scan();
        }
        lastCardReadTime = millis();
        return;
    }

    Serial.println("Unhandled badge type: " + badgeType);
    displayBuyStation2Scan();
}

void processBackpackBadgeScan(const String& scannedUuid, bool fromEspNow) {
    recordUserActivity();

    String normalizedUuid = sanitizeUuid(scannedUuid);

    // Check for admin badge
    if (isAdminBadge(normalizedUuid)) {
        Serial.println(fromEspNow ? "[ESPNOW] Admin badge detected" : "Admin badge detected!");
        displayAdminAccessGranted();
        previousDeviceMode = deviceMode; // Save current mode
        currentState = ADMIN_MODE;
        adminMenuSelection = 0;
        encoderPosition = M5Dial.Encoder.read();
        adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
        displayAdminMenu(adminMenuSelection, deviceMode);
        lastActivityTime = millis();
        lastCardReadTime = millis();
        return;
    }

    // Use whatIsIt to determine badge type (reusing Buy Station 2 function)
    displayCenteredText(fromEspNow ? "ESPNOW CHECKING..." : "CHECKING...", THEME_ACCENT_COLOR, BLACK, 2);
    Serial.println(fromEspNow ? "=== Backpack Mode: Checking ESP-NOW Badge Type ===" : "=== Backpack Mode: Checking Badge Type ===");
    Serial.println("UUID: " + normalizedUuid);
    bpLastActivityTime = millis();
    String badgeType = checkWhatIsIt(normalizedUuid);
    if (badgeType == "badge") {
        bpBadgeUuid = bs2BadgeUuid;
        bpBadgeName = bs2BadgeName;
        bpBadgeFaction = bs2BadgeFaction;
        bpLootLimit = bs2LootLimit;
        Serial.println("=== Backpack Mode: Badge Registered ===");
        Serial.println("Scanned UUID: " + scannedUuid);
        Serial.println("Resolved badge UUID: '" + bpBadgeUuid + "'");
        Serial.println("Badge name: " + bpBadgeName);
        Serial.println("Loot limit: " + String(bpLootLimit));
        Serial.println("=======================================");
        // Clear bs2 variables
        bs2BadgeUuid = "";
        bs2BadgeName = "";
        bs2BadgeFaction = "";
        bs2LootCount = 0;
        bs2LootLimit = 1;
        for (int i = 0; i < MAX_LOOT_CART_ITEMS; i++) {
            bs2LootItems[i].lootId = "";
            bs2LootItems[i].lootUuid = "";
            bs2LootItems[i].lootName = "";
            bs2LootItems[i].rewardText = "";
            bs2LootItems[i].rewardsJson = "";
            bs2LootItems[i].locked = false;
            bs2LootItems[i].claimed = false;
        }
        // Show READY screen for Backpack mode
        displayBackpackReady(bpBadgeName, bpLootLimit);
    } else if (badgeType == "loot" && bpBadgeUuid.length() > 0) {
        // Toggle behavior in Backpack mode: scan once adds, scan again removes.
        int duplicateIndex = -1;
        for (int i = 0; i < bpLootCount; i++) {
            if (bpLootItems[i].lootUuid == scannedUuid) {
                duplicateIndex = i;
                break;
            }
        }

        if (duplicateIndex >= 0) {
            String removedName = bpLootItems[duplicateIndex].lootName;

            for (int i = duplicateIndex; i < bpLootCount - 1; i++) {
                bpLootItems[i] = bpLootItems[i + 1];
            }

            bpLootItems[bpLootCount - 1].lootId = "";
            bpLootItems[bpLootCount - 1].lootUuid = "";
            bpLootItems[bpLootCount - 1].lootName = "";
            bpLootItems[bpLootCount - 1].rewardText = "";
            bpLootItems[bpLootCount - 1].rewardsJson = "";
            bpLootItems[bpLootCount - 1].locked = false;
            bpLootItems[bpLootCount - 1].claimed = false;

            bpLootCount--;
            Serial.println("Backpack Mode: Removed from cart: " + removedName + " (" + scannedUuid + ")");

            displayCenteredText("REMOVED FROM CART", ORANGE, BLACK, 2);
            delay(1200);

            if (bpLootCount > 0) {
                bpWaitingForConfirm = true;
                if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = bpLootCount - 1;
                if (bpBrowsingIndex < 0) bpBrowsingIndex = 0;
                displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
            } else {
                bpWaitingForConfirm = false;
                bpBrowsingIndex = 0;
                displayBackpackReady(bpBadgeName, bpLootLimit);
            }
        } else if (bpLootCount >= bpLootLimit) {
            // Full cart: only allow duplicate-toggle removals, block new adds.
            displayCenteredText("LOOT LIMIT REACHED", ORANGE, BLACK, 2);
            delay(1500);
            if (bpWaitingForConfirm && bpLootCount > 0) {
                displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
            } else {
                displayBackpackReady(bpBadgeName, bpLootLimit);
            }
        } else {
            // Copy loot data from bs2 temp buffer to backpack cart.
            if (bs2LootCount > 0) {
                bpLootItems[bpLootCount] = bs2LootItems[bs2LootCount - 1];
                bpLootCount++;
                Serial.println("Backpack Mode: Loot item detected: " + scannedUuid);

                M5Dial.Display.fillScreen(BLACK);
                M5Dial.Display.setTextColor(GREEN);
                M5Dial.Display.setTextSize(2);
                M5Dial.Display.setTextDatum(middle_center);
                M5Dial.Display.drawString("LOOT ADDED", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 30);
                M5Dial.Display.setTextColor(WHITE);
                M5Dial.Display.setTextSize(1);
                M5Dial.Display.drawString(bpLootItems[bpLootCount - 1].lootName, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
                M5Dial.Display.setTextColor(THEME_ACCENT_COLOR);
                M5Dial.Display.setTextSize(3);
                M5Dial.Display.drawString(String(bpLootCount) + "/" + String(bpLootLimit), M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);
                delay(1500);

                bpWaitingForConfirm = true;
                bpBrowsingIndex = bpLootCount - 1;
                encoderPosition = M5Dial.Encoder.read();
                displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
            } else {
                Serial.println("Backpack Mode warning: loot type returned but bs2 temp buffer was empty");
            }
        }

        clearBs2TempLootBuffer();
    } else if (badgeType == "loot" && bpBadgeUuid.length() == 0) {
        displayCenteredText("SCAN BADGE FIRST", ORANGE, BLACK, 2);
        delay(1500);
        displayBackpackScan();
        clearBs2TempLootBuffer();
    } else if (badgeType == "limit") {
        displayCenteredText("LOOT LIMIT REACHED", ORANGE, BLACK, 2);
        delay(1500);
        if (bpWaitingForConfirm && bpLootCount > 0) {
            displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
        } else if (bpBadgeUuid.length() > 0) {
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }
        clearBs2TempLootBuffer();
    } else if (badgeType == "duplicate") {
        // Legacy status from checkWhatIsIt; treat as remove toggle for Backpack consistency.
        int duplicateIndex = -1;
        for (int i = 0; i < bpLootCount; i++) {
            if (bpLootItems[i].lootUuid == scannedUuid) {
                duplicateIndex = i;
                break;
            }
        }
        if (duplicateIndex >= 0) {
            for (int i = duplicateIndex; i < bpLootCount - 1; i++) bpLootItems[i] = bpLootItems[i + 1];
            bpLootItems[bpLootCount - 1].lootId = "";
            bpLootItems[bpLootCount - 1].lootUuid = "";
            bpLootItems[bpLootCount - 1].lootName = "";
            bpLootItems[bpLootCount - 1].rewardText = "";
            bpLootItems[bpLootCount - 1].rewardsJson = "";
            bpLootItems[bpLootCount - 1].locked = false;
            bpLootItems[bpLootCount - 1].claimed = false;
            bpLootCount--;
            displayCenteredText("REMOVED FROM CART", ORANGE, BLACK, 2);
            delay(1200);
        } else {
            displayCenteredText("ALREADY IN CART", ORANGE, BLACK, 2);
            delay(1500);
        }

        if (bpLootCount > 0) {
            bpWaitingForConfirm = true;
            if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = bpLootCount - 1;
            if (bpBrowsingIndex < 0) bpBrowsingIndex = 0;
            displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
        } else if (bpBadgeUuid.length() > 0) {
            bpWaitingForConfirm = false;
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }

        clearBs2TempLootBuffer();
    } else if (badgeType == "removed") {
        // checkWhatIsIt toggled a bs2-side duplicate out of cart; keep Backpack flow stable
        displayCenteredText("REMOVED FROM CART", ORANGE, BLACK, 2);
        delay(1200);
        if (bpWaitingForConfirm && bpLootCount > 0) {
            if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = bpLootCount - 1;
            if (bpBrowsingIndex < 0) bpBrowsingIndex = 0;
            displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
        } else if (bpBadgeUuid.length() > 0) {
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }
        clearBs2TempLootBuffer();
    } else if (badgeType == "locked") {
        displayCenteredText("ALREADY CLAIMED", RED, BLACK, 2);
        delay(3000);
        if (bpBadgeUuid.length() > 0) {
            bpWaitingForConfirm = false;
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }
        clearBs2TempLootBuffer();
    } else if (badgeType == "round_claimed") {
        displayLootAlreadyClaimedThisRound(bs2LastRejectedLootName, bs2LastRejectedRoundTimeLeft);
        delay(2200);
        if (bpWaitingForConfirm && bpLootCount > 0) {
            if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = bpLootCount - 1;
            if (bpBrowsingIndex < 0) bpBrowsingIndex = 0;
            displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
        } else if (bpBadgeUuid.length() > 0) {
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }
        clearBs2TempLootBuffer();
    } else if (badgeType == "error") {
        displayCenteredText("NETWORK ERROR", RED, BLACK, 2);
        delay(2000);
        if (bpBadgeUuid.length() == 0) {
            displayBackpackScan();
        }
    } else if (badgeType == "none" || badgeType == "unknown") {
        displayCenteredText("NOT FOUND", ORANGE, BLACK, 2);
        delay(1000);
        if (bpWaitingForConfirm && bpLootCount > 0) {
            if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = bpLootCount - 1;
            if (bpBrowsingIndex < 0) bpBrowsingIndex = 0;
            displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
        } else if (bpBadgeUuid.length() > 0) {
            displayBackpackReady(bpBadgeName, bpLootLimit);
        } else {
            displayBackpackScan();
        }
        clearBs2TempLootBuffer();
    }

    lastCardReadTime = millis();
}

// Network session state
NetworkState networkState;
unsigned long networkLastReconnectAttemptMs = 0;
unsigned long networkReconnectBackoffMs = WIFI_RECONNECT_BASE_DELAY;
unsigned long lastTelemetrySentMs = 0;

// Display power saving
unsigned long lastUserInteractionMs = 0;
bool displaySleeping = false;

// Buy Station 2 state
bool bs2WaitingForConfirm = false;
int bs2BrowsingIndex = 0;  // Current item being viewed when browsing loot
unsigned long bs2SessionCheckTime = 0;  // For periodic timeout checking

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
const unsigned long AUDIO_WARMUP_DELAY_MS = 800;  // Give speaker/I2S time to be ready before first UI screen

// ============================================
// SETUP AND MAIN LOOP
// ============================================

// ===================================================================
// DUPLICATE FUNCTIONS REMOVED - NOW IN SEPARATE HEADER FILES
// All utility functions have been moved to separate header files:
// - sendRelayUpdate, flipLootByUUID, fetchBadgeData, sendTransactionRequest -> ApiClient.h
// - sanitizeUuid, isAdminBadge, isWiFiRetryTag, isTouchInConfirmButton -> BadgeUtils.h  
// - getErrorDisplayMessage, displayErrorMessage -> ErrorHandler.h
// - reconnectWiFi -> WiFiManager.h
// ===================================================================

// ============================================
// TONE GENERATION
// ============================================
void playAcceptTone() {
    M5Dial.Speaker.tone(1047, 80, 0); // C6, 80ms
    M5Dial.Speaker.tone(1397, 100, 0); // F6, 100ms
}

void playErrorTone() {
    M5Dial.Speaker.tone(262, 200, 0); // C4, 200ms
}

void playBuzzTone() {
    M5Dial.Speaker.setVolume(150);
    for (int i = 0; i < 3; i++) {
        M5Dial.Speaker.tone(100, 40);
        delay(40);
    }
    M5Dial.Speaker.setVolume(255);
}

// Backpack mode reuses checkWhatIsIt(), which stages loot in bs2 temp arrays.
// Clear temp state after each backpack loot scan so mode behavior stays consistent.
void clearBs2TempLootBuffer() {
    for (int i = 0; i < MAX_LOOT_CART_ITEMS; i++) {
        bs2LootItems[i].lootId = "";
        bs2LootItems[i].lootUuid = "";
        bs2LootItems[i].lootName = "";
        bs2LootItems[i].rewardText = "";
        bs2LootItems[i].rewardsJson = "";
        bs2LootItems[i].locked = false;
        bs2LootItems[i].claimed = false;
    }
    bs2LootCount = 0;
    bs2LootLimit = 1;
}

// Track user interaction to manage display sleep
void recordUserActivity() {
    lastUserInteractionMs = millis();
    if (displaySleeping) {
        displaySleeping = false;
        M5Dial.Display.setBrightness(255);
    }
}

void playScanTone() {
    M5Dial.Speaker.tone(880, 50, 0); // A5, 50ms
    M5Dial.Speaker.tone(988, 50, 0); // B5, 50ms
}

void playSoftAcceptTone() {
    M5Dial.Speaker.setVolume(100);
    M5Dial.Speaker.tone(1047, 80, 0); // C6, 80ms, softer volume
    M5Dial.Speaker.tone(1397, 100, 0); // F6, 100ms
    M5Dial.Speaker.setVolume(255); // Reset volume
}

void playSoftErrorTone() {
    M5Dial.Speaker.setVolume(100);
    M5Dial.Speaker.tone(262, 200, 0); // C4, 200ms
    M5Dial.Speaker.setVolume(255); // Reset volume
}

void playPurchaseCompleteTone() {
    M5Dial.Speaker.setVolume(220);
    M5Dial.Speaker.tone(1047, 100, 0); // C6
    delay(100);
    M5Dial.Speaker.tone(1319, 100, 0); // E6
    delay(100);
    M5Dial.Speaker.tone(1568, 150, 0); // G6
    M5Dial.Speaker.setVolume(255);
}

void playReturnToScanTone() {
    M5Dial.Speaker.setVolume(200);
    M5Dial.Speaker.tone(784, 80, 0); // G5
    delay(80);
    M5Dial.Speaker.tone(587, 120, 0); // D5
    M5Dial.Speaker.setVolume(255);
}

void playEncoderClick() {
    M5Dial.Speaker.tone(1200, 20, 0);
}

// Query backend to determine badge type and return parsed JSON in `doc`
bool whatIsIt(String uuid, StaticJsonDocument<1024>& doc) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("whatIsIt: WiFi not connected");
        playErrorTone();
        return false;
    }

    HTTPClient http;
    beginHttp(http, what_is_it_url);
    configureHttpClient(http);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_REQUEST_TIMEOUT_MS);

    String payload = "{";
    payload += "\"uuid\":\"" + uuid + "\",";
    payload += "\"mac\":\"" + macAddress + "\",";
    payload += "\"serial\":\"" + serialNum + "\"";
    payload += "}";

    startNetworkSpinner("NET");
    unsigned long startMs = markHttpStart();
    int httpCode = http.POST(payload);
    String response = http.getString();
    markHttpEnd(startMs);
    stopNetworkSpinner();

    if (httpCode == 200 || httpCode == 201) {
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            Serial.println("whatIsIt: JSON parse error: " + String(error.c_str()));
            http.end();
            playErrorTone();
            return false;
        }

        http.end();
        playScanTone();
        return true;
    }

    Serial.println("whatIsIt: HTTP Error " + String(httpCode));
    http.end();
    playErrorTone();
    return false;
}

void setup() {
    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart < 200)) {
        delay(5);
    }
    Serial.println("M5 Dial Device Starting...");

    // Configure shared HTTPS client (TLS verification disabled like existing HTTPS usage)
    sharedHttpsClient.setInsecure();

    // Allow battery packs to stabilize before heavy peripheral/WiFi init
    delay(300);
    
    M5Dial.Speaker.begin(); // Initialize speaker for audio feedback
    M5Dial.Speaker.setVolume(255); // Ensure max volume available from first boot
    initializeM5Display();
    initializeNFC();
    initializePortARfid2();
    initializeWiFi();
    initEspNowReceiver();
    broadcastCurrentModeToSender();
    if (WiFi.status() == WL_CONNECTED) {
        SecureWs.begin();
    }
    
    // Fetch game configuration caches after WiFi/TLS is ready.
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long tlsWaitStart = millis();
        while (!networkState.tlsReady && millis() - tlsWaitStart < 3000) {
            serviceNetworkState();
            delay(25);
        }

        if (waitForSecureWsAuth(4000)) {
            Serial.println("Secure widget auth ready");
            if (SecureWs.gameId().length()) {
                gameId = SecureWs.gameId();
            }
        } else {
            Serial.println("Secure widget auth pending - legacy cache init continues");
        }

        Serial.println("Fetching factions for attached game...");
        if (fetchGameFactions()) {
            Serial.println("Game factions loaded successfully");
        } else {
            Serial.println("Failed to load game factions - hardcoded fallback IDs remain available");
        }

        Serial.println("Fetching player classes for game...");
        if (fetchGamePlayerClasses()) {
            Serial.println("Player classes loaded successfully");
        } else {
            Serial.println("Failed to load player classes - will use default multipliers");
        }
    }

    // Keep the Guru Games splash visible while audio hardware finishes warming up
    drawAnimationFrame(0, "Loading audio...");
    delay(AUDIO_WARMUP_DELAY_MS);
    
    // Show initial prompt for default mode
    resetBackpackModeState();
    displayBackpackScan();
    currentState = WAIT_FOR_PLAYER;

    lastUserInteractionMs = millis();
}

void loop() {
    M5Dial.update(); // Update M5 Dial state
    if (WiFi.status() == WL_CONNECTED) {
        SecureWs.begin();
        SecureWs.loop();
    }
    if (espNowRelayPending) {
        String pendingUuid = espNowRelayUuid;
        String pendingResponse = espNowRelayResponse;
        bool pendingSuccess = espNowRelaySuccess;
        espNowRelayPending = false;
        if (isRelayPostingMode()) {
            recordUserActivity();
            relayLastUuid = pendingUuid;
            relayLastResponse = pendingResponse;
            relayLastSuccess = pendingSuccess;
            displayRelayPostingMode(relayLastUuid, relayLastResponse, relayLastSuccess);
            if (relayLastSuccess) {
                relayLastUuid = "";
                scheduleRelayDisplayClear();
            }
            playScanTone();
        } else if (deviceMode == MODE_BUY_STATION_2 && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
            processBuyStation2BadgeScan(pendingUuid, true);
        } else if (deviceMode == MODE_BACKPACK && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
            processBackpackBadgeScan(pendingUuid, true);
        } else {
            Serial.printf("[ESPNOW] Scan received while in mode %d: %s\n", (int)deviceMode, pendingUuid.c_str());
        }
    }
    clearRelayDisplayIfReady();
    handlePortARfid2Scan();
    tickNetworkSpinner();
    serviceNetworkHealth();

    // Lightweight user activity detection for display sleep/wake
    static long lastEncoderForSleep = 0;
    long encVal = M5Dial.Encoder.read();
    bool btnPressed = M5Dial.BtnA.wasPressed();
    auto touchDetail = M5Dial.Touch.getDetail();
    bool touchPressed = touchDetail.wasPressed();
    bool encoderMoved = (encVal != lastEncoderForSleep);
    bool wakeEvent = btnPressed || touchPressed || encoderMoved;
    if (wakeEvent) {
        recordUserActivity();
        lastEncoderForSleep = encVal;
    } else if (!displaySleeping && millis() - lastUserInteractionMs > SCREEN_TIMEOUT_MS) {
        displaySleeping = true;
        M5Dial.Display.setBrightness(0);
    }
    
    // --- ADMIN MODE EXCLUSIVE BLOCK ---
    if (currentState == ADMIN_MODE || currentState == ADMIN_WIFI_STATUS || 
        currentState == ADMIN_LOOT_FLIP || currentState == ADMIN_WIFI_SCAN || 
        currentState == ADMIN_WIFI_PASSWORD) {
        
        long newPosition = M5Dial.Encoder.read();
        
        // --- Admin Mode Selection ---
        if (currentState == ADMIN_MODE) {
            long currentStep = newPosition / ADMIN_ENCODER_STEP;
            if (currentStep != adminEncoderStep) {
                if (currentStep > adminEncoderStep) {
                    adminMenuSelection++;
                    if (adminMenuSelection >= ADMIN_MENU_ITEMS) adminMenuSelection = 0;
                } else if (currentStep < adminEncoderStep) {
                    adminMenuSelection--;
                    if (adminMenuSelection < 0) adminMenuSelection = ADMIN_MENU_ITEMS - 1;
                }
                adminEncoderStep = currentStep;
                playEncoderClick();
                displayAdminMenu(adminMenuSelection, previousDeviceMode);
                lastActivityTime = millis();
            }

            if (M5Dial.BtnA.wasPressed() && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
                lastButtonPress = millis();
                lastActivityTime = millis();
                playEncoderClick();
                
                switch(adminMenuSelection) {
                    case 0: // Device Info
                        displayDeviceInfo(serialNum, deviceMode);
                        delay(5000);
                        displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        break;
                    case 1: // WiFi Status
                        displayWiFiStatus();
                        delay(4000);
                        displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        break;
                    case 2: // WiFi Reconnect
                        reconnectWiFi();
                        displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        break;
                    case 3: // WiFi Scan
                        currentState = ADMIN_WIFI_SCAN;
                        wifiNetworkCount = 0;
                        selectedNetworkIndex = 0;
                        displayWiFiScanning();
                        scanWiFiNetworks(scannedSSIDs, scannedRSSI, scannedEncrypted, wifiNetworkCount);
                        encoderPosition = M5Dial.Encoder.read();
                        displayWiFiNetworkList(scannedSSIDs, scannedRSSI, scannedEncrypted, wifiNetworkCount, selectedNetworkIndex);
                        break;
                    case 4: // Loot Lock/Unlock
                        currentState = ADMIN_LOOT_FLIP;
                        lootFlipLastUuid = "";
                        lootFlipLastStatus = "";
                        displayLootFlipMode();
                        break;
                    case 5: // Backpack Mode
                        deviceMode = MODE_BACKPACK;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        resetBackpackModeState();
                        displayBackpackScan();
                        broadcastCurrentModeToSender();
                        break;
                    case 6: // Buy Station 2 Mode
                        deviceMode = MODE_BUY_STATION_2;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                        broadcastCurrentModeToSender();
                        break;
                    case 7: // Loot Transfer
                        deviceMode = MODE_LOOT_TRANSFER;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        resetLootTransferState();
                        displayLootTransferScan();
                        broadcastCurrentModeToSender();
                        break;
                    case 8: // Relay Mode
                        deviceMode = MODE_RELAY;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        displayRelayMode();
                        broadcastCurrentModeToSender();
                        break;
                    case 9: // Photobooth Mode
                        deviceMode = MODE_PHOTOBOOTH;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        displayPhotoboothMode();
                        broadcastCurrentModeToSender();
                        break;
                    case 10: // Map Mode
                        deviceMode = MODE_MAP;
                        currentState = WAIT_FOR_PLAYER;
                        displaySuccessBlink();
                        resetMapModeState();
                        displayMapModeStatus("Ready", GREEN);
                        broadcastCurrentModeToSender();
                        break;
                    case 11: // Dev Mode Toggle
                        bs2DevMode = !bs2DevMode;
                        displaySuccessBlink();
                        displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        break;
                    case 12: // View Variables
                        {
                            int scrollIndex = 0;
                            displayVariableViewer(scrollIndex);
                            long initialEncoder = M5Dial.Encoder.read();
                            bool viewingVars = true;
                            
                            while (viewingVars) {
                                M5Dial.update();
                                long currentEncoder = M5Dial.Encoder.read();
                                if (abs(currentEncoder - initialEncoder) >= 4) { // Update every 4 clicks for smoother scrolling
                                    if (currentEncoder > initialEncoder) scrollIndex++;
                                    else scrollIndex--;
                                    initialEncoder = currentEncoder;
                                    displayVariableViewer(scrollIndex);
                                }
                                
                                if (M5Dial.BtnA.wasPressed()) {
                                    playEncoderClick();
                                    viewingVars = false;
                                }
                                delay(20);
                            }
                            displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        }
                        break;
                    case 13: // Clear Variables
                        {
                            M5Dial.Display.clear();
                            M5Dial.Display.setTextColor(RED, BLACK);
                            M5Dial.Display.setTextDatum(middle_center);
                            M5Dial.Display.drawString("Clearing Cache...", 120, 120);
                            
                            bs2BadgeUuid = ""; bs2BadgeName = ""; bs2BadgeFaction = ""; bs2BadgeFactionId = "";
                            bs2PlayerClass = ""; bs2PlayerClassId = ""; bs2PlayerClassName = "";
                            bs2ClassMultiplier = 1; bs2BackpackValue = 0; bs2LootLimit = 1;
                            bs2LootCount = 0; bs2ClaimedCount = 0; bs2LastUsedBadge = "";
                            for(int i=0; i<MAX_LOOT_CART_ITEMS; i++) {
                                bs2LootItems[i].lootId = ""; bs2LootItems[i].lootUuid = ""; bs2LootItems[i].lootName = "";
                                bs2LootItems[i].rewardText = ""; bs2LootItems[i].rewardsJson = "";
                                bs2LootItems[i].locked = false; bs2LootItems[i].claimed = false;
                            }
                            bs2WaitingForConfirm = false; bs2BrowsingIndex = 0; bs2LastActivityTime = 0;
                            
                            relayLastUuid = ""; relayLastResponse = ""; relayLastSuccess = false;
                            lootFlipLastUuid = ""; lootFlipLastStatus = "";
                            
                            ltFactionA_BadgeUuid = ""; ltFactionA_BadgeName = ""; ltFactionA_FactionName = ""; ltFactionA_FactionId = "";
                            ltFactionA_ItemCount = 0; ltFactionA_SelectedIndex = -1; ltFactionA_SelectedAmount = 0;
                            ltFactionB_BadgeUuid = ""; ltFactionB_BadgeName = ""; ltFactionB_FactionName = ""; ltFactionB_FactionId = "";
                            ltFactionB_ItemCount = 0; ltFactionB_SelectedIndex = -1; ltFactionB_SelectedAmount = 0;
                            ltCurrentState = LT_WAITING_FIRST_BADGE;
                            ltBrowsingIndex = 0; ltTempAmount = 0; ltLastActivityTime = 0;
                            
                            clearPlayerClassCache();
                            
                            delay(1000);
                            playAcceptTone();
                            displayAdminMenu(adminMenuSelection, previousDeviceMode);
                        }
                        break;
                    case 14: // Exit Admin
                        currentState = WAIT_FOR_PLAYER;
                        deviceMode = previousDeviceMode;
                        displaySuccessBlink();
                        displayActiveOperationalMode();
                        break;
                }
            }
        }
        // --- Admin WiFi Scan ---
        else if (currentState == ADMIN_WIFI_SCAN) {
            long currentStep = newPosition / 4;
            if (currentStep != adminEncoderStep && wifiNetworkCount > 0) {
                if (currentStep > adminEncoderStep) {
                    selectedNetworkIndex++;
                    if (selectedNetworkIndex >= wifiNetworkCount) selectedNetworkIndex = 0;
                } else if (currentStep < adminEncoderStep) {
                    selectedNetworkIndex--;
                    if (selectedNetworkIndex < 0) selectedNetworkIndex = wifiNetworkCount - 1;
                }
                adminEncoderStep = currentStep;
                playEncoderClick();
                displayWiFiNetworkList(scannedSSIDs, scannedRSSI, scannedEncrypted, wifiNetworkCount, selectedNetworkIndex);
                lastActivityTime = millis();
            }

            if (M5Dial.BtnA.wasPressed() && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
                lastButtonPress = millis();
                lastActivityTime = millis();
                playEncoderClick();
                
                if (wifiNetworkCount > 0 && scannedEncrypted[selectedNetworkIndex]) {
                    currentState = ADMIN_WIFI_PASSWORD;
                    enteredPassword = "";
                    currentCharIndex = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    adminEncoderStep = encoderPosition / 4;
                    displayPasswordEntry(scannedSSIDs[selectedNetworkIndex], enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
                } else if (wifiNetworkCount > 0) {
                    displayWiFiConnecting(scannedSSIDs[selectedNetworkIndex]);
                    bool success = connectToSelectedNetwork(scannedSSIDs[selectedNetworkIndex], "");
                    displayWiFiConnectionResult(success, scannedSSIDs[selectedNetworkIndex]);
                    delay(3000);
                    currentState = ADMIN_MODE;
                    displayAdminMenu(adminMenuSelection, previousDeviceMode);
                } else {
                    currentState = ADMIN_MODE;
                    displayAdminMenu(adminMenuSelection, previousDeviceMode);
                }
            }
        }
        // --- Admin WiFi Password Entry ---
        else if (currentState == ADMIN_WIFI_PASSWORD) {
            long currentStep = newPosition / 4;
            if (currentStep != adminEncoderStep) {
                if (currentStep > adminEncoderStep) {
                    currentCharIndex++;
                    if (currentCharIndex >= charSetSize) currentCharIndex = 0;
                } else if (currentStep < adminEncoderStep) {
                    currentCharIndex--;
                    if (currentCharIndex < 0) currentCharIndex = charSetSize - 1;
                }
                adminEncoderStep = currentStep;
                playEncoderClick();
                displayPasswordEntry(scannedSSIDs[selectedNetworkIndex], enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
                lastActivityTime = millis();
            }

            if (M5Dial.BtnA.wasPressed() && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
                lastButtonPress = millis();
                lastActivityTime = millis();
                playEncoderClick();
                
                if (strncmp(&charSet[currentCharIndex], "<DEL>", 5) == 0) {
                    if (enteredPassword.length() > 0) {
                        enteredPassword.remove(enteredPassword.length() - 1);
                    }
                } else if (strncmp(&charSet[currentCharIndex], "<DONE>", 6) == 0) {
                    displayWiFiConnecting(scannedSSIDs[selectedNetworkIndex]);
                    bool success = connectToSelectedNetwork(scannedSSIDs[selectedNetworkIndex], enteredPassword);
                    displayWiFiConnectionResult(success, scannedSSIDs[selectedNetworkIndex]);
                    delay(3000);
                    currentState = ADMIN_MODE;
                    displayAdminMenu(adminMenuSelection, previousDeviceMode);
                } else {
                    enteredPassword += charSet[currentCharIndex];
                }
                displayPasswordEntry(scannedSSIDs[selectedNetworkIndex], enteredPassword, passwordCursorPos, charSet, charSetSize, currentCharIndex);
            }
        }
        // --- Admin Loot Lock/Unlock ---
        else if (currentState == ADMIN_LOOT_FLIP) {
            if (M5Dial.BtnA.wasPressed() && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
                lastButtonPress = millis();
                playEncoderClick();
                currentState = ADMIN_MODE;
                displayAdminMenu(adminMenuSelection, previousDeviceMode);
                return;
            }

            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > RELAY_SCAN_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                if (isAdminBadge(scannedUuid)) {
                    playAcceptTone();
                    currentState = ADMIN_MODE;
                    displayAdminMenu(adminMenuSelection, previousDeviceMode);
                } else {
                    displayCenteredText("PROCESSING...", THEME_ACCENT_COLOR, BLACK, 2);
                    lootFlipLastUuid = scannedUuid;
                    lootFlipLastStatus = flipLootByUUID(scannedUuid);
                    displayLootFlipMode(lootFlipLastUuid, lootFlipLastStatus);
                }
                lastCardReadTime = millis();
            }
        }
        
        // Auto exit admin mode after inactivity (60s)
        if (millis() - lastActivityTime > 60000) {
            Serial.println("Admin mode timeout - exiting");
            currentState = WAIT_FOR_PLAYER;
            deviceMode = previousDeviceMode;
            displayActiveOperationalMode();
        }
        
        return; // EXACTLY HERE IS THE FIX: Break out of loop entirely if we handled admin mode
    }
    // --- END ADMIN MODE BLOCK ---
    
    // OTA RFID pre-check removed to prevent consuming card reads
    // before active mode handlers can process scans.
    
    // Buy Station 2 mode - cart flow with final confirm handoff.
    if (deviceMode == MODE_BUY_STATION_2 && !(currentState == CONFIRM_TRANSACTION && bs2FinalConfirmPending)) {
        // Timeout idle sessions.
        if (millis() - bs2SessionCheckTime > 1000) {
            bs2SessionCheckTime = millis();
            if (bs2BadgeUuid.length() > 0 && (millis() - bs2LastActivityTime > BS2_SESSION_TIMEOUT)) {
                Serial.println("Buy Station 2 - Session timeout");
                playErrorTone();
                displayCenteredText("SESSION TIMEOUT", ORANGE, BLACK, 2);
                delay(1200);
                resetBuyStation2State();
                bs2FinalConfirmPending = false;
                displayBuyStation2Scan();
                return;
            }
        }

        // Browse items with encoder once cart has entries.
        if (bs2WaitingForConfirm && bs2LootCount > 0) {
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                bs2LastActivityTime = millis();
                int delta = (newPosition - encoderPosition) / 4;
                if (delta != 0) {
                    bs2BrowsingIndex += delta;
                    if (bs2BrowsingIndex < 0) bs2BrowsingIndex = bs2LootCount - 1;
                    if (bs2BrowsingIndex >= bs2LootCount) bs2BrowsingIndex = 0;
                    displayBuyStation2LootBrowser(bs2BrowsingIndex, bs2LootCount, bs2LootItems[bs2BrowsingIndex].lootName, bs2BadgeName, bs2LootItems[bs2BrowsingIndex].rewardText);
                    encoderPosition = newPosition;
                }
            }
        }

        // Enter final confirmation from cart via tap or button.
        auto bs2Touch = M5Dial.Touch.getDetail();
        if (bs2WaitingForConfirm && bs2LootCount > 0 && (M5Dial.BtnA.wasPressed() || bs2Touch.wasPressed())) {
            recordUserActivity();
            bs2LastActivityTime = millis();
            bs2FinalConfirmPending = true;
            currentState = CONFIRM_TRANSACTION;
            confirmStateEnteredMs = millis();
            displayTransactionSummary(bs2BadgeName, String(bs2LootCount) + " ITEM(S)");
            return;
        }

        // Handle card scans in BS2 mode.
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > RELAY_SCAN_DEBOUNCE)) {
            recordUserActivity();
            String scannedUuid = sanitizeUuid(readNFCCardUID());
            processBuyStation2BadgeScan(scannedUuid, false);
            lastCardReadTime = millis();
        }

        return;  // Stay in Buy Station 2 mode
    }
    
    // Backpack mode - claim cart items with the Backpack session flow.
    if (deviceMode == MODE_BACKPACK && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS && currentState != ADMIN_LOOT_FLIP && currentState != ADMIN_WIFI_SCAN && currentState != ADMIN_WIFI_PASSWORD) {
        // Check for session timeout (every 1 second)
        if (millis() - bs2SessionCheckTime > 1000) {
            bs2SessionCheckTime = millis();
            if (bpBadgeUuid.length() > 0 && (millis() - bpLastActivityTime > BP_SESSION_TIMEOUT)) {
                Serial.println("Backpack Mode - Session timeout");
                if (bpLootCount > 0) {
                    // No interaction: auto-claim cart before timing out the session.
                    Serial.println("Backpack Mode - Auto-claiming cart on timeout");
                    bpWaitingForConfirm = false;
                    int totalItems = bpLootCount;
                    int successCount = claimAndUnlockAllLoot();
                    displayBuyStation2Results(successCount, totalItems);
                    playPurchaseCompleteTone();
                    delay(2200);

                    displayCenteredText("SESSION TIMED OUT", ORANGE, BLACK, 2);
                    delay(1200);
                } else {
                    playErrorTone();
                    displayCenteredText("SESSION TIMEOUT", ORANGE, BLACK, 2);
                    delay(1500);
                }

                resetBackpackModeState();
                playReturnToScanTone();
                displayBackpackScan();
                lastCardReadTime = millis();
                return;
            }
        }
        
        // Handle encoder rotation when browsing loot items
        if (bpWaitingForConfirm && bpLootCount > 0) {
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                bpLastActivityTime = millis();
                int delta = (newPosition - encoderPosition) / 4;
                if (delta != 0) {
                    bpBrowsingIndex += delta;
                    if (bpBrowsingIndex < 0) bpBrowsingIndex = bpLootCount - 1;
                    if (bpBrowsingIndex >= bpLootCount) bpBrowsingIndex = 0;
                    
                    displayBuyStation2LootBrowser(bpBrowsingIndex, bpLootCount, bpLootItems[bpBrowsingIndex].lootName, bpBadgeName, bpLootItems[bpBrowsingIndex].rewardText);
                    encoderPosition = newPosition;
                }
            }
        }
        
        // Claim loot with either button press or tap.
        if (M5Dial.BtnA.wasPressed() || touchPressed) {
                recordUserActivity();
            bpLastActivityTime = millis();
            if (bpWaitingForConfirm && bpLootCount > 0) {
                bpWaitingForConfirm = false;
                
                Serial.println("Backpack Mode - claiming " + String(bpLootCount) + " item(s)");
                
                // Claim all loot
                int successCount = claimAndUnlockAllLoot();
                
                // Show results
                displayBuyStation2Results(successCount, bpLootCount);
                playPurchaseCompleteTone();
                delay(3000);
                
                // Reset and return to scan badge
                Serial.println("Backpack Mode - complete, resetting session");
                resetBackpackModeState();
                playReturnToScanTone();
                displayBackpackScan();
                lastCardReadTime = millis();
                return;
            }
        }
        
        // Scan for badges locally
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > RELAY_SCAN_DEBOUNCE)) {
            String scannedUuid = sanitizeUuid(readNFCCardUID());
            processBackpackBadgeScan(scannedUuid, false);
        }
        return;  // Stay in Backpack mode
    }
    
    // Loot Transfer mode - Transfer inventory items between factions
    if (deviceMode == MODE_LOOT_TRANSFER && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS && currentState != ADMIN_LOOT_FLIP && currentState != ADMIN_WIFI_SCAN && currentState != ADMIN_WIFI_PASSWORD) {
        // Check for session timeout (every 1 second)
        if (millis() - bs2SessionCheckTime > 1000) {
            bs2SessionCheckTime = millis();
            if (ltCurrentState != LT_WAITING_FIRST_BADGE && (millis() - ltLastActivityTime > LT_SESSION_TIMEOUT)) {
                Serial.println("Loot Transfer - Session timeout");
                playErrorTone();
                displayCenteredText("SESSION TIMEOUT", ORANGE, BLACK, 2);
                delay(1500);
                resetLootTransferState();
                displayLootTransferScan();
            }
        }
        
        // Handle encoder rotation based on current state
        long newPosition = M5Dial.Encoder.read();
        if (newPosition != encoderPosition) {
            ltLastActivityTime = millis();
            int delta = (newPosition - encoderPosition) / 4;
            
            if (delta != 0) {
                if (ltCurrentState == LT_BROWSING_FACTION_A) {
                    ltBrowsingIndex += delta;
                    if (ltBrowsingIndex < 0) ltBrowsingIndex = ltFactionA_ItemCount - 1;
                    if (ltBrowsingIndex >= ltFactionA_ItemCount) ltBrowsingIndex = 0;
                    displayLootTransferItemBrowser(ltBrowsingIndex, ltFactionA_ItemCount, 
                                                   ltFactionA_Items[ltBrowsingIndex].itemName,
                                                   ltFactionA_Items[ltBrowsingIndex].amount,
                                                   ltFactionA_Items[ltBrowsingIndex].canTransfer);
                } 
                else if (ltCurrentState == LT_SELECTING_AMOUNT_A) {
                    ltTempAmount += delta;
                    if (ltTempAmount < 0) ltTempAmount = 0;
                    if (ltTempAmount > ltFactionA_Items[ltFactionA_SelectedIndex].amount) {
                        ltTempAmount = ltFactionA_Items[ltFactionA_SelectedIndex].amount;
                    }
                    displayLootTransferAmountSelector(ltFactionA_Items[ltFactionA_SelectedIndex].itemName, 
                                                      ltTempAmount, 
                                                      ltFactionA_Items[ltFactionA_SelectedIndex].amount,
                                                      "FACTION A");
                }
                else if (ltCurrentState == LT_BROWSING_FACTION_B) {
                    ltBrowsingIndex += delta;
                    if (ltBrowsingIndex < 0) ltBrowsingIndex = ltFactionB_ItemCount - 1;
                    if (ltBrowsingIndex >= ltFactionB_ItemCount) ltBrowsingIndex = 0;
                    displayLootTransferItemBrowser(ltBrowsingIndex, ltFactionB_ItemCount, 
                                                   ltFactionB_Items[ltBrowsingIndex].itemName,
                                                   ltFactionB_Items[ltBrowsingIndex].amount,
                                                   ltFactionB_Items[ltBrowsingIndex].canTransfer);
                }
                else if (ltCurrentState == LT_SELECTING_AMOUNT_B) {
                    ltTempAmount += delta;
                    if (ltTempAmount < 0) ltTempAmount = 0;
                    if (ltTempAmount > ltFactionB_Items[ltFactionB_SelectedIndex].amount) {
                        ltTempAmount = ltFactionB_Items[ltFactionB_SelectedIndex].amount;
                    }
                    displayLootTransferAmountSelector(ltFactionB_Items[ltFactionB_SelectedIndex].itemName, 
                                                      ltTempAmount, 
                                                      ltFactionB_Items[ltFactionB_SelectedIndex].amount,
                                                      "FACTION B");
                }
                encoderPosition = newPosition;
            }
        }
        
        // Handle button press based on current state
        if (M5Dial.BtnA.wasPressed()) {
            ltLastActivityTime = millis();
            
            if (ltCurrentState == LT_BROWSING_FACTION_A && ltFactionA_ItemCount > 0) {
                if (ltFactionA_Items[ltBrowsingIndex].canTransfer) {
                    ltFactionA_SelectedIndex = ltBrowsingIndex;
                    ltTempAmount = min(1, ltFactionA_Items[ltBrowsingIndex].amount);
                    ltCurrentState = LT_SELECTING_AMOUNT_A;
                    displayLootTransferAmountSelector(ltFactionA_Items[ltBrowsingIndex].itemName, 
                                                      ltTempAmount, 
                                                      ltFactionA_Items[ltBrowsingIndex].amount,
                                                      "FACTION A");
                } else {
                    displayCenteredText("CANNOT TRANSFER", RED, BLACK, 2);
                    delay(1500);
                    displayLootTransferItemBrowser(ltBrowsingIndex, ltFactionA_ItemCount, 
                                                   ltFactionA_Items[ltBrowsingIndex].itemName,
                                                   ltFactionA_Items[ltBrowsingIndex].amount,
                                                   ltFactionA_Items[ltBrowsingIndex].canTransfer);
                }
            }
            else if (ltCurrentState == LT_SELECTING_AMOUNT_A) {
                ltFactionA_SelectedAmount = ltTempAmount;
                ltCurrentState = LT_WAITING_SECOND_BADGE;
                displayCenteredText("SCAN RECEIVING LEADER BADGE", THEME_ACCENT_COLOR, BLACK, 2);
                delay(1000);
            }
            else if (ltCurrentState == LT_BROWSING_FACTION_B && ltFactionB_ItemCount > 0) {
                if (ltFactionB_Items[ltBrowsingIndex].canTransfer) {
                    ltFactionB_SelectedIndex = ltBrowsingIndex;
                    ltTempAmount = min(1, ltFactionB_Items[ltBrowsingIndex].amount);
                    ltCurrentState = LT_SELECTING_AMOUNT_B;
                    displayLootTransferAmountSelector(ltFactionB_Items[ltBrowsingIndex].itemName, 
                                                      ltTempAmount, 
                                                      ltFactionB_Items[ltBrowsingIndex].amount,
                                                      "FACTION B");
                } else {
                    displayCenteredText("CANNOT TRANSFER", RED, BLACK, 2);
                    delay(1500);
                    displayLootTransferItemBrowser(ltBrowsingIndex, ltFactionB_ItemCount, 
                                                   ltFactionB_Items[ltBrowsingIndex].itemName,
                                                   ltFactionB_Items[ltBrowsingIndex].amount,
                                                   ltFactionB_Items[ltBrowsingIndex].canTransfer);
                }
            }
            else if (ltCurrentState == LT_SELECTING_AMOUNT_B) {
                ltFactionB_SelectedAmount = ltTempAmount;
                ltCurrentState = LT_CHECKOUT_CONFIRM;
                displayLootTransferCheckout();
            }
            else if (ltCurrentState == LT_CHECKOUT_CONFIRM) {
                // Execute transaction
                displayCenteredText("PROCESSING...", THEME_ACCENT_COLOR, BLACK, 2);
                bool success = executeTransaction();
                
                if (success) {
                    displayCenteredText("TRANSACTION COMPLETE", GREEN, BLACK, 2);
                    playPurchaseCompleteTone();
                } else {
                    displayCenteredText("TRANSACTION FAILED", RED, BLACK, 2);
                    playErrorTone();
                }
                delay(3000);
                
                resetLootTransferState();
                playReturnToScanTone();
                displayLootTransferScan();
            }
        }
        
        // Scan for badges
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > RELAY_SCAN_DEBOUNCE)) {
            recordUserActivity();
            String scannedUuid = sanitizeUuid(readNFCCardUID());
            
            // Check for admin badge
            if (isAdminBadge(scannedUuid)) {
                Serial.println("Admin badge detected!");
                previousDeviceMode = deviceMode; // Save current mode
                displayAdminAccessGranted();
                currentState = ADMIN_MODE;
                adminMenuSelection = 0;
                encoderPosition = M5Dial.Encoder.read();
                adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastCardReadTime = millis();
                return;
            }
            
            ltLastActivityTime = millis();
            
            // Handle first badge scan
            if (ltCurrentState == LT_WAITING_FIRST_BADGE) {
                displayCenteredText("LOADING...", THEME_ACCENT_COLOR, BLACK, 2);
                Serial.println("=== Loot Transfer: Fetching Faction A Inventory ===");
                Serial.println("UUID: " + scannedUuid);
                
                // Check if it's a badge
                String badgeType = checkWhatIsIt(scannedUuid);
                if (badgeType != "badge") {
                    displayCenteredText("BADGE REQUIRED", RED, BLACK, 2);
                    delay(1500);
                    displayLootTransferScan();
                    lastCardReadTime = millis();
                    return;
                }
                
                // Store faction ID from whatIsIt response
                ltFactionA_FactionId = bs2BadgeFactionId;
                
                // Fetch faction A inventory
                bool success = fetchFactionInventory(scannedUuid, true);
                
                if (success && ltFactionA_ItemCount > 0) {
                    ltFactionA_BadgeUuid = scannedUuid;
                    ltFactionA_BadgeName = bs2BadgeName;
                    ltFactionA_FactionName = bs2BadgeFaction;
                    // Faction ID already stored above from whatIsIt
                    
                    // Clear bs2 variables
                    bs2BadgeUuid = "";
                    bs2BadgeName = "";
                    bs2BadgeFaction = "";
                    bs2BadgeFactionId = "";
                    
                    Serial.println("Faction A: " + ltFactionA_BadgeName + " (" + ltFactionA_FactionName + ")");
                    Serial.println("Items in inventory: " + String(ltFactionA_ItemCount));
                    
                    // Show inventory and start browsing
                    displayLootTransferInventory(ltFactionA_BadgeName, ltFactionA_FactionName, ltFactionA_ItemCount);
                    delay(2000);
                    
                    ltCurrentState = LT_BROWSING_FACTION_A;
                    ltBrowsingIndex = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    displayLootTransferItemBrowser(0, ltFactionA_ItemCount, 
                                                   ltFactionA_Items[0].itemName,
                                                   ltFactionA_Items[0].amount,
                                                   ltFactionA_Items[0].canTransfer);
                } else if (success && ltFactionA_ItemCount == 0) {
                    displayCenteredText("NO ITEMS IN INVENTORY", ORANGE, BLACK, 2);
                    delay(2000);
                    displayLootTransferScan();
                } else {
                    displayCenteredText("FETCH FAILED", RED, BLACK, 2);
                    delay(2000);
                    displayLootTransferScan();
                }
            }
            // Handle second badge scan
            else if (ltCurrentState == LT_WAITING_SECOND_BADGE) {
                displayCenteredText("LOADING...", THEME_ACCENT_COLOR, BLACK, 2);
                Serial.println("=== Loot Transfer: Fetching Faction B Inventory ===");
                Serial.println("UUID: " + scannedUuid);
                
                // Check if it's a badge
                String badgeType = checkWhatIsIt(scannedUuid);
                if (badgeType != "badge") {
                    displayCenteredText("BADGE REQUIRED", RED, BLACK, 2);
                    delay(1500);
                    displayCenteredText("SCAN RECEIVING LEADER BADGE", THEME_ACCENT_COLOR, BLACK, 2);
                    lastCardReadTime = millis();
                    return;
                }
                
                // Store faction ID from whatIsIt response
                ltFactionB_FactionId = bs2BadgeFactionId;
                
                // Fetch faction B inventory
                bool success = fetchFactionInventory(scannedUuid, false);
                
                if (success && ltFactionB_ItemCount > 0) {
                    ltFactionB_BadgeUuid = scannedUuid;
                    ltFactionB_BadgeName = bs2BadgeName;
                    ltFactionB_FactionName = bs2BadgeFaction;
                    // Faction ID already stored above from whatIsIt
                    
                    // Clear bs2 variables
                    bs2BadgeUuid = "";
                    bs2BadgeName = "";
                    bs2BadgeFaction = "";
                    bs2BadgeFactionId = "";
                    
                    Serial.println("Faction B: " + ltFactionB_BadgeName + " (" + ltFactionB_FactionName + ")");
                    Serial.println("Items in inventory: " + String(ltFactionB_ItemCount));
                    
                    // Show inventory and start browsing
                    displayLootTransferInventory(ltFactionB_BadgeName, ltFactionB_FactionName, ltFactionB_ItemCount);
                    delay(2000);
                    
                    ltCurrentState = LT_BROWSING_FACTION_B;
                    ltBrowsingIndex = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    displayLootTransferItemBrowser(0, ltFactionB_ItemCount, 
                                                   ltFactionB_Items[0].itemName,
                                                   ltFactionB_Items[0].amount,
                                                   ltFactionB_Items[0].canTransfer);
                } else if (success && ltFactionB_ItemCount == 0) {
                    displayCenteredText("NO ITEMS IN INVENTORY", ORANGE, BLACK, 2);
                    delay(2000);
                    displayCenteredText("SCAN RECEIVING LEADER BADGE", THEME_ACCENT_COLOR, BLACK, 2);
                } else {
                    displayCenteredText("FETCH FAILED", RED, BLACK, 2);
                    delay(2000);
                    displayCenteredText("SCAN RECEIVING LEADER BADGE", THEME_ACCENT_COLOR, BLACK, 2);
                }
            }
            
            lastCardReadTime = millis();
        }
        return;  // Stay in Loot Transfer mode
    }
    
    // If in persistent relay-style posting mode, handle that separately
    if (isRelayPostingMode() && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
        // Relay/photobooth mode: just scan and post last_uuid to the IoT device record
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > RELAY_SCAN_DEBOUNCE)) {
            recordUserActivity();
            String scannedUuid = sanitizeUuid(readNFCCardUID());
            
            // Check for admin badge to access menu
            if (isAdminBadge(scannedUuid)) {
                Serial.println("Admin badge detected!");
                displayAdminAccessGranted();
                previousDeviceMode = deviceMode; // Save current mode
                currentState = ADMIN_MODE;
                adminMenuSelection = 0;
                encoderPosition = M5Dial.Encoder.read();
                adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastCardReadTime = millis();
                return;
            }
            
            handleRelayPostingScan(scannedUuid, "RFID");
            
            lastCardReadTime = millis();
        }
        return;  // Stay in posting mode, don't process buy station logic
    }
    
    // If in map mode, handle that separately
    if (deviceMode == MODE_MAP && currentState != ADMIN_MODE && currentState != ADMIN_WIFI_STATUS) {
        // Map mode: display map and allow refresh
        
        // Check for admin badge to access menu
        if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
            recordUserActivity();
            String scannedUuid = sanitizeUuid(readNFCCardUID());
            
            if (isAdminBadge(scannedUuid)) {
                Serial.println("Admin badge detected - exiting map mode!");
                exitMapMode();
                displayAdminAccessGranted();
                previousDeviceMode = deviceMode; // Save current mode
                currentState = ADMIN_MODE;
                adminMenuSelection = 0;
                encoderPosition = M5Dial.Encoder.read();
                adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastCardReadTime = millis();
                return;
            }
            
            lastCardReadTime = millis();
        }
        
        // Update map mode (handles button presses for refresh)
        if (mapModeActive) {
            updateMapMode();
        }
        
        return;  // Stay in map mode, don't process other logic
    }
    
    switch(currentState) {
        case WAIT_FOR_PLAYER:
            // Ensure cart/session is fresh when idle for a new player
            bs2LootCount = 0;
            bs2WaitingForConfirm = false;
            bs2BrowsingIndex = 0;
            bs2ClaimedCount = 0;

            // Draw WiFi status indicator
            drawWiFiIndicator();
            
            // Show player info if button pressed and player has been scanned
            if (M5Dial.BtnA.wasPressed() && playerUuid.length() > 0) {
                displayPlayerInfo(playerName, playerFaction);
                delay(PLAYER_INFO_DISPLAY_TIME);
                displayCenteredText(TEXT_SCAN_LOOT, SCAN_LOOT_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
            }
            
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                recordUserActivity();
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                // Check for admin badge FIRST
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge detected!");
                    displayAdminAccessGranted();
                    previousDeviceMode = deviceMode; // Save current mode
                    currentState = ADMIN_MODE;
                    adminMenuSelection = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    displayAdminMenu(adminMenuSelection, deviceMode);
                    lastActivityTime = millis();
                    lastCardReadTime = millis();
                    break;
                }
                
                // Check if this is a predefined WiFi retry tag
                if (isWiFiRetryTag(scannedUuid)) {
                    Serial.println("WiFi retry tag detected: " + scannedUuid);
                    reconnectWiFi();
                    displayBlackMarketScan();
                    lastCardReadTime = millis();
                    break;
                }
                
                playerUuid = scannedUuid;
                Serial.println("Player ID: " + playerUuid);
                
                // Visual feedback
                badgeSwipeBlink();
                displayLoading(TEXT_LOADING_PLAYER);
                
                // Fetch player data from server
                int fetchResult = fetchBadgeData(playerUuid, true);
                
                // Check if server returned ORANGE faction (WiFi retry)
                if (fetchResult == -1) {
                    Serial.println("ORANGE faction card - triggering WiFi retry");
                    reconnectWiFi();
                    displayBlackMarketScan();
                    lastCardReadTime = millis();
                    break;
                }
                
                displayPlayerInfo(playerName, playerFaction);
                delay(PLAYER_INFO_DISPLAY_TIME);
                
                // Move to next state
                currentState = WAIT_FOR_LOOT;
                displayBlackMarketScanLoot();
                lastCardReadTime = millis();
            }
            break;
            
        case WAIT_FOR_LOOT:
            // Draw WiFi status indicator
            drawWiFiIndicator();
            
            // Show info if button pressed - toggle between player and loot
            if (M5Dial.BtnA.wasPressed()) {
                if (lootUuid.length() > 0) {
                    displayItemInfo(itemName, itemType);
                    delay(PLAYER_LOOT_TOGGLE_TIME);
                    displayPlayerInfo(playerName, playerFaction);
                    delay(PLAYER_LOOT_TOGGLE_TIME);
                } else if (playerUuid.length() > 0) {
                    displayPlayerInfo(playerName, playerFaction);
                    delay(PLAYER_INFO_DISPLAY_TIME);
                }
                displayBlackMarketScanLoot();
            }
            
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                recordUserActivity();
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                // Check for admin badge FIRST
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge detected!");
                    displayAdminAccessGranted();
                    currentState = ADMIN_MODE;
                    adminMenuSelection = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    displayAdminMenu(adminMenuSelection, deviceMode);
                    lastActivityTime = millis();
                    lastCardReadTime = millis();
                    break;
                }
                
                lootUuid = scannedUuid;
                Serial.println("Loot ID: " + lootUuid);
                
                // Visual feedback with spinning globe
                lootSwipeBlink();
                displayLoading(TEXT_LOADING_LOOT);
                
                // Fetch item data from server
                fetchBadgeData(lootUuid, false);
                displayItemInfo(itemName, itemType);
                delay(LOOT_INFO_DISPLAY_TIME);
                
                // Move to confirm state
                currentState = CONFIRM_TRANSACTION;
                confirmStateEnteredMs = millis();
                displayTransactionSummary(playerName, itemName);
                lastCardReadTime = millis();
            }
            break;
            
        case CONFIRM_TRANSACTION: {
            // Draw WiFi status indicator
            drawWiFiIndicator();

            unsigned long confirmElapsedMs = 0;
            if (confirmStateEnteredMs > 0) {
                confirmElapsedMs = millis() - confirmStateEnteredMs;
            }
            drawConfirmCountdownRing(confirmElapsedMs, CONFIRM_AUTO_CONFIRM_MS);
            
            // Check for admin badge scan during confirmation
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                recordUserActivity();
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge detected!");
                    displayAdminAccessGranted();
                    currentState = ADMIN_MODE;
                    adminMenuSelection = 0;
                    encoderPosition = M5Dial.Encoder.read();
                    displayAdminMenu(adminMenuSelection, deviceMode);
                    lastActivityTime = millis();
                    lastCardReadTime = millis();
                    break;
                }
            }
            
            // Confirm via hardware button, any touch press, or timeout.
            auto t = M5Dial.Touch.getDetail();
            bool buttonConfirm = M5Dial.BtnA.wasPressed();
            bool touchConfirm = t.wasPressed();
            bool timeoutConfirm = (confirmStateEnteredMs > 0) && ((millis() - confirmStateEnteredMs) >= CONFIRM_AUTO_CONFIRM_MS);

            if (buttonConfirm || touchConfirm || timeoutConfirm) {
                if (timeoutConfirm) {
                    Serial.println("Auto-confirm timeout reached (30s)");
                }

                if (bs2FinalConfirmPending) {
                    Serial.println("Buy Station 2 final confirm - claiming cart");
                    int itemsToClaim = min(bs2LootCount, bs2LootLimit);
                    int successCount = claimAllLoot();
                    displayBuyStation2Results(successCount, itemsToClaim);
                    playPurchaseCompleteTone();
                    delay(2500);

                    confirmStateEnteredMs = 0;
                    bs2FinalConfirmPending = false;
                    currentState = WAIT_FOR_PLAYER;
                    resetBuyStation2State();
                    displayBuyStation2Scan();
                    lastCardReadTime = millis();
                    break;
                }

                Serial.println("Confirming transaction: " + playerName + " gets " + itemName);
                displayCenteredText(TEXT_SENDING, SENDING_TEXT_COLOR, PROMPT_BG_COLOR, DEFAULT_TEXT_SIZE);
                
                // Send both IDs to server
                sendTransactionRequest(playerUuid, lootUuid);
                
                // Reset to initial state
                delay(SUCCESS_MESSAGE_DELAY);
                confirmStateEnteredMs = 0;
                bs2FinalConfirmPending = false;
                playerUuid = "";
                lootUuid = "";
                playerName = "";
                itemName = "";
                playerFaction = "";
                itemType = "";
                currentState = WAIT_FOR_PLAYER;
                displayBlackMarketScan();
                lastCardReadTime = millis();
            }
            break;
        }
        
        case ADMIN_MODE: {
            // Admin menu navigation
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                playEncoderClick();
                int selected = adminMenuSelection + (newPosition - encoderPosition) / ADMIN_ENCODER_STEP;
                encoderPosition = newPosition;
                // Wrap around menu items
                if (selected < 0) selected = ADMIN_MENU_ITEMS - 1;
                if (selected >= ADMIN_MENU_ITEMS) selected = 0;
                adminMenuSelection = selected;
                displayAdminMenu(adminMenuSelection, deviceMode);
                lastActivityTime = millis();
                lastEncoderChange = millis();
                adminEncoderStep += (newPosition - encoderPosition) / ADMIN_ENCODER_STEP;
            }
            
            // Select menu item with debounce
            if (M5Dial.BtnA.wasPressed() && (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS)) {
                lastButtonPress = millis();
                lastActivityTime = millis();
                
                switch(adminMenuSelection) {
                    case 0: { // Device Info
                        Serial.println("Admin: Device Info");
                        playAcceptTone();
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
                                    // Restore previous mode
                                    deviceMode = previousDeviceMode;
                                    if (deviceMode == MODE_BUY_STATION_2) {
                                        resetBuyStation2State();
                                        displayBuyStation2Scan();
                                    } else if (deviceMode == MODE_BACKPACK) {
                                        resetBackpackModeState();
                                        displayBackpackScan();
                                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                                        resetLootTransferState();
                                        displayLootTransferScan();
                                    } else if (deviceMode == MODE_MAP) {
                                        enterMapMode();
                                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                                        displayPhotoboothMode();
                                    } else if (deviceMode == MODE_RELAY) {
                                        displayRelayMode();
                                    } else { // Default to Buy Station 2
                                        resetBuyStation2State();
                                        displayBuyStation2Scan();
                                    }
                                    currentState = WAIT_FOR_PLAYER;
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
                    
                    case 5: // Backpack Mode
                        Serial.println("Admin: Switching to Backpack Mode");
                        deviceMode = MODE_BACKPACK;
                        // Immediately exit to Backpack mode
                        resetBackpackModeState();
                        bpWaitingForConfirm = false;
                        displayBackpackScan();
                        currentState = WAIT_FOR_PLAYER;
                        break;
                    
                    case 6: // Buy Station 2 Mode
                        Serial.println("Admin: Switching to Buy Station 2 Mode");
                        deviceMode = MODE_BUY_STATION_2;
                        // Immediately exit to Buy Station 2 mode
                        resetBuyStation2State();
                        bs2WaitingForConfirm = false;
                        displayBuyStation2Scan();
                        currentState = WAIT_FOR_PLAYER; // Set any non-admin state so BS2 logic activates
                        break;
                    
                    case 7: // Loot Transfer Mode
                        Serial.println("Admin: Switching to Loot Transfer Mode");
                        deviceMode = MODE_LOOT_TRANSFER;
                        // Immediately exit to Loot Transfer mode
                        resetLootTransferState();
                        displayLootTransferScan();
                        currentState = WAIT_FOR_PLAYER;
                        break;
                    
                    case 8: // Relay Mode
                        Serial.println("Admin: Switching to Relay Mode");
                        deviceMode = MODE_RELAY;
                        // Immediately exit to relay mode
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayRelayMode();
                        currentState = WAIT_FOR_PLAYER; // Set any non-admin state so relay logic activates
                        break;
                    
                    case 9: // Photobooth Mode
                        Serial.println("Admin: Switching to Photobooth Mode");
                        deviceMode = MODE_PHOTOBOOTH;
                        relayLastUuid = "";
                        relayLastResponse = "";
                        relayLastSuccess = false;
                        displayPhotoboothMode();
                        currentState = WAIT_FOR_PLAYER; // Set any non-admin state so photobooth logic activates
                        break;
                    
                    case 10: // Map Mode
                        Serial.println("Admin: Switching to Map Mode");
                        deviceMode = MODE_MAP;
                        // Enter map mode
                        enterMapMode();
                        currentState = WAIT_FOR_PLAYER; // Set any non-admin state so map mode can handle updates
                        break;
                    
                    case 11: // Dev Mode Toggle
                        Serial.println("Admin: Toggle Dev Mode");
                        bs2DevMode = !bs2DevMode;
                        Serial.println("Dev Mode: " + String(bs2DevMode ? "ENABLED" : "DISABLED"));
                        if (bs2DevMode) {
                            displayCenteredText("DEV MODE ENABLED", GREEN, BLACK, 2);
                            bs2LastUsedBadge = "";  // Clear cooldown list when enabling dev mode
                        } else {
                            displayCenteredText("DEV MODE DISABLED", ORANGE, BLACK, 2);
                        }
                        delay(1500);
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    
                    case 12: { // View Variables
                        Serial.println("Admin: View Variables");
                        int varScrollIndex = 0;
                        long varEncoderPos = M5Dial.Encoder.read();
                        displayVariableViewer(varScrollIndex);
                        
                        bool viewingVars = true;
                        while (viewingVars) {
                            M5Dial.update();
                            
                            // Encoder to scroll through variables
                            long newPos = M5Dial.Encoder.read();
                            if (newPos != varEncoderPos) {
                                int delta = (newPos - varEncoderPos) / 4;
                                if (delta != 0) {
                                    varScrollIndex += delta;
                                    if (varScrollIndex < 0) varScrollIndex = 0;
                                    // Max scroll limit will be enforced in display function
                                    displayVariableViewer(varScrollIndex);
                                    varEncoderPos = newPos;
                                }
                            }
                            
                            // Button press returns to menu
                            if (M5Dial.BtnA.wasPressed()) {
                                viewingVars = false;
                            }
                            
                            // Admin badge exits completely
                            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
                                String scannedUuid = sanitizeUuid(readNFCCardUID());
                                if (isAdminBadge(scannedUuid)) {
                                    Serial.println("Admin badge - exiting admin mode");
                                    // Restore previous mode
                                    deviceMode = previousDeviceMode;
                                    if (deviceMode == MODE_BUY_STATION_2) {
                                        resetBuyStation2State();
                                        displayBuyStation2Scan();
                                    } else if (deviceMode == MODE_BACKPACK) {
                                        resetBackpackModeState();
                                        displayBackpackScan();
                                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                                        resetLootTransferState();
                                        displayLootTransferScan();
                                    } else if (deviceMode == MODE_MAP) {
                                        enterMapMode();
                                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                                        displayPhotoboothMode();
                                    } else if (deviceMode == MODE_RELAY) {
                                        displayRelayMode();
                                    } else { // Default to Buy Station 2
                                        resetBuyStation2State();
                                        displayBuyStation2Scan();
                                    }
                                    currentState = WAIT_FOR_PLAYER;
                                    return;
                                }
                                delay(500);
                            }
                            delay(50);
                        }
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    }
                    
                    case 13: { // Clear Variables
                        Serial.println("Admin: Clear Variables");
                        displayCenteredText("CLEARING...", ORANGE, BLACK, 2);
                        delay(500);
                        
                        // Clear all non-critical variables
                        clearNonCriticalVariables();
                        
                        displayCenteredText("VARIABLES CLEARED", GREEN, BLACK, 2);
                        delay(1500);
                        displayAdminMenu(adminMenuSelection, deviceMode);
                        break;
                    }

                    case 14: // Exit Admin
                        Serial.println("Admin: Exiting to previous mode");
                        playAcceptTone();
                        // Restore the mode that was active before entering admin
                        deviceMode = previousDeviceMode;
                        
                        if (deviceMode == MODE_BUY_STATION_2) {
                            resetBuyStation2State();
                            displayBuyStation2Scan();
                        } else if (deviceMode == MODE_BACKPACK) {
                            resetBackpackModeState();
                            displayBackpackScan();
                        } else if (deviceMode == MODE_LOOT_TRANSFER) {
                            resetLootTransferState();
                            displayLootTransferScan();
                        } else if (deviceMode == MODE_MAP) {
                            enterMapMode();
                        } else if (deviceMode == MODE_PHOTOBOOTH) {
                            displayPhotoboothMode();
                        } else if (deviceMode == MODE_RELAY) {
                            displayRelayMode();
                        } else {
                            // Default case if something is wrong, go to Buy Station 2
                            deviceMode = MODE_BUY_STATION_2;
                            resetBuyStation2State();
                            displayBuyStation2Scan();
                        }
                        
                        currentState = WAIT_FOR_PLAYER; // Exit admin state machine
                        break;
                }
                lastCardReadTime = millis();
            }
            
            // Check for admin badge scan to exit
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    // Restore the mode that was active before entering admin
                    deviceMode = previousDeviceMode;
                    if (deviceMode == MODE_BUY_STATION_2) {
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    } else if (deviceMode == MODE_BACKPACK) {
                        resetBackpackModeState();
                        displayBackpackScan();
                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                        resetLootTransferState();
                        displayLootTransferScan();
                    } else if (deviceMode == MODE_MAP) {
                        enterMapMode();
                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                        displayPhotoboothMode();
                    } else if (deviceMode == MODE_RELAY) {
                        displayRelayMode();
                    } else { // Default to Buy Station 2
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    }
                    currentState = WAIT_FOR_PLAYER; // Exit admin state machine
                }
                lastCardReadTime = millis();
            }
            break;
        }
        
        case ADMIN_WIFI_STATUS:
            // Button returns to menu
            if (M5Dial.BtnA.wasPressed()) {
                currentState = ADMIN_MODE;
                displayAdminMenu(adminMenuSelection, deviceMode);
                encoderPosition = M5Dial.Encoder.read();
                adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                lastActivityTime = millis();
                lastCardReadTime = millis();
            }
            
            // Admin badge exits admin completely
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    // Restore previous mode
                    deviceMode = previousDeviceMode;
                    if (deviceMode == MODE_BUY_STATION_2) {
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    } else if (deviceMode == MODE_BACKPACK) {
                        resetBackpackModeState();
                        displayBackpackScan();
                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                        resetLootTransferState();
                        displayLootTransferScan();
                    } else if (deviceMode == MODE_MAP) {
                        enterMapMode();
                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                        displayPhotoboothMode();
                    } else if (deviceMode == MODE_RELAY) {
                        displayRelayMode();
                    } else { // Default to Buy Station 2
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    }
                    currentState = WAIT_FOR_PLAYER;
                }
                lastCardReadTime = millis();
            }
            break;
        
        case ADMIN_LOOT_FLIP:
            // Button returns to menu
            if (M5Dial.BtnA.wasPressed()) {
                currentState = ADMIN_MODE;
                displayAdminMenu(adminMenuSelection, deviceMode);
                encoderPosition = M5Dial.Encoder.read();
                adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                lastActivityTime = millis();
                lastCardReadTime = millis();
            }
            
            // Scan loot badge to flip lock status
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                
                // Check if admin badge - exit completely
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    // Restore previous mode
                    deviceMode = previousDeviceMode;
                    if (deviceMode == MODE_BUY_STATION_2) {
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    } else if (deviceMode == MODE_BACKPACK) {
                        resetBackpackModeState();
                        displayBackpackScan();
                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                        resetLootTransferState();
                        displayLootTransferScan();
                    } else if (deviceMode == MODE_MAP) {
                        enterMapMode();
                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                        displayPhotoboothMode();
                    } else if (deviceMode == MODE_RELAY) {
                        displayRelayMode();
                    } else { // Default to Buy Station 2
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    }
                    currentState = WAIT_FOR_PLAYER;
                } else {
                    // Regular badge - flip loot status
                    lootFlipLastUuid = scannedUuid;
                    Serial.println("Flipping loot: " + scannedUuid);
                    
                    // Show loading
                    displayCenteredText("FLIPPING...", THEME_ACCENT_COLOR, BLACK, 2);
                    
                    // Call flip function
                    lootFlipLastStatus = flipLootByUUID(scannedUuid);
                    
                    // Update display and stay in ADMIN_LOOT_FLIP mode
                    displayLootFlipMode(lootFlipLastUuid, lootFlipLastStatus);
                    // Don't change state - remain in ADMIN_LOOT_FLIP for continuous scanning
                }
                
                lastCardReadTime = millis();
            }
            break;
        
        case ADMIN_WIFI_SCAN: {
            // Handle encoder rotation to select network
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                playEncoderClick();
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
                        encoderPosition = M5Dial.Encoder.read();
                        adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
                    }
                }
                lastCardReadTime = millis();
            }
            
            // Admin badge exits admin completely
            if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial() && (millis() - lastCardReadTime > CARD_READ_DEBOUNCE)) {
                String scannedUuid = sanitizeUuid(readNFCCardUID());
                if (isAdminBadge(scannedUuid)) {
                    Serial.println("Admin badge - exiting admin mode");
                    // Restore previous mode
                    deviceMode = previousDeviceMode;
                    if (deviceMode == MODE_BUY_STATION_2) {
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    } else if (deviceMode == MODE_BACKPACK) {
                        resetBackpackModeState();
                        displayBackpackScan();
                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                        resetLootTransferState();
                        displayLootTransferScan();
                    } else if (deviceMode == MODE_MAP) {
                        enterMapMode();
                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                        displayPhotoboothMode();
                    } else if (deviceMode == MODE_RELAY) {
                        displayRelayMode();
                    } else { // Default to Buy Station 2
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    }
                    currentState = WAIT_FOR_PLAYER;
                }
                lastCardReadTime = millis();
            }
            break;
        }
        
        case ADMIN_WIFI_PASSWORD: {
            String selectedSSID = scannedSSIDs[selectedNetworkIndex];
            
            // Handle encoder rotation to scroll through characters
            long newPosition = M5Dial.Encoder.read();
            if (newPosition != encoderPosition) {
                playEncoderClick();
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
                    encoderPosition = M5Dial.Encoder.read();
                    adminEncoderStep = encoderPosition / ADMIN_ENCODER_STEP;
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
                    // Restore previous mode
                    deviceMode = previousDeviceMode;
                    if (deviceMode == MODE_BUY_STATION_2) {
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    } else if (deviceMode == MODE_BACKPACK) {
                        resetBackpackModeState();
                        displayBackpackScan();
                    } else if (deviceMode == MODE_LOOT_TRANSFER) {
                        resetLootTransferState();
                        displayLootTransferScan();
                    } else if (deviceMode == MODE_MAP) {
                        enterMapMode();
                    } else if (deviceMode == MODE_PHOTOBOOTH) {
                        displayPhotoboothMode();
                    } else if (deviceMode == MODE_RELAY) {
                        displayRelayMode();
                    } else { // Default to Buy Station 2
                        resetBuyStation2State();
                        displayBuyStation2Scan();
                    }
                    currentState = WAIT_FOR_PLAYER;
                }
                lastCardReadTime = millis();
            }
            break;
        }
    }
}

// ===================================================================
// END OF relay2_M5.ino - Main Program File
// All utility functions have been moved to separate header files
// See header files for: API calls, badge utils, error handling, WiFi mgmt
// =================================================================
