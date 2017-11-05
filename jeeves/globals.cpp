#include "globals.h"
#include "configuration.h"
#include "Arduino.h"

void soundBell(bool enable)
{
#ifdef RELAY_INVERTED
  digitalWrite(RELAY_PIN, !enable);
#else
  digitalWrite(RELAY_PIN, enable);
#endif
}
