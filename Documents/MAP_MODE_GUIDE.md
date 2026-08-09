# Map Mode Implementation

## Overview
Map Mode has been successfully added to your M5 Dial Buy Station project. It uses the Mapbox API to display terrain maps with a "You Are Here" indicator.

## Features
- **Real-time map download** from Mapbox using terrain/outdoors style
- **GPS location marker** showing current position
- **Touch to refresh** - tap the screen to reload the map
- **Auto-timeout** after 2 minutes of inactivity (returns to Buy Station 2 mode)
- **Admin badge exit** - scan admin badge to return to admin menu

## How to Access Map Mode

1. **Scan an admin badge** to enter admin mode
2. **Rotate the dial** to navigate to "Map Mode" (menu item #9)
3. **Press the button** to select and enter Map Mode
4. The device will:
   - Show "LOADING MAP..." screen
   - Download the map from Mapbox
   - Display the terrain map with "YOU ARE HERE" marker

## Configuration

Edit the following settings in `MapMode.h`:

```cpp
// GPS Coordinates (change to your location)
static double MAP_LAT = 41.6573364;  // Latitude
static double MAP_LON = -86.7609855; // Longitude

// Map zoom level (1-20, higher = more zoomed in)
static const int MAP_ZOOM = 14;

// Map style (see Mapbox styles)
static const char* MAP_STYLE = "mapbox/outdoors-v12";
```

### Available Map Styles
- `mapbox/streets-v12` - Standard street map
- `mapbox/outdoors-v12` - Terrain/hiking map (default)
- `mapbox/satellite-v9` - Satellite imagery
- `mapbox/satellite-streets-v12` - Satellite with street labels
- `mapbox/dark-v11` - Dark theme
- `mapbox/light-v11` - Light theme

## Usage

### In Map Mode:
- **Tap screen**: Refresh/reload the map
- **Scan admin badge**: Exit to admin menu
- **Wait 2 minutes**: Auto-return to Buy Station 2 mode

### Programmatically Update Location:
You can update the map coordinates from code:

```cpp
// Update to a new location
updateMapCoordinates(34.0522, -118.2437);  // Los Angeles

// Change zoom level
updateMapZoom(16);  // Zoom in closer
```

## API Token
The Mapbox token is already configured in `MapMode.h`:
```cpp
const char* MAPBOX_TOKEN = "pk.eyJ1Ijoicm9uaW5raW5ldGljIiwiYSI6ImNrNTgzcG9xbDBhMTczbnBjczk0NTF5MWcifQ.LyLQcR1QxUKFJfXZNx4Utw";
```

**Note:** This is a public token from your example code. For production use, you should:
1. Create your own Mapbox account at https://www.mapbox.com/
2. Get your own API token
3. Replace the token in `MapMode.h`

## Files Modified

1. **config.h**
   - Added `MODE_MAP` to the `OperationalMode` enum
   - Updated admin menu item count to 12

2. **buyStation_M5v2.ino**
   - Added `#include "MapMode.h"`
   - Added Map Mode case in admin menu (case 9)
   - Added Map Mode handler in main loop
   - Updated exit logic to handle Map Mode

3. **M5DialControl_ADMIN_DISPLAYS.h**
   - Added "Map Mode" to menu names
   - Updated mode indicator to show "MAP"

4. **MapMode.h** (NEW FILE)
   - Contains all map functionality
   - Handles HTTP download from Mapbox
   - Manages map display and refresh

## Troubleshooting

### Map doesn't load
- **Check WiFi connection** - Map Mode requires active WiFi
- **Verify Mapbox token** - The token must be valid
- **Check Serial Monitor** - Look for HTTP error codes

### "Download failed" error
- WiFi may be disconnected or unstable
- Mapbox API might be temporarily unavailable
- Token might be invalid or rate-limited

### Map is blank or corrupted
- Download may have been interrupted
- Try tapping to refresh the map
- Exit and re-enter Map Mode

## Future Enhancements

You could extend Map Mode with:
- **GPS module integration** - Auto-update location
- **Multiple location markers** - Show player/loot positions
- **Map pan/zoom** - Use encoder to navigate map
- **Location history** - Trail of previous positions
- **Waypoint markers** - Mark important locations
- **Custom overlays** - Draw game-specific information

## Example: Dynamic Location Updates

If you integrate a GPS module, you could update the map periodically:

```cpp
// In your main loop or GPS callback:
if (gpsDataAvailable) {
    double currentLat = gps.getLatitude();
    double currentLon = gps.getLongitude();
    
    // Update map coordinates
    updateMapCoordinates(currentLat, currentLon);
    
    // Reload the map if in map mode
    if (deviceMode == MODE_MAP && mapModeActive) {
        loadAndDisplayMap();
    }
}
```

## Support

The implementation follows your existing mode patterns (Backpack, Buy Station 2, Loot Transfer, Relay) so it integrates seamlessly with your current architecture.
