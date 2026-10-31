#pragma once

#include <Arduino.h>

struct RidWifiRf {
  int      noise;       // dBm, last valid noise floor
  int      rssi;        // dBm, last packet
  uint32_t pps;         // all 802.11 packets per second
  uint32_t packets;     // total packets since boot
  uint32_t mgmt;        // total management packets
  bool     noiseValid;
};

struct RidWifiRfCh {
  uint8_t  channel;
  uint32_t pps;
  int      noise;
  int      rssi;
  bool     noiseValid;
  bool     rssiValid;
};

void ridWifiStart();
void ridWifiStats();
void ridWifiUpdate();
void ridWifiGetRf(RidWifiRf *out);

void ridWifiRfScanStart();
void ridWifiRfScanStop();
void ridWifiRfScanUpdate();
void ridWifiRfScanNext();
void ridWifiRfScanGet(RidWifiRfCh *out, int *currentIdx, int *currentChannel);
