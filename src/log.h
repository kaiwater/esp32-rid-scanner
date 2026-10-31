#pragma once

#include <Arduino.h>

// Logs to both the USB-CDC port (native USB) and UART0 (CH340 port),
// so whichever cable/port you use you still see output.
void logInit();
void logf(const char *fmt, ...);
