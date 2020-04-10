#include "globals.h"
#include "configuration.h"
#include "Arduino.h"

void soundBell(int count)
{
#ifdef RELAY_INVERTED
	for (int i=0; i < count; i++){
		digitalWrite(RELAY_PIN, false);
		delay(RELAY_TIMER_MS);
		digitalWrite(RELAY_PIN, true);
		delay(RELAY_TIMER_MS);
	}
#else
	for (int i=0; i < count; i++){
		digitalWrite(RELAY_PIN, true);
		delay(RELAY_TIMER_MS);
		digitalWrite(RELAY_PIN, false);
		delay(RELAY_TIMER_MS);
	}
#endif
}

void append(char* s, char c) {
  int len = strlen(s);
  s[len] = c;
  s[len+1] = '\0';
}