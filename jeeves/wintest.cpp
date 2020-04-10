#include "Arduino.h"
#include "WinTest.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>

WinTest::WinTest()
{
}

void WinTest::startListening()
{
  _wifiUdp.begin(WINTEST_UDP_PORT);
  Serial.printf("%s enabled.  Listening on port %d\n", NAME, WINTEST_UDP_PORT);
}

void WinTest::stopListening()
{
  _wifiUdp.stop();
  Serial.printf("%s disabled.\n", NAME);
}

void WinTest::service()
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

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void WinTest::processPacket(char * incomingPacket)
{
  // Remove the checksum character
  incomingPacket[strlen(incomingPacket) - 1] = '\0';

  // Check if the packet is for when a QSO is logged.
  if (strstr(incomingPacket, "ADDQSO: "))
  {
#ifdef DISPLAY_EVERY_QSO || SOUND_EVERY_QSO
#ifdef DISPLAY_EVERY_QSO
    #ifdef ENABLE_LEDS
    ws2812fx.setMode(FX_MODE_RUNNING_LIGHTS);
    #endif
#endif
#ifdef SOUND_EVERY_QSO
    soundBell(1);
#endif
    last_change = now;
#endif
  }

  // Check if the packet is a gab message.  eg: GAB: "STN1" "" "test"
  if (strstr(incomingPacket, "GAB: "))
  {
    // Extract the message
    soundBell(1);
  }

  // Check if the packet is a summary record.
  else if (strstr(incomingPacket, "SUMMARY: "))
  {

    // SUMMARY: "STN1" "" 32581 "HEADERS" 1 5 8 10 6 14 15
    // SUMMARY: "STN1" "" 32581 "ROW" 0 "TOTAL" 400 18 78 0 577 1.44

    if (strstr(incomingPacket, "\"HEADERS\""))
    {
      // Reset the position of multipliers
      mult1Position = 0;
      mult2Position = 0;
      mult3Position = 0;

      // Find the position of the first data element.
      int startPosition = strstr(incomingPacket, "\"HEADERS\"") - incomingPacket + 10;

      // Loop through each value.
      int i = 0;
      char *token = strtok(&incomingPacket[startPosition], " ");
      while (token != NULL)
      {
        unsigned int id = atoi(token); // Convert to integer
        if (isHeaderMultiplier(id))
        {
          //Serial.printf("Mult found [%u] at position: %u\n", id, i);
          if (mult1Position == 0)
          {
            mult1Position = i;
          }
          else if (mult2Position == 0)
          {
            mult2Position = i;
          }
          else if (mult3Position == 0)
          {
            mult3Position = i;
          }
        }
        token = strtok(NULL, " ");
        i++;
      }
    }

    // Check if the packet is a summary total.
    else if (strstr(incomingPacket, "\"TOTAL\""))
    {

      // Find the position of the first data element.
      int startPosition = strstr(incomingPacket, "\"TOTAL\"") - incomingPacket + 8;

      unsigned int countOfQsos = 0;
      unsigned int countOfMult1 = 0;
      unsigned int countOfMult2 = 0;
      unsigned int countOfMult3 = 0;
      unsigned int countOfDupe = 0;
      unsigned int pointsNet = 0;
      float average;

      // Loop through each value.
      int i = 1;
      char *token = strtok(&incomingPacket[startPosition], " ");
      while (token != NULL)
      {
        if (mult1Position == i)
        {
          countOfMult1 = atoi(token);
        }
        else if (mult2Position == i)
        {
          countOfMult2 = atoi(token);
        }
        else if (mult3Position == i)
        {
          countOfMult3 = atoi(token);
        }
        token = strtok(NULL, " ");
        i++;
      }

      //Serial.printf("QSOS: %u (%u)\n", countOfQsos, previousCountOfQsos);
      //Serial.printf("countOfMult1: %u (%u)\n", countOfMult1, previousCountOfMult1);
      //Serial.printf("countOfMult2: %u (%u)\n", countOfMult2, previousCountOfMult2);
      //Serial.printf("countOfMult3: %u (%u)\n", countOfMult3, previousCountOfMult3);
      //Serial.printf("countOfDupe: %u (%u)\n", countOfDupe, previousCountOfDupe);
      //Serial.printf("pointsNet: %u (%u)\n", pointsNet, previousPointsNet);
      //Serial.printf("average: %f\n", average);  //  Can't parse or print floats currently.  Seems like a bug.

      bool isMultiplier1 = 0;
      bool isMultiplier2 = 0;
      bool isMultiplier3 = 0;

      // When the unit starts cold, we don't want to action the first summary total update.  We just store the data for next time.
      if (isWarm)
      {
        isMultiplier1 = (countOfMult1 > previousCountOfMult1);
        isMultiplier2 = (countOfMult2 > previousCountOfMult2);
        isMultiplier3 = (countOfMult3 > previousCountOfMult3);
      }

      if (isMultiplier1 && !isMultiplier2 && !isMultiplier3 ) {
        Serial.printf("MULT 1:0:0 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and red.
        ws2812fx.setColor(0xFF0000);
        ws2812fx.setMode(FX_MODE_STATIC);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (!isMultiplier1 && isMultiplier2 && !isMultiplier3) {
        Serial.printf("MULT 0:1:0 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and blue.
        ws2812fx.setColor(0x0000FF);
        ws2812fx.setMode(FX_MODE_STATIC);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (!isMultiplier1 && !isMultiplier2 && isMultiplier3) {
        Serial.printf("MULT 0:0:1 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and green.
        ws2812fx.setColor(0x00FF00);
        ws2812fx.setMode(FX_MODE_STATIC);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (isMultiplier1 && isMultiplier2 && !isMultiplier3 ) {
        Serial.printf("MULT 1:1:0 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Red & Blue
        ws2812fx.setMode(FX_MODE_RUNNING_RED_BLUE);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (!isMultiplier1 && isMultiplier2 && isMultiplier3 ) {
        Serial.printf("MULT 0:1:1 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Green & Blue
        ws2812fx.setMode(FX_MODE_RUNNING_GREEN_BLUE);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (isMultiplier1 && !isMultiplier2 && isMultiplier3 ) {
        Serial.printf("MULT 1:0:1 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Red & Green
        ws2812fx.setMode(FX_MODE_MERRY_CHRISTMAS);
        #endif
        soundBell(1);
        last_change = now;
      }
      else if (isMultiplier1 && isMultiplier2 && isMultiplier3 ) {
        Serial.printf("MULT 1:1:1 (%s)\n", NAME);
        #ifdef ENABLE_LEDS
        // Rainbow
        ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);
        #endif
        soundBell(1);
        last_change = now;
      }

      previousCountOfQsos = countOfQsos;
      previousCountOfMult1 = countOfMult1;
      previousCountOfMult2 = countOfMult2;
      previousCountOfMult3 = countOfMult3;
      previousCountOfDupe = countOfDupe;
      previousPointsNet = pointsNet;
      isWarm = true;

    }
  }
}

bool WinTest::isHeaderMultiplier(int id)
{
  for (int i = 0; i < sizeof(validMultipliers) ; i++) {
    if (validMultipliers[i] == id)
      return true;
  }
  return false;
}
