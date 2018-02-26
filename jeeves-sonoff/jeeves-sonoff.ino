/*
  Jeeves v1.3 - The wireless, automated mult bell.
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

Hardware
  * Sonoff Basic

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
  2018-02 Added a wifi management system and ported to the Sonoff Basic hardware.
  
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

#include <ESP8266WiFi.h>          // ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     // Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

// Global variables

Ticker ticker;                    // Interval timer for the internal LED

#ifdef STATIC_IP
  IPAddress ip(192, 168, 0, 142);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);
#endif

#ifdef ENABLE_WINTEST
WinTest wintest;
#endif

#ifdef ENABLE_N1MM
N1MM n1mm;
#endif

#ifdef ENABLE_LEDS
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

unsigned long last_change = 0;
unsigned long now = 0;
WiFiManager wifiManager;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.print("Please connect to the SSID \"");
  Serial.print(myWiFiManager->getConfigPortalSSID());
  Serial.println("\" and configure a new network for me to connect to.");
  Serial.println("\nhttp://192.168.4.1/");
  Serial.println("\nYou have 3 minutes to configure me before I reboot.");
  Serial.println();
  Serial.println("Waiting...");
  ticker.attach(0.2, tick);
}

void saveConfigCallback () {
  Serial.println("Should reboot...");
  ESP.restart();
}

void tick()
{
  int state = digitalRead(INTERNAL_LED_PIN);  // get the current state of GPIO1 pin
  digitalWrite(INTERNAL_LED_PIN, !state);     // set pin to the opposite state
}

void setup()
{

  pinMode(CONFIG_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  
  digitalWrite(INTERNAL_LED_PIN, true);       // Ensure the internal LED is switched off
  soundBell(false);                           // Ensure the bell is switched off

  ticker.attach(0.6, tick);                   // Start flashing the internal LED to indicate we're in startup mode.

  Serial.begin(115200);
  
  //wifiManager.resetSettings();                                      // Debug purposes - Clears configuration.

#ifdef STATIC_IP
  wifiManager.setSTAStaticIPConfig(ip, gateway, subnet);
#endif
  wifiManager.setAPCallback(configModeCallback);                      // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setSaveConfigCallback(saveConfigCallback);              // Set callback that gets called when saving configuration
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT_SECONDS);  // Set the number of seconds that a user has to connect to the configuration portal before a reset occurs.
  wifiManager.setConnectTimeout(WIFI_TIMEOUT_SECONDS);                // Set the number of seconds allowed for WiFi to connect before the configuration portal is presented.
  wifiManager.setDebugOutput(false);                                  // Disables the debug output.

  delay(1000);

  // Fetches ssid and pass and tries to connect. If it does not connect it starts an access point
  // with the specified name and password and goes into a blocking loop awaiting configuration.
  Serial.println("Attempting to connect to wifi network...");
  if(!wifiManager.autoConnect("Jeeves")) {
    Serial.println("Failed to connect to the configured network.");
    ESP.reset();
    delay(1000);
  } 

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

  // Report the IP address of the device to the serial port.
  Serial.println("Connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ticker.detach();
  digitalWrite(BUILTIN_LED, LOW);

  displaySplash();

#ifdef ENABLE_WINTEST
  wintest.startListening();
#endif

#ifdef ENABLE_N1MM
  n1mm.startListening();
#endif

  Serial.println();
}

void check_for_button_press()
{
  if ( digitalRead(CONFIG_PIN) == LOW )
  {
    Serial.println("\n\n***************************************************");
    Serial.println("Entering config mode.");
    Serial.println("***************************************************");
    if(!wifiManager.startConfigPortal("Jeeves"))
    {
      Serial.println("Failed to connect and hit timeout.");
      ESP.reset();
      delay(1000);
    } 
  }  
}

void loop()
{
  now = millis();

  check_for_button_press();

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
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("Attempting to connect.");
  }
  else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("No SSID available.");
  }
  else if (WiFi.status() == WL_SCAN_COMPLETED) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("WIFI scan completed.");
  }
  else if (WiFi.status() == WL_CONNECT_FAILED) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("Could not connect to WIFI.");
  }
  else if (WiFi.status() == WL_CONNECTION_LOST) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("Connection to WIFI has been lost.");
  }
  else if (WiFi.status() == WL_DISCONNECTED) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("WIFI disconnected.");
  }
  else
  {
    digitalWrite(INTERNAL_LED_PIN, false);       // Switch on the internal LED.
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
  Serial.println("Jeeves v1.3.0 - James Patterson (M1DST) - February 2018\n");
  Serial.println("An automated multiplier bell for amateur radio contests.\n");
  Serial.println("Website: http://www.m1dst.co.uk");
  Serial.println("Twitter: @m1dst\n\n");
}


