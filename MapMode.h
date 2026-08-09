// MapMode.h - Map Display Mode with Mapbox Terrain
#ifndef MAPMODE_H
#define MAPMODE_H

#include <M5Dial.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <vector>
#include "config.h"
#include "HttpUtils.h"

// ============================================
// MAP CONFIGURATION
// ============================================
// Mapbox Token
const char* MAPBOX_TOKEN = "pk.eyJ1Ijoicm9uaW5raW5ldGljIiwiYSI6ImNrNTgzcG9xbDBhMTczbnBjczk0NTF5MWcifQ.LyLQcR1QxUKFJfXZNx4Utw";

// Coordinate (lat, lon) - Default location
static double MAP_LAT = 41.6573364;
static double MAP_LON = -86.7609855;

// Map settings
static int         MAP_ZOOM  = 14;              // Non-const so it can be updated
static const int   MAP_SIZE  = 240;
static const char* MAP_STYLE = "mapbox/outdoors-v12";   // Terrain / hiking / topo style

// Map mode state
bool mapModeActive = false;
bool mapLoadSuccess = false;
unsigned long mapLastActivityTime = 0;
const unsigned long MAP_MODE_TIMEOUT = 120000;  // Exit map mode after 2 minutes of inactivity

// ============================================
// HTTP DOWNLOAD FUNCTION
// ============================================
bool httpGetToBuffer(const String& url, std::vector<uint8_t>& out) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("HTTP begin failed");
        return false;
    }
    configureHttpClient(http);

    Serial.println("Downloading map...");
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("HTTP error %d\n", code);
        http.end();
        return false;
    }

    int len = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    out.clear();
    uint8_t buf[1024];

    while (http.connected()) {
        int avail = stream->available();
        if (avail) {
            int r = stream->readBytes(buf, min(avail, (int)sizeof(buf)));
            out.insert(out.end(), buf, buf + r);
        } else {
            delay(1);
        }
        if (len > 0 && out.size() >= (size_t)len) break;
    }

    http.end();
    Serial.printf("Downloaded %d bytes\n", out.size());
    return !out.empty();
}

