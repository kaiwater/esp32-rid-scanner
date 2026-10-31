#include "log.h"
#include <stdarg.h>

void logInit()
{
  Serial0.begin(115200);
  Serial.begin(115200);
  delay(100);
}

void logf(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  Serial0.print(buf);
  Serial.print(buf);
}
