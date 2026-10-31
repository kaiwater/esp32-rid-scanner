#include <Arduino.h>
#include "log.h"
#include "display.h"

#ifdef ALIGN_TEST
#include "align.h"
#elif defined(HW_TEST)
#include "hw_test.h"
#else
#include "scanner.h"
#endif

void setup()
{
  logInit();
  displayInit();

#ifdef ALIGN_TEST
  alignSetup();
#elif defined(HW_TEST)
  hwTestSetup();
#else
  scannerSetup();
#endif
}

void loop()
{
#ifdef ALIGN_TEST
  alignLoop();
#elif defined(HW_TEST)
  hwTestLoop();
#else
  scannerLoop();
#endif
}
