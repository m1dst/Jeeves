/*
  Jeeves v1.2 - The wireless, automated mult bell.
-----------------------------------------------------------
  http://m1dst.co.uk
  2017-07 by M1DST

              _     _     _                    _    
    _ __ ___ / | __| |___| |_   ___ ___  _   _| | __
   | '_ ` _ \| |/ _` / __| __| / __/ _ \| | | | |/ /
   | | | | | | | (_| \__ \ |_ | (_| (_) | |_| |   < 
   |_| |_| |_|_|\__,_|___/\__(_)___\___(_)__,_|_|\_\
                                                     
                                                   
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

Features:
Support inputs
  * N1MM+
  * Win-Test

Outputs
  * Relay to drive a bell.
  * Collection of WS2812 LEDs.

  Changelog
  ---------
  2017-07 Initial version.  Drives LEDs and supports N1MM+
  2017-10 Add support for a relay/bell, trigger light/bell on each QSO (configurable)
  2017-11 Add full support for Win-Test
  
*/

/* =======================================================================================
   All configuration should be made to configuration.h
   ======================================================================================= */

#include "configuration.h"
#include "globals.h"
#ifdef ENABLE_WINTEST
#include "wintest.h"
#endif
#ifdef ENABLE_N1MM
#include "n1mm.h"
#endif
#ifdef ENABLE_LEDS
#include "WS2812FX.h"
#endif
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Global variables

#ifdef ENABLE_WINTEST
WinTest wintest;
#endif

#ifdef ENABLE_N1MM
N1MM n1mm;
#endif

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
unsigned long last_change = 0;
unsigned long now = 0;

void setup()
{

  pinMode(RELAY_PIN, OUTPUT);
  soundBell(false);

  Serial.begin(115200);

  // Write the splash screen out to the serial port.
  displaySplash();

#ifdef ENABLE_LEDS
  // Setup the LED string to white with a brightness of about 60%
  ws2812fx.init();
  ws2812fx.setBrightness(150);
  ws2812fx.setSpeed(255);
  ws2812fx.setColor(0xFFFFFF);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.start();
  ws2812fx.service();
#endif

  // Start connecting to the WIFI network.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);  // Add an initial delay to give the processor time to init.

    if (WiFi.status() == WL_IDLE_STATUS) {
      Serial.println("Attempting to connect.");
    }
    else if (WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println("No SSID available.");
    }
    else if (WiFi.status() == WL_SCAN_COMPLETED) {
      Serial.println("WIFI scan completed.");
    }
    else if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Could not connect to WIFI.");
    }
    else if (WiFi.status() == WL_CONNECTION_LOST) {
      Serial.println("Connection to WIFI has been lost.");
    }
    else if (WiFi.status() == WL_DISCONNECTED) {
      Serial.println("Not connected.");
    }
  }
  Serial.println();

  // Report the IP address of the device to the serial port.
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

#ifdef ENABLE_WINTEST
  wintest.startListening();
#endif

#ifdef ENABLE_N1MM
  n1mm.startListening();
#endif

  Serial.println();
}

void loop()
{
  now = millis();

#ifdef ENABLE_LEDS
  ws2812fx.service();
#endif

#ifdef ENABLE_WINTEST
  wintest.service();
#endif

#ifdef ENABLE_N1MM
  n1mm.service();
#endif

#ifdef ENABLE_AUDIBLE
  // Detect if the bell has been running for the configured length of time and reset the relay if it has.
  if (now - last_change > RELAY_TIMER_MS) {
    soundBell(false);
  }
#endif

#ifdef ENABLE_LEDS
  // Detect if the pattern has been running for the configured length of time and reset the pattern if it has.
  if (now - last_change > LED_TIMER_MS) {
    ws2812fx.setColor(0xFFFFFF);
    ws2812fx.setMode(FX_MODE_STATIC);
    last_change = now;
  }
#endif

  if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.println("Attempting to connect.");
  }
  else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    Serial.println("No SSID available.");
  }
  else if (WiFi.status() == WL_SCAN_COMPLETED) {
    Serial.println("WIFI scan completed.");
  }
  else if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.println("Could not connect to WIFI.");
  }
  else if (WiFi.status() == WL_CONNECTION_LOST) {
    Serial.println("Connection to WIFI has been lost.");
  }
  else if (WiFi.status() == WL_DISCONNECTED) {
    Serial.println("WIFI disconnected.");
  }
}

void displaySplash()
{
  Serial.println("\n\n\n       _                           ");
  Serial.println("      | |                          ");
  Serial.println("      | | ___  _____   _____  ___  ");
  Serial.println("  _   | |/ _ \\/ _ \\ \\ / / _ \\/ __| ");
  Serial.println(" | |__| |  __/  __/\\ V /  __/\\__ \\ ");
  Serial.println("  \\____/ \\___|\\___| \\_/ \\___||___/ \n\n");
  Serial.println("Jeeves v1.2.0 - James Patterson (M1DST) - November 2017\n");
  Serial.println("An automated multiplier bell for amateur radio contests.\n");
  Serial.println("Website: http://www.m1dst.co.uk");
  Serial.println("Twitter: @m1dst\n\n");
}


