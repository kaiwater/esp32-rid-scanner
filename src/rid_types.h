#pragma once

#include <Arduino.h>
#include "opendroneid.h"

#define RID_SOURCE_BLE  1
#define RID_SOURCE_WIFI 2

struct RidDrone {
  bool     used;
  uint8_t  mac[6];
  uint8_t  source;       // RID_SOURCE_BLE / RID_SOURCE_WIFI
  int      rssi;
  uint32_t lastSeen;

  bool     hasBasicId;
  char     uasId[ODID_ID_SIZE + 1];

  bool     hasOperatorId;
  char     opId[ODID_ID_SIZE + 1];

  bool     hasLocation;
  double   lat;
  double   lon;
  float    altGeo;
  float    height;
  float    speedH;
  float    heading;

  bool     hasSystem;
  double   opLat;
  double   opLon;
};
