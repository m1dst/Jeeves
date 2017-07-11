#include "WS2812FX.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define LED_COUNT 300   // Change this to the number of lights in your string.
#define LED_PIN 2       // This is the pin the data line is connected to.
#define TIMER_MS 2500   // The duration in ms that the pattern displays for.
#define DISPLAY_EVERY_QSO  // Comment out this line if you DO NOT want a pattern to show for a non mult.

const char* ssid = "yourSSID";   // Change this to match your SSID.
const char* password = "yourPASSWORD";  // Change this to the passwod of your wifi network.

// Global variables
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiUDP Udp;
unsigned int localUdpPort = 12060;  // The default N1MM+ UDP broadcast port.  Change if you are not using defaults.
char incomingPacket[2000];
unsigned long last_change = 0;
unsigned long now = 0;

void setup()
{

    // Write the splash screen out to the serial port.
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println();
    Serial.println("Jeeves v1.0.0 - James Patterson (M1DST) - June 2017");
    Serial.println("Website: http://www.m1dst.co.uk");
    Serial.println("Twitter: @m1dst");

    // Setup the LED string to white with a brightness of about 60%
    ws2812fx.init();
    ws2812fx.setBrightness(150);
    ws2812fx.setSpeed(255);
    ws2812fx.setColor(0xFFFFFF);
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.start();
    ws2812fx.service();

    // Start connecting to the WIFI network.
    WiFi.begin(ssid, password);

    Serial.println("Connecting...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);

        if (WiFi.status() == WL_IDLE_STATUS) {
            Serial.println("Attempting to connect.");
        }
        else if (WiFi.status() == WL_NO_SSID_AVAIL) {
            Serial.println("No SSIS available.");
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
    Serial.println();

    // Report the IP address of the device to the serial port.
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    // Start listening to UDP traffic on the specified port.
    Udp.begin(localUdpPort);
}

void loop()
{
    now = millis();

    ws2812fx.service();

    int packetSize = Udp.parsePacket();
    if (packetSize) {

        // Report that a packet was received to the serial port. (for debug purposes)
        Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
        int len = Udp.read(incomingPacket, packetSize);
        if (len > 0) {
            incomingPacket[len] = 0;
        }

        // Display the contents of the packet (for debug purposes)
        //Serial.printf("UDP packet contents: %s\n", incomingPacket);
        //Serial.println();

        processN1MMPacket();
    }

    // Detect if the pattern has been running for the configured length of time and reset the pattern if it has.
    if (now - last_change > TIMER_MS) {
        ws2812fx.setColor(0xFFFFFF);
        ws2812fx.setMode(FX_MODE_STATIC);
        last_change = now;
    }
}

// Returns the value of an xml parameter from the supplied string
String xmlTakeParam(String inStr, String needParam)
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
void processN1MMPacket()
{
    char* output = NULL;
    char b[] = "<contactinfo>";
    output = strstr(incomingPacket, b);

    // Check if the packet received is an N1MM Plus ContactInfo broadcast.
    // http://n1mm.hamdocs.com/tiki-index.php?page=UDP+Broadcasts
    if (output) {
        String isMultiplier1 = xmlTakeParam(incomingPacket, "ismultiplier1");
        String isMultiplier2 = xmlTakeParam(incomingPacket, "ismultiplier2");
        String isMultiplier3 = xmlTakeParam(incomingPacket, "ismultiplier3");

        if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "0") {
            // Set the LEDs to be all on and red.
            ws2812fx.setColor(0xFF0000);
            ws2812fx.setMode(FX_MODE_STATIC);
            last_change = now;
        }
        else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "0") {
            // Set the LEDs to be all on and blue.
            ws2812fx.setColor(0x0000FF);
            ws2812fx.setMode(FX_MODE_STATIC);
            last_change = now;
        }
        else if (isMultiplier1 == "0" && isMultiplier2 == "0" && isMultiplier3 == "1") {
            // Set the LEDs to be all on and green.
            ws2812fx.setColor(0x00FF00);
            ws2812fx.setMode(FX_MODE_STATIC);
            last_change = now;
        }
        else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "0") {
            // Red & Blue
            ws2812fx.setMode(FX_MODE_RUNNING_RED_BLUE);
            last_change = now;
        }
        else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "1") {
            //Green & Blue
            ws2812fx.setMode(FX_MODE_RUNNING_GREEN_BLUE);
            last_change = now;
        }
        else if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "1") {
            // Red & Green
            ws2812fx.setMode(FX_MODE_MERRY_CHRISTMAS);
            last_change = now;
        }
        else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "1") {
            // Rainbow
            ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);
            last_change = now;
        }
        #ifdef DISPLAY_EVERY_QSO
        else {
            ws2812fx.setMode(FX_MODE_RUNNING_LIGHTS);
            last_change = now;
        }
        #else
        else {
            // Set the LEDs to be all on and white.
            ws2812fx.setColor(0xFFFFFF);
            ws2812fx.setMode(FX_MODE_STATIC);
            last_change = now;
        }
        #endif

    }
}
