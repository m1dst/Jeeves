#include "globals.h"
#include "configuration.h"
#include "Arduino.h"

void soundBell(bool enable)
{
  digitalWrite(RELAY_PIN, enable);
}
