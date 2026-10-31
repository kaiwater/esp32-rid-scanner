#include "align.h"
#include "display.h"
#include "buttons.h"
#include "config.h"
#include "log.h"

static int  gOx = LCD_OFFSET_X;
static int  gOy = LCD_OFFSET_Y;
static bool gOkWasDown = false;
static uint32_t gOkDownAt = 0;

static void drawAlign()
{
  tft.fillScreen(TFT_BLACK);

  // Two borders: if either is clipped, the offset is wrong.
  tft.drawRect(0, 0, 160, 128, TFT_WHITE);
  tft.drawRect(1, 1, 158, 126, TFT_RED);

  displayPrintf(60, 1, TFT_GREEN, TFT_BLACK, "TOP");
  displayPrintf(50, 119, TFT_GREEN, TFT_BLACK, "BOTTOM");
  displayPrintf(1, 60, TFT_CYAN, TFT_BLACK, "L");
  displayPrintf(151, 60, TFT_CYAN, TFT_BLACK, "R");

  displayPrintf(24, 50, TFT_YELLOW, TFT_BLACK, "OX=%d OY=%d", gOx, gOy);
  displayPrintf(8, 70, TFT_WHITE, TFT_BLACK, "K1/K2 = VERT");
  displayPrintf(8, 82, TFT_WHITE, TFT_BLACK, "K3 = HORIZ");
  displayPrintf(8, 94, 0x7BEF, TFT_BLACK, "HOLD K3 = RESET");
}

static void apply()
{
  displaySetOffsets(gOx, gOy);
  drawAlign();
  logf("[ALIGN] OX=%d OY=%d\n", gOx, gOy);
}

void alignSetup()
{
  buttonsInit();
  displaySetOffsets(gOx, gOy);
  drawAlign();
  logf("[ALIGN] start OX=%d OY=%d\n", gOx, gOy);
}

void alignLoop()
{
  uint32_t now = millis();
  uint8_t pressed = buttonsPoll();

  if (pressed & (1u << BTN_UP_BIT)) {
    if (gOy > 0) { gOy--; apply(); }
  }
  if (pressed & (1u << BTN_DOWN_BIT)) {
    if (gOy < 15) { gOy++; apply(); }
  }
  if (pressed & (1u << BTN_BACK_BIT)) {
    if (gOx > 0) { gOx--; apply(); }
  }

  bool okDown = (buttonsHeld() & (1u << BTN_OK_BIT)) != 0;
  if (okDown && !gOkWasDown) gOkDownAt = now;
  if (!okDown && gOkWasDown) {
    if (now - gOkDownAt >= 700) {
      gOx = 0;
      gOy = 0;
      apply();
    } else {
      gOx = (gOx + 1) % 8;
      apply();
    }
  }
  gOkWasDown = okDown;

  delay(5);
}
