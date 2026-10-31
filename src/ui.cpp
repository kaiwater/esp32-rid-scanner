#include "ui.h"
#include "display.h"
#include "rid_wifi.h"

static LGFX_Sprite gCanvas(&tft);
static lgfx::LGFXBase *gGfx = &tft;
static bool gCanvasReady = false;

static inline lgfx::LGFXBase& gfx() { return *gGfx; }

static void pushCanvas()
{
  if (gCanvasReady) gCanvas.pushSprite(0, 0);
}

// Traffic-light scale: green = strong signal / quiet noise,
// yellow = medium, red = weak signal / high noise.
static uint16_t rssiColor(int rssi)
{
  if (rssi >= -60) return 0x07E0;  // green: strong
  if (rssi >= -80) return 0xFFE0;  // yellow: medium
  return 0xF800;                   // red: weak
}

static uint16_t noiseColor(int noise)
{
  if (noise <= -95) return 0x07E0;  // green: quiet
  if (noise <= -85) return 0xFFE0;  // yellow: medium
  return 0xF800;                    // red: high noise
}

static void textAt(int x, int y, uint16_t fg, uint16_t bg, const char *s)
{
  gfx().setTextDatum(top_left);
  gfx().setTextColor(fg, bg);
  gfx().setCursor(x, y);
  gfx().print(s);
}

static void textRight(int xRight, int y, uint16_t fg, uint16_t bg, const char *s)
{
  int w = (int)strlen(s) * 6;
  textAt(xRight - w, y, fg, bg, s);
}

static void header(const char *title, int count, bool scanning)
{
  gfx().fillRect(0, 0, 160, 14, C_HEADER);
  textAt(2, 3, C_ACCENT, C_HEADER, title);

  char buf[12];
  snprintf(buf, sizeof(buf), "%d", count);
  textRight(158, 3, C_SEL_FG, C_HEADER, buf);

  if (scanning && ((millis() / 400) & 1)) {
    gfx().fillCircle(146, 7, 2, C_ACCENT);
  }
}

static void footer(const char *s)
{
  gfx().fillRect(0, 120, 160, 8, C_HEADER);
  textAt(2, 120, C_DIM, C_HEADER, s);
}

static void fmtAge(char *buf, size_t n, uint32_t now, uint32_t lastSeen)
{
  uint32_t ms = now - lastSeen;
  if (ms < 1000) snprintf(buf, n, "%.1fs", ms / 1000.0f);
  else if (ms < 10000) snprintf(buf, n, "%.1fs", ms / 1000.0f);
  else snprintf(buf, n, "%lus", (unsigned long)(ms / 1000));
}

void uiInit()
{
  gCanvas.setColorDepth(16);
  gCanvasReady = (gCanvas.createSprite(160, 128) != nullptr);
  gGfx = gCanvasReady ? (lgfx::LGFXBase *)&gCanvas : (lgfx::LGFXBase *)&tft;

  gfx().fillScreen(C_BG);
  gfx().setFont(&fonts::Font0);
  gfx().setTextSize(1);
  gfx().setTextWrap(false);
  pushCanvas();
}

void uiDrawBoot()
{
  gfx().fillScreen(C_BG);
  textAt(35, 38, C_ACCENT, C_BG,  "RID SCANNER");
  textAt(17, 54, C_SEL_FG, C_BG,  "ESP32-S3 + ST7735S");
  textAt(38, 70, C_DIM, C_BG,     "starting...");
  pushCanvas();
}

void uiDrawRfScan(const RidWifiRfCh *stats, int currentIdx, int currentChannel, uint32_t now)
{
  (void)now;
  char buf[32];

  gfx().fillRect(0, 0, 160, 14, C_HEADER);
  textAt(2, 3, C_ACCENT, C_HEADER, "RF SCAN");
  snprintf(buf, sizeof(buf), "CH%d", currentChannel);
  textRight(158, 3, C_SEL_FG, C_HEADER, buf);

  gfx().fillRect(0, 14, 160, 106, C_BG);

  textAt(4, 18, C_DIM, C_BG, "CH");
  textAt(28, 18, C_DIM, C_BG, "PPS");
  textAt(64, 18, C_DIM, C_BG, "NOISE");
  textAt(112, 18, C_DIM, C_BG, "RSSI");

  for (int i = 0; i < 3; i++) {
    int y = 32 + i * 14;
    bool sel = (i == currentIdx);
    uint16_t bg = sel ? C_SEL_BG : C_BG;
    uint16_t fg = sel ? C_SEL_FG : C_ACCENT;

    if (sel) gfx().fillRect(0, y - 2, 160, 12, bg);

    snprintf(buf, sizeof(buf), "%2d", stats[i].channel);
    textAt(8, y, fg, bg, buf);

    snprintf(buf, sizeof(buf), "%4lu", (unsigned long)stats[i].pps);
    textAt(28, y, fg, bg, buf);

    if (stats[i].noiseValid) snprintf(buf, sizeof(buf), "%4d", stats[i].noise);
    else snprintf(buf, sizeof(buf), "  --");
    textAt(66, y, stats[i].noiseValid ? noiseColor(stats[i].noise) : C_DIM, bg, buf);

    if (stats[i].rssiValid) snprintf(buf, sizeof(buf), "%4d", stats[i].rssi);
    else snprintf(buf, sizeof(buf), "  --");
    textAt(114, y, stats[i].rssiValid ? rssiColor(stats[i].rssi) : C_DIM, bg, buf);
  }

  textAt(4, 80, C_WARN, C_BG, "RID PAUSED");
  textAt(4, 92, C_DIM, C_BG, "PPS = packets/s");

  footer("K3 CH  HOLD=BACK");
  pushCanvas();
}

