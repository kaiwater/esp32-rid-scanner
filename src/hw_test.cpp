#include "hw_test.h"
#include "display.h"
#include "buttons.h"
#include "config.h"
#include "log.h"

static uint8_t  gRotation = LCD_ROTATION;
static uint8_t  gBrightness = 80;
static bool     gInvert = LCD_INVERT;

static const uint16_t kBarColors[8] = {
  0xF800, 0xFD20, 0xFFE0, 0x07E0, 0x07FF, 0x001F, 0xF81F, 0xFFFF
};

static void textAt(int x, int y, uint16_t fg, uint16_t bg, const char *s)
{
  tft.setTextDatum(top_left);
  tft.setTextColor(fg, bg);
  tft.setCursor(x, y);
  tft.print(s);
}

static void drawButtonBoxes()
{
  const uint8_t held = buttonsHeld();
  const char *labels[4] = {"U", "D", "OK", "B"};

  for (int i = 0; i < 4; i++) {
    int x = 4 + i * 40;
    bool down = (held >> i) & 1;
    uint16_t fill = down ? C_ACCENT : 0x18E3;
    uint16_t fg = down ? C_BG : C_SEL_FG;

    tft.fillRect(x, 84, 32, 22, fill);
    tft.drawRect(x, 84, 32, 22, C_DIM);
    textAt(x + 10, 90, fg, fill, labels[i]);
  }
}

static void drawAll()
{
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, 160, 14, C_HEADER);
  textAt(2, 3, C_ACCENT, C_HEADER, "HW TEST  ESP32-S3");

  char buf[48];
  snprintf(buf, sizeof(buf), "ROT %u  BL %u%%  INV %u",
           gRotation, gBrightness, gInvert ? 1 : 0);
  textAt(2, 18, C_SEL_FG, C_BG, buf);

  for (int i = 0; i < 8; i++) {
    tft.fillRect(i * 20, 30, 20, 18, kBarColors[i]);
  }

  textAt(2, 52, C_DIM, C_BG, "SCK12 MOSI11 CS10");
  textAt(2, 62, C_DIM, C_BG, "DC9  RST8  BL7");
  textAt(2, 72, C_DIM, C_BG, "BTN 4 5 6 13");

  drawButtonBoxes();

  textAt(2, 110, C_WARN, C_BG, "OK=rotate  B=invert");
  textAt(2, 120, C_DIM, C_BG, "U/D=brightness");
}

void hwTestSetup()
{
  logf("\n=== ESP32-S3 ST7735 hardware test ===\n");
  logf("chip=%s rev=%d cores=%d\n",
       ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores());
  logf("flash=%u bytes  psram=%u bytes  heap=%u\n",
       (unsigned)ESP.getFlashChipSize(), (unsigned)ESP.getPsramSize(),
       (unsigned)ESP.getFreeHeap());
  logf("display: SCK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d\n",
       PIN_LCD_SCK, PIN_LCD_MOSI, PIN_LCD_CS, PIN_LCD_DC,
       PIN_LCD_RST, PIN_LCD_BL);
  logf("buttons: UP=%d DOWN=%d OK=%d BACK=%d (to GND)\n",
       PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_OK, PIN_BTN_BACK);
  logf("If the screen stays white: check BLK, RST, DC, CS and 3V3/GND.\n");

  buttonsInit();
  displaySetBrightness(gBrightness);
  displaySetInvert(gInvert);
  drawAll();

  logf("HW test ready. Press each button and watch the boxes.\n");
}

void hwTestLoop()
{
  uint8_t pressed = buttonsPoll();

  if (pressed & (1u << BTN_UP_BIT)) {
    gBrightness = (gBrightness >= 100) ? 100 : gBrightness + 10;
    displaySetBrightness(gBrightness);
    logf("BTN UP   -> brightness %u%%\n", gBrightness);
    drawAll();
  }
  if (pressed & (1u << BTN_DOWN_BIT)) {
    gBrightness = (gBrightness <= 10) ? 10 : gBrightness - 10;
    displaySetBrightness(gBrightness);
    logf("BTN DOWN -> brightness %u%%\n", gBrightness);
    drawAll();
  }
  if (pressed & (1u << BTN_OK_BIT)) {
    gRotation = (gRotation + 1) & 3;
    displaySetRotation(gRotation);
    logf("BTN OK   -> rotation %u\n", gRotation);
    drawAll();
  }
  if (pressed & (1u << BTN_BACK_BIT)) {
    gInvert = !gInvert;
    displaySetInvert(gInvert);
    logf("BTN BACK -> invert %u\n", gInvert ? 1 : 0);
    drawAll();
  }

  if (buttonsHeld()) {
    drawButtonBoxes();
  } else {
    static uint8_t lastHeld = 0xFF;
    uint8_t held = buttonsHeld();
    if (held != lastHeld) {
      lastHeld = held;
      drawButtonBoxes();
    }
  }

  delay(5);
}
