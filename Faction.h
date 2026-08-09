// Faction.h
#ifndef Faction_h
#define Faction_h

#include <Arduino.h>
#include <LinkedList.h> // Ensure you have this library or an equivalent
#include "Badge.h"      // Include the Badge definition

struct Faction {
  String id;
  String colorCode;
  LinkedList<Badge*> badges; // List of badges associated with this faction

  // Constructor to ensure badges list is initialized
  Faction() : badges(LinkedList<Badge*>()) {}
  
  // Destructor to properly clean up badges list
  ~Faction() {
    for (int i = 0; i < badges.size(); i++) {
      delete badges.get(i); // Free each dynamically allocated Badge
    }
  }
};

#endif