void uiDrawFatal(const char *line1, const char *line2)
{
  gfx().fillScreen(C_BG);
  gfx().fillRect(0, 40, 160, 48, C_ERR);
  textAt(4, 48, C_SEL_FG, C_ERR, line1);
  textAt(4, 64, C_SEL_FG, C_ERR, line2);
  pushCanvas();
}

void uiDrawList(const RidDrone *list, int count, int selected, uint32_t now)
{
  header("RID MON", count, true);

  RidWifiRf rf;
  ridWifiGetRf(&rf);
  char nf[12];
  if (rf.noiseValid) snprintf(nf, sizeof(nf), "NF%d", rf.noise);
  else snprintf(nf, sizeof(nf), "NF--");
  textAt(78, 3, rf.noiseValid ? noiseColor(rf.noise) : C_DIM, C_HEADER, nf);

  gfx().fillRect(0, 14, 160, 106, C_BG);

  if (count == 0) {
    textAt(22, 44, C_ACCENT, C_BG, "Scanning for");
    textAt(22, 56, C_ACCENT, C_BG, "Remote ID...");
    textAt(10, 76, C_DIM, C_BG,    "BLE + WiFi ch6");
    textAt(10, 90, C_DIM, C_BG,    "ASTM F3411");
    footer("K1/K2 SEL  K3 VIEW");
    pushCanvas();
    return;
  }

  if (selected < 0) selected = 0;
  if (selected >= count) selected = count - 1;

  for (int i = 0; i < UI_ROW_COUNT; i++) {
    int y = 16 + i * 26;
    bool sel = (i == selected);
    uint16_t bg = sel ? C_SEL_BG : C_BG;
    uint16_t fg = sel ? C_SEL_FG : C_ACCENT;

    if (i < count) {
      const RidDrone &d = list[i];
      if (sel) gfx().fillRect(0, y, 160, 26, bg);

      char l1[32];
      const char *id = d.hasBasicId && d.uasId[0] ? d.uasId : "(no id)";
      snprintf(l1, sizeof(l1), "%c%-12.12s", sel ? '>' : ' ', id);
      textAt(2, y + 2, fg, bg, l1);

      snprintf(l1, sizeof(l1), "%4d", d.rssi);
      textAt(84, y + 2, rssiColor(d.rssi), bg, l1);

      char l2[32];
      if (d.hasLocation) {
        snprintf(l2, sizeof(l2), "%.5f,%.5f", d.lat, d.lon);
      } else {
        snprintf(l2, sizeof(l2), "waiting for position");
      }
      textAt(9, y + 13, sel ? C_SEL_FG : C_DIM, bg, l2);

      char age[12];
      fmtAge(age, sizeof(age), now, d.lastSeen);
      textRight(158, y + 2, sel ? C_SEL_FG : C_DIM, bg, age);
    } else if (sel) {
      gfx().fillRect(0, y, 160, 26, bg);
    }

    gfx().drawFastHLine(0, y + 25, 160, 0x2104);
  }

  footer("K1/K2 SEL  K3 VIEW");
  pushCanvas();
}

