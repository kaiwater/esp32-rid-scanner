#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7735S _panel_instance;
  lgfx::Bus_SPI      _bus_instance;
#if PIN_LCD_BL >= 0
  lgfx::Light_PWM    _light_instance;
#endif

public:
  LGFX()
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host    = SPI2_HOST;       // FSPI on ESP32-S3
      cfg.spi_mode    = 0;
      cfg.freq_write  = 27000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = true;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = PIN_LCD_SCK;
      cfg.pin_mosi    = PIN_LCD_MOSI;
      cfg.pin_miso    = -1;
      cfg.pin_dc      = PIN_LCD_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs          = PIN_LCD_CS;
      cfg.pin_rst         = PIN_LCD_RST;
      cfg.pin_busy        = -1;
      cfg.panel_width     = LCD_WIDTH;
      cfg.panel_height    = LCD_HEIGHT;
      cfg.offset_x        = LCD_OFFSET_X;
      cfg.offset_y        = LCD_OFFSET_Y;
      cfg.offset_rotation = 0;
      cfg.readable        = false;
      cfg.invert          = LCD_INVERT;
      cfg.rgb_order       = false;
      cfg.dlen_16bit      = false;
      cfg.bus_shared      = false;
      _panel_instance.config(cfg);
    }
#if PIN_LCD_BL >= 0
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = PIN_LCD_BL;
      cfg.invert      = false;
      cfg.freq        = 12000;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
#endif
    setPanel(&_panel_instance);
  }

  void setPanelOffsets(int x, int y)
  {
    auto cfg = _panel_instance.config();
    cfg.offset_x = (uint16_t)x;
    cfg.offset_y = (uint16_t)y;
    _panel_instance.config(cfg);
    _panel_instance.setRotation(_panel_instance.getRotation());
  }
};

extern LGFX tft;

void displayInit();
void displaySetBrightness(uint8_t percent);
void displaySetRotation(uint8_t rotation);
void displaySetInvert(bool invert);
void displaySetOffsets(int x, int y);
void displayPrintf(int x, int y, uint16_t fg, uint16_t bg, const char *fmt, ...);

// Colours used by the UI
#define C_BG        0x0000
#define C_HEADER    0x0A2A
#define C_ACCENT    0x07E0
#define C_DIM       0x7BEF
#define C_WARN      0xFD20
#define C_SEL_BG    0x0320
#define C_SEL_FG    0xFFFF
#define C_ERR       0xF800
