#include "buttons.h"
#include "config.h"

static const uint8_t kPins[4] = {
  PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_OK, PIN_BTN_BACK
};

static uint8_t  gStable = 0;
static uint8_t  gPrev   = 0;
static uint32_t gLastChange[4] = {0, 0, 0, 0};

static uint8_t readRaw()
{
  uint8_t raw = 0;
  for (int i = 0; i < 4; i++) {
#if BTN_ACTIVE_LOW
    bool down = (digitalRead(kPins[i]) == LOW);
#else
    bool down = (digitalRead(kPins[i]) == HIGH);
#endif
    if (down) raw |= (1u << i);
  }
  return raw;
}

void buttonsInit()
{
  for (int i = 0; i < 4; i++) {
#if BTN_ACTIVE_LOW
    pinMode(kPins[i], INPUT_PULLUP);
#else
    pinMode(kPins[i], INPUT_PULLDOWN);
#endif
  }
  uint32_t now = millis();
  gStable = readRaw();
  gPrev = gStable;
  for (int i = 0; i < 4; i++) gLastChange[i] = now;
}

uint8_t buttonsPoll()
{
  uint32_t now = millis();
  uint8_t raw = readRaw();

  for (int i = 0; i < 4; i++) {
    bool rawDown = (raw >> i) & 1;
    bool stableDown = (gStable >> i) & 1;
    if (rawDown != stableDown) {
      if (now - gLastChange[i] >= BTN_DEBOUNCE_MS) {
        if (rawDown) gStable |= (1u << i);
        else         gStable &= ~(1u << i);
        gLastChange[i] = now;
      }
    } else {
      gLastChange[i] = now;
    }
  }

  uint8_t pressed = gStable & ~gPrev;
  gPrev = gStable;
  return pressed;
}

uint8_t buttonsHeld()
{
  return gStable;
}
