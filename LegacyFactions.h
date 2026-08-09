// LegacyFactions.h - configured fallback faction ids and display names
#ifndef LEGACY_FACTIONS_H
#define LEGACY_FACTIONS_H

#include <Arduino.h>
#include "secrets.h"
#include "FactionCache.h"

inline String normalizeFactionLookupValue(String value) {
    value.trim();
    value.toLowerCase();
    return value;
}

inline bool legacyFactionAliasesContain(const char* aliases, const String& normalizedName) {
    if (!aliases || !normalizedName.length()) return false;

    String list = String(aliases);
    int start = 0;
    while (start <= list.length()) {
        int end = list.indexOf('|', start);
        if (end < 0) end = list.length();
        String alias = list.substring(start, end);
        alias.trim();
        alias.toLowerCase();
        if (alias == normalizedName) return true;
        start = end + 1;
    }

    return false;
}

inline String legacyFactionNameById(const String& id) {
    if (!id.length()) return "";
    for (int i = 0; i < FACTION_FALLBACK_COUNT; i++) {
        if (id == String(FACTION_FALLBACK_IDS[i])) {
            return String(FACTION_FALLBACK_NAMES[i]);
        }
    }
    return "";
}

inline String legacyFactionIdByName(String name) {
    String normalizedName = normalizeFactionLookupValue(name);
    if (!normalizedName.length()) return "";

    for (int i = 0; i < FACTION_FALLBACK_COUNT; i++) {
        String configuredName = normalizeFactionLookupValue(String(FACTION_FALLBACK_NAMES[i]));
        if (configuredName == normalizedName || legacyFactionAliasesContain(FACTION_FALLBACK_ALIASES[i], normalizedName)) {
            return String(FACTION_FALLBACK_IDS[i]);
        }
    }

    return "";
}

inline String displayFactionNameById(const String& id) {
    String cachedName = cachedFactionNameById(id);
    if (cachedName.length()) return cachedName;

    String legacyName = legacyFactionNameById(id);
    if (legacyName.length()) return legacyName;

    return id;
}

inline String resolveFactionIdByName(String name) {
    String cachedId = cachedFactionIdByName(name);
    if (cachedId.length()) return cachedId;
    return legacyFactionIdByName(name);
}

#endif // LEGACY_FACTIONS_H
