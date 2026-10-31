#include "display.h"

LGFX tft;

void displayInit()
{
  tft.init();
  tft.setRotation(LCD_ROTATION);
  tft.setBrightness(200);
  tft.fillScreen(C_BG);
  tft.setTextWrap(false);
  tft.setFont(&fonts::Font0);
  tft.setTextSize(1);
}

void displaySetBrightness(uint8_t percent)
{
#if PIN_LCD_BL >= 0
  if (percent > 100) percent = 100;
  tft.setBrightness((uint8_t)((uint16_t)percent * 255 / 100));
#else
  (void)percent;
#endif
}

void displaySetRotation(uint8_t rotation)
{
  tft.setRotation(rotation & 3);
}

void displaySetInvert(bool invert)
{
  tft.invertDisplay(invert);
}

void displaySetOffsets(int x, int y)
{
  tft.setPanelOffsets(x, y);
}

void displayPrintf(int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...)
{
  char buf[96];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  tft.setTextDatum(top_left);
  tft.setTextColor(fg, bg);
  tft.setCursor(x, y);
  tft.print(buf);
}
