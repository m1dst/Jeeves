#include "Arduino.h"
#include "n1mm.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>

N1MM::N1MM()
{
}

void N1MM::startListening()
{
  _wifiUdp.begin(N1MM_UDP_PORT);
  Serial.printf("%s enabled.  Listening on port %d\n", NAME, N1MM_UDP_PORT);
}

void N1MM::stopListening()
{
  _wifiUdp.stop();
  Serial.printf("%s disabled.\n", NAME);
}

void N1MM::service()
{
  int packetSize = _wifiUdp.parsePacket();
  if (packetSize) {

    // Report that a packet was received to the serial port. (for debug purposes)
    Serial.printf("Received %d bytes from %s, port %d (%s)\n", packetSize, _wifiUdp.remoteIP().toString().c_str(), _wifiUdp.remotePort(), NAME);

    char incomingPacket[2000];
    int len = _wifiUdp.read(incomingPacket, packetSize);
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    // Display the contents of the packet (for debug purposes)
    //Serial.printf("UDP packet contents: %s\n\n", incomingPacket);

    processPacket(incomingPacket);
  }

}


// Returns the value of an xml parameter from the supplied string
String N1MM::xmlTakeParam(String inStr, String needParam)
{
  if (inStr.indexOf("<" + needParam + ">") > 0) {
    int CountChar = needParam.length();
    int indexStart = inStr.indexOf("<" + needParam + ">");
    int indexStop = inStr.indexOf("</" + needParam + ">");
    return inStr.substring(indexStart + CountChar + 2, indexStop);
  }
  return "";
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processPacket(char * incomingPacket)
{
  char b[] = "<contactinfo>";
  char* output = strstr(incomingPacket, b);

  // Check if the packet received is an N1MM Plus ContactInfo broadcast.
  // http://n1mm.hamdocs.com/tiki-index.php?page=UDP+Broadcasts
  if (output) {
    String isMultiplier1 = xmlTakeParam(incomingPacket, "ismultiplier1");
    String isMultiplier2 = xmlTakeParam(incomingPacket, "ismultiplier2");
    String isMultiplier3 = xmlTakeParam(incomingPacket, "ismultiplier3");

    if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "0") {
      Serial.printf("MULT 1:0:0 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Set the LEDs to be all on and red.
      ws2812fx.setColor(0xFF0000);
      ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "0") {
      Serial.printf("MULT 0:1:0 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Set the LEDs to be all on and blue.
      ws2812fx.setColor(0x0000FF);
      ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "0" && isMultiplier3 == "1") {
      Serial.printf("MULT 0:0:1 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Set the LEDs to be all on and green.
      ws2812fx.setColor(0x00FF00);
      ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "0") {
      Serial.printf("MULT 1:1:0 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Red & Blue
      ws2812fx.setMode(FX_MODE_RUNNING_RED_BLUE);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "1") {
      Serial.printf("MULT 0:1:1 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      //Green & Blue
      ws2812fx.setMode(FX_MODE_RUNNING_GREEN_BLUE);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "1") {
      Serial.printf("MULT 1:0:1 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Red & Green
      ws2812fx.setMode(FX_MODE_MERRY_CHRISTMAS);
      #endif
      soundBell(true);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "1") {
      Serial.printf("MULT 1:1:1 (%s)\n", NAME);
      #ifdef ENABLE_LEDS
      // Rainbow
      ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);
      #endif
      soundBell(true);
      last_change = now;
    }
#ifdef DISPLAY_EVERY_QSO || SOUND_EVERY_QSO
    else {
#ifdef DISPLAY_EVERY_QSO
      #ifdef ENABLE_LEDS
      ws2812fx.setMode(FX_MODE_RUNNING_LIGHTS);
      #endif
#endif
#ifdef SOUND_EVERY_QSO
      soundBell(true);
#endif
      last_change = now;
    }
#else
    else {
      #ifdef ENABLE_LEDS
      // Set the LEDs to be all on and white.
      ws2812fx.setColor(0xFFFFFF);
      ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      soundBell(false);
      last_change = now;
    }
#endif

  }
}
