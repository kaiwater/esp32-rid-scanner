#include "rid_store.h"
#include "config.h"

static RidDrone gTable[MAX_DRONES];
static portMUX_TYPE gMux = portMUX_INITIALIZER_UNLOCKED;

static bool macEqual(const uint8_t *a, const uint8_t *b)
{
  return memcmp(a, b, 6) == 0;
}

static bool macIsZero(const uint8_t *a)
{
  for (int i = 0; i < 6; i++) if (a[i]) return false;
  return true;
}

void ridStoreInit()
{
  portENTER_CRITICAL(&gMux);
  memset(gTable, 0, sizeof(gTable));
  portEXIT_CRITICAL(&gMux);
}

void ridStoreUpsert(const RidDrone &incoming)
{
  if (macIsZero(incoming.mac)) return;

  portENTER_CRITICAL(&gMux);

  RidDrone *slot = nullptr;
  RidDrone *oldest = &gTable[0];

  for (int i = 0; i < MAX_DRONES; i++) {
    if (gTable[i].used && macEqual(gTable[i].mac, incoming.mac)) {
      slot = &gTable[i];
      break;
    }
    if (!gTable[i].used && slot == nullptr) {
      slot = &gTable[i];
    }
    if (gTable[i].used &&
        (int32_t)(gTable[i].lastSeen - oldest->lastSeen) < 0) {
      oldest = &gTable[i];
    }
  }

  if (slot == nullptr) {
    slot = oldest;
  }

  if (!slot->used || !macEqual(slot->mac, incoming.mac)) {
    memset(slot, 0, sizeof(*slot));
    slot->used = true;
    memcpy(slot->mac, incoming.mac, 6);
  }

  slot->lastSeen = incoming.lastSeen;
  slot->source = incoming.source;

  if (slot->rssi == 0) slot->rssi = incoming.rssi;
  else slot->rssi = (slot->rssi * 3 + incoming.rssi) / 4;

  if (incoming.hasBasicId) {
    slot->hasBasicId = true;
    memset(slot->uasId, 0, sizeof(slot->uasId));
    strncpy(slot->uasId, incoming.uasId, ODID_ID_SIZE);
  }

  if (incoming.hasOperatorId) {
    slot->hasOperatorId = true;
    memset(slot->opId, 0, sizeof(slot->opId));
    strncpy(slot->opId, incoming.opId, ODID_ID_SIZE);
  }

  if (incoming.hasLocation) {
    slot->hasLocation = true;
    slot->lat      = incoming.lat;
    slot->lon      = incoming.lon;
    slot->altGeo   = incoming.altGeo;
    slot->height   = incoming.height;
    slot->speedH   = incoming.speedH;
    slot->heading  = incoming.heading;
  }

  if (incoming.hasSystem) {
    slot->hasSystem = true;
    slot->opLat = incoming.opLat;
    slot->opLon = incoming.opLon;
  }

  portEXIT_CRITICAL(&gMux);
}

void ridStoreExpire(uint32_t now)
{
  portENTER_CRITICAL(&gMux);
  for (int i = 0; i < MAX_DRONES; i++) {
    if (gTable[i].used && (uint32_t)(now - gTable[i].lastSeen) > DRONE_TIMEOUT_MS) {
      gTable[i].used = false;
    }
  }
  portEXIT_CRITICAL(&gMux);
}

void ridStoreClear()
{
  portENTER_CRITICAL(&gMux);
  memset(gTable, 0, sizeof(gTable));
  portEXIT_CRITICAL(&gMux);
}

int ridStoreSnapshot(RidDrone *out, int maxCount, uint32_t now)
{
  int count = 0;

  portENTER_CRITICAL(&gMux);
  for (int i = 0; i < MAX_DRONES && count < maxCount; i++) {
    if (!gTable[i].used) continue;
    if ((uint32_t)(now - gTable[i].lastSeen) > DRONE_TIMEOUT_MS) continue;
    out[count++] = gTable[i];
  }
  portEXIT_CRITICAL(&gMux);

  // Simple insertion sort: strongest / freshest first.
  for (int i = 1; i < count; i++) {
    RidDrone key = out[i];
    int j = i - 1;
    while (j >= 0 && (int32_t)(out[j].lastSeen - key.lastSeen) < 0) {
      out[j + 1] = out[j];
      j--;
    }
    out[j + 1] = key;
  }

  return count;
}

int ridStoreCount(uint32_t now)
{
  int count = 0;
  portENTER_CRITICAL(&gMux);
  for (int i = 0; i < MAX_DRONES; i++) {
    if (gTable[i].used && (uint32_t)(now - gTable[i].lastSeen) <= DRONE_TIMEOUT_MS) {
      count++;
    }
  }
  portEXIT_CRITICAL(&gMux);
  return count;
}
