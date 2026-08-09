// initSequence_M5.h
#ifndef INIT_SEQUENCE_M5_H
#define INIT_SEQUENCE_M5_H

#include <WiFi.h>
#include <Arduino.h>
#include <M5Dial.h>
#include "config.h"
#include "M5DialControl.h"
#include "WiFiManager.h"
#include "secrets.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LinkedList.h>


String mac = "";
String ip = "";
String postData;
int httpCode;
String payload;
DynamicJsonDocument doc(2048);
String mode = "";
String currentMinigameId = "";
String currentMinigameType = "";
String owningFactionId = "";
LinkedList<Faction*> factionsList;
TaskHandle_t activeMinigameTaskHandle = NULL;

void initializeNFC() {
    Serial.println("Initializing M5 Dial built-in NFC...");
    
    // Show animation during NFC init
    for (int frame = 0; frame < 20; frame++) {
        drawAnimationFrame(frame, TEXT_INIT_NFC);
        delay(STARTUP_ANIMATION_SPEED);
    }
    
    M5Dial.Rfid.PCD_Init();
    Serial.println("NFC reader initialized.");
}

// Read NFC card UID and return as String
String readNFCCardUID() {
    String uidString = "";
    for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) {
        if (M5Dial.Rfid.uid.uidByte[i] < 0x10) {
            uidString += "0";
        }
        uidString += String(M5Dial.Rfid.uid.uidByte[i], HEX);
        if (i != M5Dial.Rfid.uid.size - 1) {
            uidString += " ";
        }
    }
    M5Dial.Rfid.PICC_HaltA();
    M5Dial.Rfid.PCD_StopCrypto1();
    playScanTone();
    return uidString;
}

void initializeM5Display() {
    Serial.println("Initializing M5 Dial Display");
    initM5Display();
    // Boot heartbeat to confirm display power
    M5Dial.Display.fillScreen(BLUE);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString("BOOT", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
    delay(300);
    M5Dial.Display.fillScreen(BLACK);
    // Animation will be shown during other init steps
}

void initializeWiFi() {
    Serial.println("Beginning Wifi Connection...");
    
    // Force STA mode and enable sleep to reduce power/heat
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);

    WiFi.begin(ssid, password);
    int attempts = 0;
    int animFrame = 0;
    int delayAccumulator = 0;
    
    while (WiFi.status() != WL_CONNECTED) {
        // Draw animation frame
        drawAnimationFrame(animFrame, TEXT_CONNECTING_WIFI);
        animFrame++;
        
        delay(STARTUP_ANIMATION_SPEED);
        delayAccumulator += STARTUP_ANIMATION_SPEED;
        
        // Only increment attempts every 500ms for proper WiFi polling
        if (delayAccumulator >= WIFI_CONNECT_RETRY_DELAY) {
            attempts++;
            delayAccumulator = 0;
        }
        
        if (attempts > WIFI_CONNECT_ATTEMPTS) {
            // FAILED - Show error and wait for WiFi reset badge
            Serial.println("WiFi connection failed - waiting for WiFi reset badge");
            
            while (true) {
                // Display error message
                M5Dial.Display.fillScreen(RED);
                M5Dial.Display.setTextColor(WHITE);
                M5Dial.Display.setTextSize(2);
                M5Dial.Display.setTextDatum(middle_center);
                M5Dial.Display.drawString(TEXT_WIFI_FAILED, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 20);
                M5Dial.Display.setTextSize(1);
                M5Dial.Display.drawString("SCAN WIFI", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 20);
                M5Dial.Display.drawString("RESET BADGE", M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 40);
                
                // Check for NFC card
                M5Dial.update();
                if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
                    String scannedUid = "";
                    for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) {
                        if (M5Dial.Rfid.uid.uidByte[i] < 0x10) {
                            scannedUid += "0";
                        }
                        scannedUid += String(M5Dial.Rfid.uid.uidByte[i], HEX);
                    }
                    scannedUid.toUpperCase();
                    M5Dial.Rfid.PICC_HaltA();
                    M5Dial.Rfid.PCD_StopCrypto1();
                    
                    Serial.println("Badge scanned during WiFi error: " + scannedUid);
                    
                    // Check if it's a WiFi retry tag
                    bool isRetryTag = false;
                    for (int i = 0; i < WIFI_RETRY_TAGS_COUNT; i++) {
                        if (scannedUid == WIFI_RETRY_TAGS[i]) {
                            isRetryTag = true;
                            break;
                        }
                    }
                    
                    if (isRetryTag) {
                        Serial.println("WiFi reset badge detected - retrying connection");
                        M5Dial.Display.fillScreen(BLACK);
                        M5Dial.Display.setTextColor(CYAN);
                        M5Dial.Display.setTextSize(1);
                        M5Dial.Display.setTextDatum(middle_center);
                        M5Dial.Display.drawString(TEXT_WIFI_RECONNECTING, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
                        delay(1000);
                        
                        // Restart WiFi connection
                        WiFi.disconnect();
                        delay(500);
                        WiFi.begin(ssid, password);
                        attempts = 0;
                        animFrame = 0;
                        break; // Exit the error loop and retry connection
                    }
                }
                
                delay(100); // Small delay to prevent tight loop
            }
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi");  
        mac = macAddress;
        Serial.println("MAC: " + mac);
        Serial.println("Serial Number: " + serialNum);
        ip = WiFi.localIP().toString();
        Serial.println("IP: " + ip);  
        WiFi.setSleep(false);
        WiFi.setAutoReconnect(true);
        WiFi.setTxPower(WIFI_POWER_19_5dBm);

        updateNetworkStateOnConnect();
        networkState.rssi = WiFi.RSSI();
        
        // Success - no blink, just continue
        Serial.println("WiFi initialization complete");
    }
}

void handleDeviceInitError() {
    M5Dial.Display.fillScreen(RED);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.drawString(TEXT_ERROR, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2);
}

void handleInitializationSuccess() {
    // Success - no blinks, just ready
    Serial.println("Initialization complete - ready for use");
}

#endif // INIT_SEQUENCE_M5_H