// ============================================
// MAP DISPLAY FUNCTIONS
// ============================================
void displayMapLoading() {
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(WHITE, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("LOADING", 120, 100);
    M5Dial.Display.drawString("MAP...", 120, 130);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Please wait", 120, 160);
}

void displayMapError(const String& errorMsg) {
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(RED, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("MAP ERROR", 120, 100);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextColor(WHITE, BLACK);
    M5Dial.Display.drawString(errorMsg, 120, 140);
    M5Dial.Display.drawString("Press button", 120, 170);
    M5Dial.Display.drawString("to exit", 120, 190);
}

bool loadAndDisplayMap() {
    displayMapLoading();

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected for map download");
        displayMapError("WiFi not connected");
        return false;
    }

    // Build Mapbox Static Image URL
    String url = "https://api.mapbox.com/styles/v1/";
    url += MAP_STYLE;
    url += "/static/";
    url += String(MAP_LON, 6) + "," + String(MAP_LAT, 6) + "," + String(MAP_ZOOM);
    url += "/";
    url += String(MAP_SIZE) + "x" + String(MAP_SIZE);
    url += "?access_token=";
    url += MAPBOX_TOKEN;

    Serial.println("Map URL: " + url);

    std::vector<uint8_t> imageData;

    if (!httpGetToBuffer(url, imageData)) {
        Serial.println("Map download failed!");
        displayMapError("Download failed");
        return false;
    }

    Serial.printf("Image data si: %02X %02X %02X %02X\n", 
                  imageData[0], imageData[1], imageData[2], imageData[3]);
    
    // Check if it's PNG (89 50 4E 47) or JPG (FF D8 FF)
    bool isPNG = (imageData[0] == 0x89 && imageData[1] == 0x50 && imageData[2] == 0x4E && imageData[3] == 0x47);
    bool isJPG = (imageData[0] == 0xFF && imageData[1] == 0xD8 && imageData[2] == 0xFF);
    
    Serial.printf("Detected format: %s\n", isPNG ? "PNG" : (isJPG ? "JPG" : "UNKNOWN"));

    // Decode and display JPG from memory
    M5Dial.Display.clear();
    
    // Try to draw the image
    Serial.println("Attempting to decode image...");
    bool success = false;
    
    if (isJPG) {
        success = M5Dial.Display.drawJpg(imageData.data(), imageData.size(), 0, 0);
    } else if (isPNG) {
        success = M5Dial.Display.drawPng(imageData.data(), imageData.size(), 0, 0);
    }
    
    if (!success) {
        Serial.println("Image decode failed!");
        displayMapError("Image decode failed");
        return false;
    }
    
    Serial.println("Image decoded and displayed");
    
    // Small delay to let the image render
    delay(100);

    // Draw "You Are Here" dot
    int cx = MAP_SIZE / 2;
    int cy = MAP_SIZE / 2;

    M5Dial.Display.fillCircle(cx, cy, 8, TFT_RED);
    M5Dial.Display.drawCircle(cx, cy, 9, TFT_WHITE);

    // Optional label with background for better visibility
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Dial.Display.fillRect(cx - 35, cy - 30, 70, 15, TFT_BLACK);
    M5Dial.Display.drawString("YOU", cx, cy - 23);
    M5Dial.Display.fillRect(cx - 35, cy + 15, 70, 15, TFT_BLACK);
    M5Dial.Display.drawString("ARE HERE", cx, cy + 22);

    Serial.println("Map displayed successfully");
    return true;
}

// ============================================
// MAP MODE INITIALIZATION
// ============================================
void enterMapMode() {
    Serial.println("=== Entering Map Mode ===");
    mapModeActive = true;
    mapLastActivityTime = millis();
    
    // Load and display the map
    mapLoadSuccess = loadAndDisplayMap();
    
    if (mapLoadSuccess) {
        Serial.println("Map mode ready");
    } else {
        Serial.println("Map mode failed to load");
    }
}

void exitMapMode() {
    Serial.println("=== Exiting Map Mode ===");
    mapModeActive = false;
    mapLoadSuccess = false;
    M5Dial.Display.clear();
}

// ============================================
// MAP COORDINATE UPDATE FUNCTIONS
// ============================================
// Update coordinates (can be called externally)
void updateMapCoordinates(double lat, double lon) {
    MAP_LAT = lat;
    MAP_LON = lon;
    Serial.printf("Map coordinates updated: %.6f, %.6f\n", lat, lon);
}

// Update zoom level
void updateMapZoom(int zoom) {
    MAP_ZOOM = constrain(zoom, 1, 20);  // Mapbox supports zoom 0-22, but 1-20 is practical
    Serial.printf("Map zoom updated: %d\n", MAP_ZOOM);
}

// Lightweight status screen for map mode readiness
void displayMapModeStatus(const String& status, uint32_t color) {
    M5Dial.Display.clear();
    M5Dial.Display.setTextColor(color, BLACK);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString("MAP MODE", 120, 110);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString(status, 120, 135);
}

// Reset map mode runtime state
void resetMapModeState() {
    mapModeActive = false;
    mapLoadSuccess = false;
    mapLastActivityTime = 0;
}

// ============================================
// MAP MODE UPDATE FUNCTION
// ============================================
void updateMapMode() {
    // Handle NFC scan for location update
    if (M5Dial.Rfid.PICC_IsNewCardPresent() && M5Dial.Rfid.PICC_ReadCardSerial()) {
        String scannedUuid = sanitizeUuid(readNFCCardUID());
        Serial.println("=== Map Mode: Card scanned ===");
        Serial.println("Tag UID: " + scannedUuid);
        
        // Check if it's an admin badge first - don't process as location
        if (isAdminBadge(scannedUuid)) {
            Serial.println("Admin badge detected - handled by main loop");
            mapLastActivityTime = millis();
            return;  // Let the main loop handle admin badge
        }
        
        Serial.println("Add this to MAP_LOCATIONS in config.h");
        
        // Check if it's a location tag
        bool locationFound = false;
        for (int i = 0; i < MAP_LOCATIONS_COUNT; i++) {
            if (scannedUuid == MAP_LOCATIONS[i].tagUid) {
                Serial.println("Location tag detected: " + MAP_LOCATIONS[i].name);
                updateMapCoordinates(MAP_LOCATIONS[i].lat, MAP_LOCATIONS[i].lon);
                updateMapZoom(MAP_LOCATIONS[i].zoom);
                
                // Show location name briefly
                M5Dial.Display.fillRect(0, 0, 240, 30, TFT_BLACK);
                M5Dial.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
                M5Dial.Display.setTextDatum(middle_center);
                M5Dial.Display.setTextSize(2);
                M5Dial.Display.drawString(MAP_LOCATIONS[i].name, 120, 15);
                delay(1500);
                
                // Reload map
                mapLoadSuccess = loadAndDisplayMap();
                locationFound = true;
                break;
            }
        }
        
        if (!locationFound) {
            Serial.println("Unknown tag - not a location marker");
            // Show "Unknown Location" message
            M5Dial.Display.fillRect(0, 0, 240, 30, TFT_RED);
            M5Dial.Display.setTextColor(TFT_WHITE, TFT_RED);
            M5Dial.Display.setTextDatum(middle_center);
            M5Dial.Display.setTextSize(1);
            M5Dial.Display.drawString("Unknown Location Tag", 120, 15);
            delay(1000);
        }
        
        mapLastActivityTime = millis();
    }
    
    // Handle button press to refresh map or exit
    auto t = M5Dial.Touch.getDetail();
    if (t.wasPressed()) {
        mapLastActivityTime = millis();
        
        // Refresh the map
        Serial.println("Refreshing map...");
        mapLoadSuccess = loadAndDisplayMap();
    }
}

#endif // MAPMODE_H
