#include "rid_wifi.h"
#include "rid_parse.h"
#include "rid_store.h"
#include "log.h"
#include "config.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include "odid_wifi.h"

static ODID_UAS_Data gUas;
static volatile uint32_t gMgmtCount = 0;
static volatile uint32_t gBeaconCount = 0;
static volatile uint32_t gVendorCount = 0;
static volatile uint32_t gOdidCount = 0;
static volatile uint32_t gPktCount = 0;
static volatile uint32_t gPps = 0;
static volatile int      gNoise = 0;
static volatile int      gRssi = 0;
static volatile bool     gNoiseValid = false;

static const uint8_t kRfChannels[3] = {1, 6, 11};
static volatile uint32_t gChPkt[3] = {0};
static volatile int      gChNoise[3] = {0};
static volatile int      gChRssi[3] = {0};
static volatile bool     gChNoiseValid[3] = {false};
static volatile bool     gChRssiValid[3] = {false};
static uint32_t          gChLastCount[3] = {0};
static volatile uint32_t gChPps[3] = {0};
static volatile bool     gRfScan = false;
static volatile int      gRfIdx = 0;
static uint32_t          gRfLastSwitch = 0;

static int chIndex(uint8_t ch)
{
  if (ch == 1) return 0;
  if (ch == 6) return 1;
  if (ch == 11) return 2;
  return -1;
}

void ridWifiRfScanStart()
{
  for (int i = 0; i < 3; i++) {
    gChPkt[i] = 0;
    gChPps[i] = 0;
    gChNoise[i] = 0;
    gChRssi[i] = 0;
    gChNoiseValid[i] = false;
    gChRssiValid[i] = false;
    gChLastCount[i] = 0;
  }

  gRfIdx = 0;
  gRfScan = true;
  gRfLastSwitch = millis();
  esp_wifi_set_channel(kRfChannels[0], WIFI_SECOND_CHAN_NONE);
  logf("[RF] scan start\n");
}

void ridWifiRfScanStop()
{
  gRfScan = false;
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  logf("[RF] scan stop, back to ch%d\n", WIFI_CHANNEL);
}

void ridWifiRfScanNext()
{
  if (!gRfScan) return;

  uint32_t now = millis();
  int i = gRfIdx;
  uint32_t count = gChPkt[i];
  uint32_t dt = now - gRfLastSwitch;
  if (dt < 200) dt = 200;

  gChPps[i] = (uint32_t)((uint64_t)(count - gChLastCount[i]) * 1000 / dt);
  gChLastCount[i] = count;

  gRfIdx = (gRfIdx + 1) % 3;
  esp_wifi_set_channel(kRfChannels[gRfIdx], WIFI_SECOND_CHAN_NONE);
  gRfLastSwitch = now;
}

void ridWifiRfScanUpdate()
{
  if (gRfScan && (millis() - gRfLastSwitch >= 1000)) {
    ridWifiRfScanNext();
  }
}

void ridWifiRfScanGet(RidWifiRfCh *out, int *currentIdx, int *currentChannel)
{
  if (!out) return;
  for (int i = 0; i < 3; i++) {
    out[i].channel    = kRfChannels[i];
    out[i].pps        = gChPps[i];
    out[i].noise      = gChNoise[i];
    out[i].rssi       = gChRssi[i];
    out[i].noiseValid = gChNoiseValid[i];
    out[i].rssiValid  = gChRssiValid[i];
  }
  if (currentIdx) *currentIdx = gRfIdx;
  if (currentChannel) *currentChannel = kRfChannels[gRfIdx];
}

void ridWifiUpdate()
{
  static uint32_t lastMs = 0;
  static uint32_t lastCount = 0;

  uint32_t now = millis();
  if (now - lastMs >= 1000) {
    uint32_t count = gPktCount;
    gPps = (uint32_t)((uint64_t)(count - lastCount) * 1000 / (now - lastMs));
    lastCount = count;
    lastMs = now;
  }
}

void ridWifiGetRf(RidWifiRf *out)
{
  if (!out) return;
  out->noise      = gNoise;
  out->rssi       = gRssi;
  out->pps        = gPps;
  out->packets    = gPktCount;
  out->mgmt       = gMgmtCount;
  out->noiseValid = gNoiseValid;
}

void ridWifiStats()
{
  logf("[WiFi] ch=%d pps=%lu noise=%d rssi=%d mgmt=%lu beacon=%lu vendor=%lu odid=%lu\n",
       WIFI_CHANNEL,
       (unsigned long)gPps,
       gNoiseValid ? gNoise : 0,
       gRssi,
       (unsigned long)gMgmtCount,
       (unsigned long)gBeaconCount,
       (unsigned long)gVendorCount,
       (unsigned long)gOdidCount);

  logf("[RF] ch1 pps=%lu nf=%d rssi=%d | ch6 pps=%lu nf=%d rssi=%d | ch11 pps=%lu nf=%d rssi=%d\n",
       (unsigned long)gChPps[0], gChNoise[0], gChRssi[0],
       (unsigned long)gChPps[1], gChNoise[1], gChRssi[1],
       (unsigned long)gChPps[2], gChNoise[2], gChRssi[2]);
}

