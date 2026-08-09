// HttpUtils.h - Shared HTTP helpers
#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Shared HTTPS client for keep-alive reuse
extern WiFiClientSecure sharedHttpsClient;

inline bool beginHttp(HTTPClient &http, const String &url) {
    if (url.startsWith("https://")) {
        return http.begin(sharedHttpsClient, url);
    }
    return http.begin(url);
}

inline void configureHttpClient(HTTPClient &http) {
    http.setReuse(true);
    http.addHeader("Connection", "keep-alive");
}

#endif
