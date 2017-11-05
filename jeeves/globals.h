#ifndef globals_h
#define globals_h

#include "Arduino.h"

#ifdef ENABLE_LEDS
#include "WS2812FX.h"
#endif

extern unsigned long last_change;
extern unsigned long now;

#ifdef ENABLE_LEDS
extern WS2812FX ws2812fx;
#endif

void soundBell(bool enable);

#endif


