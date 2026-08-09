// WiFiManager.h - WiFi Management Functions
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "config.h"

// Forward declarations to avoid circular includes
void initializeWiFi();
void displayWiFiReconnecting();

// Network session/health state (defined in main .ino)
struct NetworkState {
    bool connected = false;
    bool tlsReady = false;
    bool wifiConnecting = false;
    bool httpInFlight = false;
    long rssi = -127;
    unsigned long lastReconnectMs = 0;
    unsigned long lastTlsReadyMs = 0;
    unsigned long lastRttMs = 0;
    unsigned long lastHeartbeatMs = 0;
    int reconnectCount = 0;
    String apiStatus = "";
};

extern NetworkState networkState;
extern unsigned long networkLastReconnectAttemptMs;
extern unsigned long networkReconnectBackoffMs;

// Markers for HTTP timing
inline unsigned long markHttpStart() {
    networkState.httpInFlight = true;
    return millis();
}

inline void markHttpEnd(unsigned long startMs) {
    networkState.httpInFlight = false;
    networkState.lastRttMs = millis() - startMs;
}

// Update network state after a successful WiFi connect
inline void updateNetworkStateOnConnect() {
    networkState.connected = true;
    networkState.wifiConnecting = false;
    networkState.lastReconnectMs = millis();
    networkState.lastTlsReadyMs = networkState.lastReconnectMs + TLS_READY_DELAY_MS;
    networkState.tlsReady = false; // will flip true after delay passes
    networkLastReconnectAttemptMs = networkState.lastReconnectMs;
    networkReconnectBackoffMs = WIFI_RECONNECT_BASE_DELAY;
    networkState.reconnectCount++;
}

// Periodic network servicing (RSSI refresh, TLS ready flip)
inline void serviceNetworkState() {
    static unsigned long lastRssiUpdateMs = 0;
    if (WiFi.status() == WL_CONNECTED) {
        networkState.connected = true;
        if (millis() >= networkState.lastTlsReadyMs) {
            networkState.tlsReady = true;
        }
        // Refresh RSSI occasionally
        if (millis() - lastRssiUpdateMs >= TELEMETRY_MIN_RSSI_UPDATE_MS) {
            networkState.rssi = WiFi.RSSI();
            lastRssiUpdateMs = millis();
        }
    } else {
        networkState.connected = false;
        networkState.tlsReady = false;
    }
}

// Attempt to maintain WiFi with basic backoff
inline bool ensureWiFiConnected() {
    serviceNetworkState();
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    unsigned long now = millis();
    if (now - networkLastReconnectAttemptMs < networkReconnectBackoffMs) {
        return false;
    }

    networkState.wifiConnecting = true;
    networkLastReconnectAttemptMs = now;
    WiFi.reconnect();
    networkReconnectBackoffMs = min(networkReconnectBackoffMs * 2, (unsigned long)WIFI_RECONNECT_MAX_DELAY);
    delay(50);
    if (WiFi.status() == WL_CONNECTED) {
        updateNetworkStateOnConnect();
        return true;
    }
    return false;
}

// Reconnect to WiFi
void reconnectWiFi() {
    Serial.println("WiFi retry requested...");
    displayWiFiReconnecting();
    WiFi.disconnect();
    delay(WIFI_RECONNECT_DELAY);
    initializeWiFi();
}

#endif // WIFI_MANAGER_H
