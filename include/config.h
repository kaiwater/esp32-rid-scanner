#pragma once

// ============================================================
//  ESP32-S3 + ST7735S handheld Remote ID scanner
//  All hardware pins are here. Change them to match your board.
// ============================================================

// ---------------- ST7735S 128x160 SPI display ----------------
// Defaults use the ESP32-S3 "FSPI" pins (safe, no strapping/USB pads).
#define PIN_LCD_SCK    12   // SCL / SCK / CLK
#define PIN_LCD_MOSI   11   // SDA / MOSI
#define PIN_LCD_CS     10   // CS
#define PIN_LCD_DC      9   // DC / A0
#define PIN_LCD_RST     8   // RES / RST
#define PIN_LCD_BL      7   // BLK / LED. Set to -1 if BLK is wired to 3V3.

#define LCD_WIDTH     128
#define LCD_HEIGHT    160
#define LCD_ROTATION    1   // 1 = landscape (160x128)

// Some 1.8" ST7735 panels need an offset. If the image is shifted or has
// a border, try (2,1), (1,2) or (0,1).
#define LCD_OFFSET_X    4
#define LCD_OFFSET_Y    0
#define LCD_INVERT      0   // 1 if colours look like a photo negative

// ---------------- 4 push buttons ----------------
// Wire each button between the GPIO and GND. Internal pull-ups are enabled.
#define PIN_BTN_UP      4
#define PIN_BTN_DOWN    5
#define PIN_BTN_OK      6
#define PIN_BTN_BACK   14   // K4; if your module's K4 is dead, hold OK (K3) to go back

#define BTN_ACTIVE_LOW  1
#define BTN_DEBOUNCE_MS 25

// ---------------- Remote ID scanning ----------------
#define WIFI_CHANNEL    6   // ASTM WiFi Beacon channel
#define WIFI_HOP        0   // 1 = hop channels 1/6/11 (may miss frames)
#define MAX_DRONES     16
#define DRONE_TIMEOUT_MS 30000UL

// ---------------- UI ----------------
#define UI_REFRESH_MS  200
#define UI_ROW_COUNT    4
