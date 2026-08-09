// Badge.h
#ifndef Badge_h
#define Badge_h

#include <Arduino.h>

struct Badge {
  String uuid;
  String markerCode;
  String player; // Optional, based on your requirements
};

#endif