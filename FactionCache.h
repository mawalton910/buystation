// FactionCache.h - Boot-time game faction cache
#ifndef FACTION_CACHE_H
#define FACTION_CACHE_H

#include <Arduino.h>

#define GAME_FACTION_CACHE_MAX 16

struct CachedGameFaction {
    String id;
    String name;
    String colorCode;
    String keywords;
};

CachedGameFaction gameFactionCache[GAME_FACTION_CACHE_MAX];
int gameFactionCount = 0;
String gameFactionCacheStatus = "not loaded";
String gameFactionCacheWidgetId = "";
String gameFactionCachePoiId = "";
String gameFactionCacheUpdatedAt = "";

inline void clearGameFactionCache(const String& status = "cleared") {
    for (int i = 0; i < GAME_FACTION_CACHE_MAX; i++) {
        gameFactionCache[i].id = "";
        gameFactionCache[i].name = "";
        gameFactionCache[i].colorCode = "";
        gameFactionCache[i].keywords = "";
    }
    gameFactionCount = 0;
    gameFactionCacheStatus = status;
    gameFactionCacheWidgetId = "";
    gameFactionCachePoiId = "";
    gameFactionCacheUpdatedAt = "";
}

inline bool addGameFactionToCache(const String& id, const String& name, const String& colorCode, const String& keywords) {
    if (!id.length() || gameFactionCount >= GAME_FACTION_CACHE_MAX) {
        return false;
    }

    gameFactionCache[gameFactionCount].id = id;
    gameFactionCache[gameFactionCount].name = name.length() ? name : id;
    gameFactionCache[gameFactionCount].colorCode = colorCode;
    gameFactionCache[gameFactionCount].keywords = keywords;
    gameFactionCount++;
    return true;
}

inline String cachedFactionNameById(const String& id) {
    if (!id.length()) return "";
    for (int i = 0; i < gameFactionCount; i++) {
        if (gameFactionCache[i].id == id) {
            return gameFactionCache[i].name;
        }
    }
    return "";
}

inline String cachedFactionIdByName(String name) {
    name.trim();
    name.toLowerCase();
    if (!name.length()) return "";

    for (int i = 0; i < gameFactionCount; i++) {
        String cachedName = gameFactionCache[i].name;
        cachedName.trim();
        cachedName.toLowerCase();
        if (cachedName == name) {
            return gameFactionCache[i].id;
        }
    }
    return "";
}

#endif // FACTION_CACHE_H