static void storeDecoded(const uint8_t *mac, int rssi, const ODID_UAS_Data *uas)
{
  RidDrone d;
  memset(&d, 0, sizeof(d));
  memcpy(d.mac, mac, 6);
  d.source = RID_SOURCE_WIFI;
  d.rssi = rssi;
  d.lastSeen = millis();

  if (ridFillFromUas((ODID_UAS_Data *)uas, &d)) {
    ridStoreUpsert(d);

    static uint32_t lastLog = 0;
    if (millis() - lastLog > 1000) {
      lastLog = millis();
      logf("[WiFi] %02X:%02X:%02X:%02X:%02X:%02X rssi=%d id=%s\n",
           d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5],
           d.rssi, d.hasBasicId ? d.uasId : "?");
    }
  }
}

static void wifiSniffer(void *buf, wifi_promiscuous_pkt_type_t type)
{
  const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

  gPktCount++;
  gRssi = pkt->rx_ctrl.rssi;
  int8_t nf = pkt->rx_ctrl.noise_floor;
  if (nf < 0 && nf > -120) {
    gNoise = nf;
    gNoiseValid = true;
  }

  int chIdx = chIndex((uint8_t)pkt->rx_ctrl.channel);
  if (chIdx >= 0) {
    gChPkt[chIdx]++;
    gChRssi[chIdx] = pkt->rx_ctrl.rssi;
    gChRssiValid[chIdx] = true;
    if (nf < 0 && nf > -120) {
      gChNoise[chIdx] = nf;
      gChNoiseValid[chIdx] = true;
    }
  }

  if (type != WIFI_PKT_MGMT) return;
  gMgmtCount++;

  const uint8_t *p = pkt->payload;
  const int len = pkt->rx_ctrl.sig_len;
  if (!p || len < 24) return;

  // Frame control: type 0 = management.
  if ((p[0] & 0x0C) != 0) return;
  const uint8_t subtype = (p[0] >> 4) & 0x0F;

  // ---- WiFi NAN action frame (ASTM Remote ID over NAN) ----
  static const uint8_t nanDest[6] = {0x51, 0x6F, 0x9A, 0x01, 0x00, 0x00};
  if (subtype == 0x0D && memcmp(p + 4, nanDest, 6) == 0) {
    char sa[6];
    if (odid_wifi_receive_message_pack_nan_action_frame(
            &gUas, sa, (uint8_t *)p, len) == 0) {
      storeDecoded((const uint8_t *)sa, pkt->rx_ctrl.rssi, &gUas);
    }
    return;
  }

  // ---- WiFi Beacon / Probe Response with vendor IE ----
  if (subtype != 0x08 && subtype != 0x05) return;
  gBeaconCount++;

  const int fixed = 24 + 12;   // 802.11 header + beacon fixed fields
  if (len < fixed + 2) return;

  int off = fixed;
  while (off + 2 <= len) {
    const uint8_t id = p[off];
    const uint8_t ieLen = p[off + 1];
    if (off + 2 + ieLen > len) break;

    if (id == 0xDD && ieLen >= 6) {
      const uint8_t *v = p + off + 2;
      const bool asdStan = (v[0] == 0xFA && v[1] == 0x0B && v[2] == 0xBC);
      const bool oldOui  = (v[0] == 0x90 && v[1] == 0x3A && v[2] == 0xE6);
      if ((asdStan || oldOui) && v[3] == 0x0D) {
        gVendorCount++;
        // v[4] = message counter, v[5..] = ODID message pack
        RidDrone d;
        memset(&d, 0, sizeof(d));
        memcpy(d.mac, p + 10, 6);
        d.source = RID_SOURCE_WIFI;
        d.rssi = pkt->rx_ctrl.rssi;
        d.lastSeen = millis();

        if (ridParseOdid(v + 5, ieLen - 5, &gUas, &d)) {
          gOdidCount++;
          ridStoreUpsert(d);

          static uint32_t lastLog = 0;
          if (millis() - lastLog > 1000) {
            lastLog = millis();
            logf("[WiFi] %02X:%02X:%02X:%02X:%02X:%02X rssi=%d id=%s\n",
                 d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5],
                 d.rssi, d.hasBasicId ? d.uasId : "?");
          }
        }
        return;
      }
    }

    off += 2 + ieLen;
  }
}

#if WIFI_HOP
static void hopTask(void *)
{
  static const uint8_t channels[] = {1, 6, 11};
  int i = 0;
  for (;;) {
    esp_wifi_set_channel(channels[i], WIFI_SECOND_CHAN_NONE);
    i = (i + 1) % 3;
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}
#endif

void ridWifiStart()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // BLE and WiFi coexistence requires WiFi modem sleep. WIFI_PS_NONE makes
  // the BT controller abort in pm_set_sleep_type / coex_core_enable.
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

  wifi_promiscuous_filter_t filter = {};
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_ALL;
  esp_wifi_set_promiscuous_filter(&filter);

  esp_wifi_set_promiscuous_rx_cb(&wifiSniffer);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

#if WIFI_HOP
  xTaskCreatePinnedToCore(hopTask, "rid_hop", 2048, nullptr, 1, nullptr, 0);
#endif

  logf("[WiFi] promiscuous on channel %d\n", WIFI_CHANNEL);
}
