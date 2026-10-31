#include "scanner.h"
#include "config.h"
#include "display.h"
#include "buttons.h"
#include "rid_store.h"
#include "rid_ble.h"
#include "rid_wifi.h"
#include "ui.h"
#include "log.h"

static RidDrone gList[MAX_DRONES];
static int      gSelected = 0;
static bool     gDetail = false;
static int      gDetailPage = 0;
static bool     gRfMode = false;
static uint32_t gLastDraw = 0;
static uint32_t gLastExpire = 0;
static bool     gOkWasDown = false;
static uint32_t gOkDownAt = 0;

#ifdef ENABLE_DEMO
static bool     gDemoActive = false;
static uint32_t gDemoLast = 0;

static void fillDemoDrone(RidDrone &d)
{
  memset(&d, 0, sizeof(d));
  const uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
  memcpy(d.mac, mac, 6);
  d.source = RID_SOURCE_BLE;
  d.rssi = -52;

  d.hasBasicId = true;
  strncpy(d.uasId, "DEMO-1234567890", ODID_ID_SIZE);
  d.hasOperatorId = true;
  strncpy(d.opId, "OP-DEMO-001", ODID_ID_SIZE);

  d.hasLocation = true;
  d.lat = 31.230416;
  d.lon = 121.473701;
  d.altGeo = 120.0f;
  d.height = 50.0f;
  d.speedH = 8.5f;
  d.heading = 270.0f;

  d.hasSystem = true;
  d.opLat = 31.229000;
  d.opLon = 121.472000;
}

static void injectDemoDrone()
{
  RidDrone d;
  fillDemoDrone(d);
  d.lastSeen = millis();
  ridStoreUpsert(d);

  gDemoActive = true;
  gDemoLast = millis();
  logf("[CMD] demo drone injected\n");
}

static void pumpDemoDrone()
{
  if (!gDemoActive) return;
  if (millis() - gDemoLast < 1000) return;
  gDemoLast = millis();

  RidDrone d;
  fillDemoDrone(d);
  d.lastSeen = gDemoLast;
  ridStoreUpsert(d);
}
#endif

static void pollSerialCommands(Stream &s)
{
  while (s.available()) {
    int c = s.read();
#ifdef ENABLE_DEMO
    if (c == 'd' || c == 'D') {
      injectDemoDrone();
    } else
#endif
    if (c == 'c' || c == 'C') {
#ifdef ENABLE_DEMO
      gDemoActive = false;
#endif
      ridStoreClear();
      logf("[CMD] list cleared\n");
    } else if (c == 's' || c == 'S') {
      logf("[CMD] drones=%d heap=%u\n", ridStoreCount(millis()), (unsigned)ESP.getFreeHeap());
    } else if (c == 'w' || c == 'W') {
      ridWifiStats();
    } else if (c == 'r' || c == 'R') {
      gRfMode = !gRfMode;
      if (gRfMode) {
        gDetail = false;
        gDetailPage = 0;
        ridWifiRfScanStart();
      } else {
        ridWifiRfScanStop();
      }
      logf("[CMD] rf mode=%d\n", gRfMode ? 1 : 0);
    }
  }
}

void scannerSetup()
{
  ridStoreInit();
  buttonsInit();
  uiInit();
  uiDrawBoot();
  logf("[SYS] booting scanner, heap=%u\n", (unsigned)ESP.getFreeHeap());

  // Bring up BLE first, then WiFi. Enabling the BT controller after WiFi has
  // entered promiscuous mode aborts in coex_core_enable on Arduino core 2.x.
  ridBleStart();
  ridWifiStart();

  gLastDraw = millis();
  gLastExpire = gLastDraw;
  logf("[SYS] scanner running\n");
}

void scannerLoop()
{
  uint32_t now = millis();
  uint8_t pressed = buttonsPoll();
  if (pressed) {
    logf("[BTN] pressed=%02X held=%02X\n", pressed, buttonsHeld());
  }

  pollSerialCommands(Serial0);
  pollSerialCommands(Serial);
  ridWifiUpdate();
#ifdef ENABLE_DEMO
  pumpDemoDrone();
#endif

  if (gRfMode) {
    ridWifiRfScanUpdate();

    bool okDown = (buttonsHeld() & (1u << BTN_OK_BIT)) != 0;
    if (okDown && !gOkWasDown) gOkDownAt = now;
    if (!okDown && gOkWasDown) {
      uint32_t held = now - gOkDownAt;
      if (held >= 700) {
        gRfMode = false;
        ridWifiRfScanStop();
      } else {
        ridWifiRfScanNext();
      }
    }
    gOkWasDown = okDown;

    if (pressed & (1u << BTN_BACK_BIT)) {
      gRfMode = false;
      ridWifiRfScanStop();
    }

    if (pressed || (now - gLastDraw >= UI_REFRESH_MS)) {
      gLastDraw = now;
      RidWifiRfCh stats[3];
      int idx = 0;
      int ch = 0;
      ridWifiRfScanGet(stats, &idx, &ch);
      uiDrawRfScan(stats, idx, ch, now);
    }

    delay(5);
    return;
  }

  if (now - gLastExpire > 1000) {
    gLastExpire = now;
    ridStoreExpire(now);
  }

  int count = ridStoreSnapshot(gList, MAX_DRONES, now);

  if (count == 0) {
    gSelected = 0;
    gDetail = false;
    gDetailPage = 0;
  } else {
    if (gSelected >= count) gSelected = count - 1;
    if (gSelected < 0) gSelected = 0;
  }

  if (pressed & (1u << BTN_UP_BIT)) {
    if (count > 0) {
      gSelected--;
      if (gSelected < 0) gSelected = count - 1;
      gDetailPage = 0;
    }
  }

  if (pressed & (1u << BTN_DOWN_BIT)) {
    if (count > 0) {
      gSelected++;
      if (gSelected >= count) gSelected = 0;
      gDetailPage = 0;
    }
  }

  // K3 (OK): short press = open detail / next drone.
  // Hold for 700 ms = back. This keeps the device usable with 3 buttons.
  bool okDown = (buttonsHeld() & (1u << BTN_OK_BIT)) != 0;
  if (okDown && !gOkWasDown) {
    gOkDownAt = now;
  }
  if (!okDown && gOkWasDown) {
    uint32_t held = now - gOkDownAt;
    logf("[OK] held=%lu ms detail=%d page=%d count=%d\n",
         (unsigned long)held, gDetail ? 1 : 0, gDetailPage, count);
    if (held >= 700) {
      if (gDetail) {
        gDetail = false;
        gDetailPage = 0;
      } else {
        gRfMode = true;
        ridWifiRfScanStart();
      }
    } else if (count > 0) {
      if (!gDetail) {
        gDetail = true;
        gDetailPage = 0;
      } else {
        gDetailPage = (gDetailPage + 1) % 3;
      }
    }
  }
  gOkWasDown = okDown;

  if (pressed & (1u << BTN_BACK_BIT)) {
    gDetail = false;
    gDetailPage = 0;
  }

  if (pressed || (now - gLastDraw >= UI_REFRESH_MS)) {
    gLastDraw = now;

    if (gDetail && count > 0) {
      uiDrawDetail(gList[gSelected], gSelected, count, now, gDetailPage);
    } else {
      uiDrawList(gList, count, gSelected, now);
    }
  }

  delay(5);
}
