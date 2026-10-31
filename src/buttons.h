#pragma once

#include <Arduino.h>

// Bit order: 0=UP, 1=DOWN, 2=OK, 3=BACK
#define BTN_UP_BIT   0
#define BTN_DOWN_BIT 1
#define BTN_OK_BIT   2
#define BTN_BACK_BIT 3

void buttonsInit();

// Call exactly once per loop. Returns bits that were just pressed (edges).
uint8_t buttonsPoll();

// Current debounced state (1 = pressed).
uint8_t buttonsHeld();
