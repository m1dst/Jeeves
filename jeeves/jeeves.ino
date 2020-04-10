/*
  Jeeves v1.4.0 - The wireless, automated mult bell.
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
  * Wemos D1 Mini

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
  2018-02 Added a wifi management system.
  2018-09 Added support for an LED matrix.
  
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
#include <WiFiUdp.h>
#include <Ticker.h>
#include "NTPClient.h"

#ifdef ENABLE_LED_MATRIX
	#include "MessageManager.h"
#endif


/* =======================================================================================
   Global variables
   ======================================================================================= */

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

unsigned long memory_max = 0;
unsigned long memory_min = 0;

unsigned long last_change = 0;
unsigned long now = 0;
unsigned long previousEpoch = 0;			  // Snapshot of epoch
WiFiManager wifiManager;
WiFiUDP ntpUDP;

	// By default 'pool.ntp.org' is used with 60 seconds update interval and no offset
	NTPClient timeClient(ntpUDP);

#ifdef ENABLE_LED_MATRIX
	MessageManager gMessageManager;
#endif

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

	soundBell(4);
	
	ticker.attach(0.6, tick);                   // Start flashing the internal LED to indicate we're in startup mode.
	
	Serial.begin(115200);
	
	displaySplash();
	
	//wifiManager.resetSettings();                                      // Debug purposes - Clears configuration.
	
#ifdef STATIC_IP
	wifiManager.setSTAStaticIPConfig(ip, gateway, subnet);
#endif
	wifiManager.setAPCallback(configModeCallback);                      // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setSaveConfigCallback(saveConfigCallback);              // Set callback that gets called when saving configuration
	wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT_SECONDS);  // Set the number of seconds that a user has to connect to the configuration portal before a reset occurs.
	wifiManager.setConnectTimeout(WIFI_TIMEOUT_SECONDS);                // Set the number of seconds allowed for WiFi to connect before the configuration portal is presented.
	wifiManager.setDebugOutput(false);                                  // Disables the debug output.
	
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

	// Print the IP address of the device to the serial port.
	Serial.printf("Connected to '%s' on channel %d.\n\n", WiFi.SSID().c_str(), WiFi.channel());

	Serial.println("===========================================");
	Serial.print("MAC Addr: ");
	Serial.println(WiFi.macAddress());
	Serial.print("IP Addr:  ");
	Serial.println(WiFi.localIP());
	Serial.print("Subnet:   ");
	Serial.println(WiFi.subnetMask());
	Serial.print("Gateway:  ");
	Serial.println(WiFi.gatewayIP());
	Serial.print("DNS Addr: ");
	Serial.println(WiFi.dnsIP());

	// Set a hostname
	String hostname = "JEEVES-" + WiFi.macAddress();
	hostname.replace(":", "");
	WiFi.hostname(hostname);
	Serial.print("Hostname: ");
	Serial.println(WiFi.hostname());
	Serial.println("===========================================\n");

	ticker.detach();
	digitalWrite(BUILTIN_LED, LOW);

	#ifdef ENABLE_LED_MATRIX
	gMessageManager.queueMessage( "IP Addr : " + WiFi.localIP().toString(), 2000, MessageManager::kHighPriority );
	gMessageManager.queueMessage( WiFi.hostname(), 2000, MessageManager::kHighPriority );
	#endif

	timeClient.begin();

delay(1000);

#ifdef ENABLE_WINTEST
	wintest.startListening();
#endif

#ifdef ENABLE_N1MM
	n1mm.startListening();
#endif

	Serial.println();
	
#ifdef ENABLE_AUDIBLE
	Serial.print("Relay enabled. Pin ");
	Serial.print(RELAY_PIN);
	Serial.print(", ");
	Serial.print(RELAY_TIMER_MS);
	Serial.println("ms.\n");
#endif	

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
	timeClient.update();

#ifdef ENABLE_LEDS
	ws2812fx.service();
#endif

#ifdef ENABLE_WINTEST
	wintest.service();
#endif

#ifdef ENABLE_N1MM
	n1mm.service();
#endif

#ifdef ENABLE_LEDS
  // Detect if the pattern has been running for the configured length of time and reset the pattern if it has.
  if (now - last_change > LED_TIMER_MS) {
    ws2812fx.setColor(0xFFFFFF);
    ws2812fx.setMode(FX_MODE_STATIC);
    last_change = now;
  }
#endif

#ifdef ENABLE_LED_MATRIX

 gMessageManager.processMessage();
 
#endif

  if (WiFi.status() == WL_IDLE_STATUS) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("Attempting to connect.");
	#ifdef ENABLE_LED_MATRIX
	//printText(0, MAX_DEVICES-1, "Attempting to connect.");
	#endif	
  }
  else if (WiFi.status() == WL_NO_SSID_AVAIL) {
    digitalWrite(INTERNAL_LED_PIN, true);       // Switch off the internal LED.
    Serial.println("No SSID available.");
	#ifdef ENABLE_LED_MATRIX
	//printText(0, MAX_DEVICES-1, "No SSID available.");
	#endif	
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

	//displayAvailableRAM();

}

void displayAvailableRAM()
{
	if(ESP.getFreeHeap() > memory_max || memory_max == 0) 
	{
		memory_max = ESP.getFreeHeap();
		Serial.printf("Available RAM: %d,  Max: %d,  Min: %d\n", ESP.getFreeHeap(), memory_max, memory_min);
	}
	if(ESP.getFreeHeap() < memory_min || memory_min == 0) 
	{
		memory_min = ESP.getFreeHeap();
		Serial.printf("Available RAM: %d,  Max: %d,  Min: %d\n", ESP.getFreeHeap(), memory_max, memory_min);
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
	Serial.println("Jeeves v1.4.0 - James Patterson (M1DST) - September 2018\n");
	Serial.println("An automated multiplier bell for amateur radio contests.\n");
	Serial.println("Website: http://www.m1dst.co.uk");
	Serial.println("Twitter: @m1dst\n\n");
	Serial.println("TODO: Add support for printing how long ago the last qso was.\n\n");
}