void uiDrawDetail(const RidDrone &d, int index, int total, uint32_t now, int page)
{
  char buf[64];

  if (page < 0) page = 0;
  page %= 3;

  snprintf(buf, sizeof(buf), "DRONE %d/%d P%d/3", index + 1, total, page + 1);
  header(buf, total, false);

  gfx().fillRect(0, 14, 160, 98, C_BG);

  int y = 16;
  const int step = 9;

  if (page == 0) {
    textAt(2, y, C_DIM, C_BG, "ID"); y += step;
    textAt(2, y, C_SEL_FG, C_BG, d.hasBasicId && d.uasId[0] ? d.uasId : "--"); y += step + 2;

    if (d.hasLocation) {
      snprintf(buf, sizeof(buf), "LAT %.6f", d.lat);
      textAt(2, y, C_ACCENT, C_BG, buf); y += step;
      snprintf(buf, sizeof(buf), "LON %.6f", d.lon);
      textAt(2, y, C_ACCENT, C_BG, buf); y += step;
      snprintf(buf, sizeof(buf), "ALT %.0fm  HGT %.0fm", d.altGeo, d.height);
      textAt(2, y, C_SEL_FG, C_BG, buf); y += step;
      snprintf(buf, sizeof(buf), "SPD %.1fm/s HDG %.0f", d.speedH, d.heading);
      textAt(2, y, C_SEL_FG, C_BG, buf); y += step;
    } else {
      textAt(2, y, C_DIM, C_BG, "LAT --"); y += step;
      textAt(2, y, C_DIM, C_BG, "LON --"); y += step;
      textAt(2, y, C_DIM, C_BG, "ALT --  HGT --"); y += step;
      textAt(2, y, C_DIM, C_BG, "SPD --  HDG --"); y += step;
    }

    snprintf(buf, sizeof(buf), "SRC %s",
             d.source == RID_SOURCE_BLE ? "BLE" : "WiFi");
    textAt(2, y, C_SEL_FG, C_BG, buf);
    snprintf(buf, sizeof(buf), "RSSI %d", d.rssi);
    textAt(78, y, rssiColor(d.rssi), C_BG, buf);
  } else if (page == 1) {
    textAt(2, y, C_DIM, C_BG, "OPERATOR ID"); y += step;
    textAt(2, y, C_SEL_FG, C_BG, d.hasOperatorId && d.opId[0] ? d.opId : "--"); y += step + 2;

    if (d.hasSystem) {
      snprintf(buf, sizeof(buf), "OP LAT %.5f", d.opLat);
      textAt(2, y, C_WARN, C_BG, buf); y += step;
      snprintf(buf, sizeof(buf), "OP LON %.5f", d.opLon);
      textAt(2, y, C_WARN, C_BG, buf); y += step;
    } else {
      textAt(2, y, C_DIM, C_BG, "OP LAT --"); y += step;
      textAt(2, y, C_DIM, C_BG, "OP LON --"); y += step;
    }

    char age[12];
    fmtAge(age, sizeof(age), now, d.lastSeen);
    snprintf(buf, sizeof(buf), "AGE %s", age);
    textAt(2, y, C_SEL_FG, C_BG, buf); y += step;

    snprintf(buf, sizeof(buf), "MAC %02X:%02X:%02X:%02X:%02X:%02X",
             d.mac[0], d.mac[1], d.mac[2], d.mac[3], d.mac[4], d.mac[5]);
    textAt(2, y, C_DIM, C_BG, buf); y += step;

    snprintf(buf, sizeof(buf), "SRC %s",
             d.source == RID_SOURCE_BLE ? "BLE" : "WiFi");
    textAt(2, y, C_SEL_FG, C_BG, buf);
    snprintf(buf, sizeof(buf), "RSSI %d", d.rssi);
    textAt(78, y, rssiColor(d.rssi), C_BG, buf);
  } else {
    RidWifiRf rf;
    ridWifiGetRf(&rf);

    textAt(2, y, C_ACCENT, C_BG, "RF STATUS"); y += step + 2;

    snprintf(buf, sizeof(buf), "CH %d  PPS %lu", WIFI_CHANNEL, (unsigned long)rf.pps);
    textAt(2, y, C_SEL_FG, C_BG, buf); y += step;

    if (rf.noiseValid) snprintf(buf, sizeof(buf), "NOISE %d dBm", rf.noise);
    else snprintf(buf, sizeof(buf), "NOISE -- dBm");
    textAt(2, y, C_WARN, C_BG, buf); y += step;

    snprintf(buf, sizeof(buf), "RSSI %d dBm", rf.rssi);
    textAt(2, y, C_SEL_FG, C_BG, buf); y += step;

    snprintf(buf, sizeof(buf), "PKT %lu", (unsigned long)rf.packets);
    textAt(2, y, C_DIM, C_BG, buf); y += step;

    snprintf(buf, sizeof(buf), "MGMT %lu", (unsigned long)rf.mgmt);
    textAt(2, y, C_DIM, C_BG, buf);
  }

  footer("K3 PAGE  HOLD=BACK");
  pushCanvas();
}
