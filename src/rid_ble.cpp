#include "rid_ble.h"
#include "rid_parse.h"
#include "rid_store.h"
#include "log.h"
#include "config.h"

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

static BLEScan *gScan = nullptr;
static ODID_UAS_Data gUas;

class RidBleCallbacks : public BLEAdvertisedDeviceCallbacks
{
public:
  void onResult(BLEAdvertisedDevice device) override
  {
    const uint8_t *payload = device.getPayload();
    const int len = device.getPayloadLength();
    if (!payload || len < 4) return;

    const uint8_t *msg = nullptr;
    size_t msgLen = 0;

    // Walk the AD structures instead of assuming the service data is first.
    int i = 0;
    while (i + 1 < len) {
      const uint8_t adLen = payload[i];
      if (adLen == 0) break;
      if (i + 1 + adLen > len) break;

      const uint8_t type = payload[i + 1];
      const uint8_t *ad = payload + i + 2;
      const size_t adDataLen = adLen - 1;

      // 0x16 = Service Data, 0xFFFA = ASTM Remote ID, 0x0D = app code.
      if (type == 0x16 && adDataLen >= 4 &&
          ad[0] == 0xFA && ad[1] == 0xFF && ad[2] == 0x0D) {
        msg = ad + 4;              // skip UUID(2) + app code(1) + counter(1)
        msgLen = adDataLen - 4;
        break;
      }

      i += adLen + 1;
    }

    if (!msg || msgLen < 1) return;

    BLEAddress addr = device.getAddress();
    const uint8_t (*mac)[6] = addr.getNative();
    if (!mac) return;

    RidDrone d;
    memset(&d, 0, sizeof(d));
    memcpy(d.mac, *mac, 6);
    d.source = RID_SOURCE_BLE;
    d.rssi = device.getRSSI();
    d.lastSeen = millis();

    if (ridParseOdid(msg, msgLen, &gUas, &d)) {
      ridStoreUpsert(d);

      static uint32_t lastLog = 0;
      if (millis() - lastLog > 1000) {
        lastLog = millis();
        logf("[BLE] %02X:%02X:%02X:%02X:%02X:%02X rssi=%d id=%s\n",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5],
             d.rssi, d.hasBasicId ? d.uasId : "?");
      }
    }
  }
};

static void bleTask(void *)
{
  for (;;) {
    if (gScan) {
      gScan->start(1, false);
      gScan->clearResults();
    }
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

void ridBleStart()
{
  BLEDevice::init("RID-SCANNER");

  gScan = BLEDevice::getScan();
  gScan->setAdvertisedDeviceCallbacks(new RidBleCallbacks(), true);
  // Passive scan: listen to advertisements only, never send SCAN_REQ.
  gScan->setActiveScan(false);
  gScan->setInterval(100);
  gScan->setWindow(100);

  xTaskCreatePinnedToCore(bleTask, "rid_ble", 8192, nullptr, 1, nullptr, 1);
  logf("[BLE] scanner started\n");
}
