#include "Arduino.h"
#include "n1mm.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include "NTPClient.h"

#ifdef ENABLE_LED_MATRIX
  #include "MessageManager.h"
  extern MessageManager gMessageManager;
#endif

// = Globals =======================================================
WiFiServer n1mmTcpServer(N1MM_PRIVATE_PORT);
WiFiClient *n1mmClients[N1MM_MAX_CLIENTS] = { NULL };
WiFiClient *n1mmOutputClients[N1MM_MAX_CLIENTS] = { NULL };
char n1mmInputBuffers[N1MM_MAX_CLIENTS][200] = { 0 };
extern NTPClient timeClient;
// =================================================================

//N1MM::N1MM()
//{
//}

void N1MM::startListening()
{
  _publicUdp.begin(N1MM_PUBLIC_PORT);
  _privateUdp.begin(N1MM_PRIVATE_PORT);
  n1mmTcpServer.begin();
  Serial.printf("%s enabled.  Listening on ports %d and %d\n", NAME, N1MM_PUBLIC_PORT, N1MM_PRIVATE_PORT);
}

void N1MM::stopListening()
{
  _publicUdp.stop();
  _privateUdp.stop();
  Serial.printf("%s disabled.\n", NAME);
}

void N1MM::service()
{

  if(millis() >= mMillisNextInternalBroadcastAllowed)
  {
    sendInternalBroadcast();
  }

  serviceTcpConnections();

  int packetSize = _publicUdp.parsePacket();
  if (packetSize) {

    // Report that a packet was received to the serial port. (for debug purposes)
    Serial.printf("Received %d bytes from %s, port %d (%s)\n", packetSize, _publicUdp.remoteIP().toString().c_str(), _publicUdp.remotePort(), NAME);

    char incomingPacket[2000];
    int len = _publicUdp.read(incomingPacket, packetSize);
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    // Display the contents of the packet (for debug purposes)
    //Serial.printf("UDP packet contents: %s\n\n", incomingPacket);

    routeUdpPacket(incomingPacket);
  }

  packetSize = _privateUdp.parsePacket();
  if (packetSize) {

    // Report that a packet was received to the serial port. (for debug purposes)
    Serial.printf("Received %d bytes from %s, port %d (%s)\n", packetSize, _privateUdp.remoteIP().toString().c_str(), _privateUdp.remotePort(), NAME);

    char incomingPacket[2000];
    int len = _privateUdp.read(incomingPacket, packetSize);
    if (len > 0) {
      incomingPacket[len] = 0;
    }

    // Display the contents of the packet (for debug purposes)
    //Serial.printf("UDP packet contents: %s\n\n", incomingPacket);

    routeUdpPacket(incomingPacket);
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

// Returns the value of an xml parameter from the supplied string
String N1MM::xmlTakeParamWithAttributes(String inStr, String needParam)
{
  if (inStr.indexOf("<" + needParam) > -1) {
    int CountChar = needParam.length();
    int indexStart = inStr.indexOf(">") + 1;
    int indexStop = inStr.indexOf("</" + needParam + ">");
    return inStr.substring(indexStart, indexStop);
  }
  return "";
}

String N1MM::xmlTakeAttribute(String inStr, String needAttribute)
{
  if (inStr.indexOf(needAttribute + "=\"") > 0) {
    int CountChar = needAttribute.length();
    int indexStart = inStr.indexOf(needAttribute + "=\"") + CountChar + 2;
    int indexStop = inStr.indexOf("\"", indexStart);
    return inStr.substring(indexStart, indexStop);
  }
  return ".";
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::routeUdpPacket(char * incomingPacket)
{
  if (strstr(incomingPacket, "<dynamicresults>")) {
    processScorePacket(incomingPacket);
  }
  else if (strstr(incomingPacket, "<contactinfo>")) {
    processQsoPacket(incomingPacket);
  }
  else if (strstr(incomingPacket, "<RadioInfo>")) {
    processRadioInfoPacket(incomingPacket);
  }
  else if (countOccurances(incomingPacket, '%') == 3)
  {
      processBeaconPacket(incomingPacket);
  }
  else
  {
    Serial.println("Unroutable packet received.\n");
  }
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processBeaconPacket(char * incomingPacket)
{
  char *machineName;
  char *ipAddress;
  char *port;
  const char deliminator[2] = "%";
  machineName = strtok(incomingPacket, deliminator);
  ipAddress = strtok(NULL, deliminator);
  port = strtok(NULL, deliminator);
  Serial.printf("BEACON (%s) - %s - %s - %s\n\n", NAME, machineName, ipAddress, port);
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processQsoPacket(char * incomingPacket)
{
  char b[] = "<contactinfo>";
  char* output = strstr(incomingPacket, b);

  // Check if the packet received is an N1MM Plus ContactInfo broadcast.
  // http://n1mm.hamdocs.com/tiki-index.php?page=UDP+Broadcasts
  if (output) {
    String isMultiplier1 = xmlTakeParam(incomingPacket, "ismultiplier1");
    String isMultiplier2 = xmlTakeParam(incomingPacket, "ismultiplier2");
    String isMultiplier3 = xmlTakeParam(incomingPacket, "ismultiplier3");
    String callsign = xmlTakeParam(incomingPacket, "call");
    String band = xmlTakeParam(incomingPacket, "band");
    String mode = xmlTakeParam(incomingPacket, "mode"); mode.replace("USB", "SSB"); mode.replace("LSB", "SSB");
    String op = xmlTakeParam(incomingPacket, "operator");
    String message = "MULT " + isMultiplier1 + ":" + isMultiplier2 + ":" + isMultiplier3 + " - " + band + mode;
    String message2 = callsign + " by " + op;

    if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "0") {
      Serial.printf("MULT 1:0:0 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and red.
        ws2812fx.setColor(0xFF0000);
        ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "0") {
      Serial.printf("MULT 0:1:0 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and blue.
        ws2812fx.setColor(0x0000FF);
        ws2812fx.setMode(FX_MODE_STATIC);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "0" && isMultiplier3 == "1") {
      Serial.printf("MULT 0:0:1 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Set the LEDs to be all on and green.
        ws2812fx.setColor(0x00FF00);
        ws2812fx.setMode(FX_MODE_STATIC);
      #endif
        #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "0") {
      Serial.printf("MULT 1:1:0 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Red & Blue
        ws2812fx.setMode(FX_MODE_RUNNING_RED_BLUE);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "0" && isMultiplier2 == "1" && isMultiplier3 == "1") {
      Serial.printf("MULT 0:1:1 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Green & Blue
        ws2812fx.setMode(FX_MODE_RUNNING_GREEN_BLUE);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "0" && isMultiplier3 == "1") {
      Serial.printf("MULT 1:0:1 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Red & Green
        ws2812fx.setMode(FX_MODE_MERRY_CHRISTMAS);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif      
      soundBell(1);
      last_change = now;
    }
    else if (isMultiplier1 == "1" && isMultiplier2 == "1" && isMultiplier3 == "1") {
      Serial.printf("MULT 1:1:1 (%s)\n\n", NAME);
      #ifdef ENABLE_LEDS
        // Rainbow
        ws2812fx.setMode(FX_MODE_CHASE_RAINBOW);
      #endif
      #ifdef ENABLE_LED_MATRIX
        gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
        gMessageManager.queueMessage( message2, 5000, MessageManager::kHighPriority );
      #endif
      soundBell(1);
      last_change = now;
    }
#ifdef DISPLAY_EVERY_QSO || SOUND_EVERY_QSO
    else {
#ifdef DISPLAY_EVERY_QSO
      #ifdef ENABLE_LEDS
      ws2812fx.setMode(FX_MODE_RUNNING_LIGHTS);
      #endif
      #ifdef ENABLE_LED_MATRIX
      message = "QSO: " + callsign + " - " + band + mode;
      gMessageManager.queueMessage( message, 5000, MessageManager::kHighPriority );
      #endif
#endif
#ifdef SOUND_EVERY_QSO
      soundBell(1);
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
      last_change = now;
    }
#endif

  }
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processRadioInfoPacket(char * incomingPacket)
{
  //Serial.printf("RADIO INFO (%s)\n\n", NAME);    
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processScorePacket(char * incomingPacket)
{
  if(millis() >= mMillisNextScorePacketAllowed)
  {
    Serial.printf("SCORE (%s)\n", NAME);
    mMillisNextScorePacketAllowed = millis() + MINIMUM_INTERVAL_BETWEEN_SCORE_UPDATES;

    soundBell(2);

    gMessageManager.queueMessage( "* * CURRENT SCORE * *", 4000, MessageManager::kLowPriority );

    String breakdown = xmlTakeParam(incomingPacket, "breakdown");

    if (breakdown.indexOf("<qso band=\"total\" mode=\"ALL\">") > 0) {
      int indexStart = breakdown.indexOf("<qso band=\"total\" mode=\"ALL\">") + 29;
      int indexStop = breakdown.indexOf("</qso>", indexStart );
      String qsos = breakdown.substring(indexStart, indexStop);
      String message = "QSOS: " + qsos;
      Serial.println(message);
      gMessageManager.queueMessage( message, 4000, MessageManager::kLowPriority );
    }

    int positionOfTagStart = 0;
    int positionOfTagEnd = 0;

    while (positionOfTagStart != -1) {
      positionOfTagStart = breakdown.indexOf("<mult ", positionOfTagEnd);
      positionOfTagEnd = breakdown.indexOf("</mult>", positionOfTagStart) + 7;
      if (positionOfTagStart > -1) {
        String element = breakdown.substring(positionOfTagStart, positionOfTagEnd);
        String band = xmlTakeAttribute(element, "band"); band.toUpperCase();
        String mode = xmlTakeAttribute(element, "mode"); mode.toUpperCase();
        String type = xmlTakeAttribute(element, "type"); type.toUpperCase();
        String count = xmlTakeParamWithAttributes(element, "mult");
        Serial.printf("MULT ---> Band: %s, Mode: %s, Type: %s, Count: %s\n", band.c_str(), mode.c_str(), type.c_str(), count.c_str());

        if(band == "TOTAL")
        {
          String message = "MULTS: " + count + " (" + type + ")";
          gMessageManager.queueMessage( message, 4000, MessageManager::kLowPriority );
        }

      }
    }

    String score = xmlTakeParam(incomingPacket, "score");
    String message = "SCORE: " + score;
    Serial.println(message); Serial.println("\n");
    gMessageManager.queueMessage( message, 4000, MessageManager::kLowPriority );

  }
  else
  {
    Serial.printf("SCORE (%s) - Ignoring due to MINIMUM_INTERVAL_BETWEEN_SCORE_UPDATES.\n\n", NAME);
  }
}

// Checks the packet contents and sets the LED pattern based on the multiplier status.
void N1MM::processSpotPacket(char * incomingPacket)
{
  Serial.printf("SPOT (%s)\n\n", NAME);  
}

int N1MM::countOccurances(char *str, char character)
{
    const char *p = str;
    int count = 0;

    do {
        if (*p == character)
            count++;
    } while (*(p++));

    return count;
}

void N1MM::sendInternalBroadcast()
{
  
  char packetBuffer[100] = { 0 };
  strcat(packetBuffer, WiFi.hostname().c_str());
  strcat(packetBuffer, "%");
  sprintf(packetBuffer + strlen(packetBuffer), "%d.", WiFi.localIP()[0]);
  sprintf(packetBuffer + strlen(packetBuffer), "%d.", WiFi.localIP()[1]);
  sprintf(packetBuffer + strlen(packetBuffer), "%d.", WiFi.localIP()[2]);
  sprintf(packetBuffer + strlen(packetBuffer), "%d", WiFi.localIP()[3]);
  strcat(packetBuffer, "%");
  sprintf(packetBuffer + strlen(packetBuffer), "%d", N1MM_PRIVATE_PORT);
  strcat(packetBuffer, "%");

  //Serial.println(packetBuffer);

  IPAddress broadcastIp(255,255,255,255);
  _privateUdp.beginPacket(broadcastIp, N1MM_PRIVATE_PORT);
  _privateUdp.write(packetBuffer);
  _privateUdp.endPacket();

  mMillisNextInternalBroadcastAllowed = millis() + N1MM_BROADCAST_INTERVAL_MS;

}

void N1MM::serviceTcpConnections()
{

  // Check if a new client has connected
  WiFiClient newClient = n1mmTcpServer.available();
  
  if (newClient) {
    Serial.printf("New client connected : %s (%s)\n", newClient.remoteIP().toString().c_str(), NAME);
    
    // Find the first unused space
    for (int i = 0; i < N1MM_MAX_CLIENTS; ++i) {
        if (NULL == n1mmClients[i]) {
            n1mmClients[i] = new WiFiClient(newClient);
            n1mmOutputClients[i] = new WiFiClient();
            n1mmOutputClients[i] -> connect(n1mmClients[i] -> remoteIP().toString().c_str(), N1MM_PRIVATE_PORT);

            memset(n1mmInputBuffers[i], '\0', sizeof n1mmInputBuffers[i]);
            break;
        }
     }
  }

  // Check whether each client has some data
  for (int i = 0; i < N1MM_MAX_CLIENTS; ++i) {
    // If the client is in use, and has some data...
    if (NULL != n1mmClients[i] && n1mmClients[i]->available() ) {
      // Read the data 
      char newChar = n1mmClients[i]->read();

      // If we have the end of a string
      if ('\r' == newChar) {

        // Empty the string for next time
        memset(n1mmInputBuffers[i], '\0', sizeof n1mmInputBuffers[i]);

        // If you want to disconnect the client here, then do this:
        n1mmClients[i]->stop();
        delete n1mmClients[i];
        n1mmClients[i] = NULL;

      } else {
        
        // Add it to the buffer
        append(n1mmInputBuffers[i], newChar);

        //Serial.printf("%s - %d\n", n1mmInputBuffers[i], strlen(n1mmInputBuffers[i]));

        if(strcmp(&n1mmInputBuffers[i][strlen(n1mmInputBuffers[i])-6], "__DATA") == 0)
        {
          //Serial.println(n1mmInputBuffers[i]);
          routeTcpPacket(i, n1mmInputBuffers[i]);

          // Clean the buffer.
          memset(n1mmInputBuffers[i], '\0', sizeof n1mmInputBuffers[i]);
        }

        // TODO: Nothing stops this from overrunning the string and
        // trashing the memory. We SHOULD guard against this.
      }
    }
  }
}

void N1MM::routeTcpPacket(int clientPosition, char *incomingPacket)
{

  if (strstr(incomingPacket, "ECHOREQ")) {
    processEchoRequestPacket(clientPosition, incomingPacket);
  } else if (strstr(incomingPacket, "MASTER")) {
    processMasterPacket(incomingPacket);
  } else if (strstr(incomingPacket, "STATUS")) {
    processStatusPacket(incomingPacket);
  } else if (strstr(incomingPacket, "TALK")) {
    processTalkPacket(incomingPacket);
  }
  else {
    Serial.println("Unroutable packet received.");
    Serial.println(incomingPacket);
  }

}

void N1MM::processEchoRequestPacket(int clientPosition, char * incomingPacket) {
  // Example packet content
  // DATA__00%DESKTOP-3QCI0Q9%ECHOREQ%2018-10-05%21:11:50%~__DATA

  // Check we actually have an echo request.
  if (strstr(incomingPacket, "ECHOREQ")) {
    //Serial.print("ECHO REQUEST: ");
    //Serial.println(incomingPacket);

    // Get the first token and validate it matches the start of a normal packet.
    char * token = strtok(incomingPacket, deliminator);

    if (strcmp(token, "DATA__00") != 0) {
      Serial.println("INVALID PACKET. Discarding.");
      return;
    }

    char *station = strtok(NULL, deliminator);
    char *cmd = strtok(NULL, deliminator);
    char *date = strtok(NULL, deliminator);
    char *time = strtok(NULL, deliminator);

    //Serial.printf( "%s - %s - %s - %s\n", station, cmd, date, time );

    sendEchoResponse(clientPosition);

  }

}

void N1MM::processMasterPacket(char * incomingPacket) {
  // Example packet content
  // DATA__00%DESKTOP-3QCI0Q9%MASTER%DESKTOP-3QCI0Q9%~__DATA

  // Check we actually have an echo request.
  if (strstr(incomingPacket, "MASTER")) {
    //Serial.print("MASTER ANNOUNCEMENT: ");
    //Serial.println(incomingPacket);

    // Get the first token and validate it matches the start of a normal packet.
    char *token = strtok(incomingPacket, deliminator);

    if (strcmp(token, "DATA__00") != 0) {
      Serial.println("INVALID PACKET. Discarding.");
      return;
    }

    char * station = strtok(NULL, deliminator);
    char * cmd = strtok(NULL, deliminator);

    //Serial.printf( "%s - %s\n", station, cmd );

  }

}

void N1MM::processStatusPacket(char * incomingPacket) {
  // Example packet content
  // DATA__00%DESKTOP-3QCI0Q9%STATUS%1420000%1420000%0%M1N%0%0%SB%SINGLE-OP%ONE%VK9W%1.0.7295.0%~__DATA

  // Check we actually have an echo request.
  if (strstr(incomingPacket, "STATUS")) {
    //Serial.print("STATUS: ");
    //Serial.println(incomingPacket);

    // Get the first token and validate it matches the start of a normal packet.
    char * token = strtok(incomingPacket, deliminator);

    if (strcmp(token, "DATA__00") != 0) {
      Serial.println("INVALID PACKET. Discarding.");
      return;
    }

    char * station = strtok(NULL, deliminator);
    char * cmd = strtok(NULL, deliminator);
    char * vfoA = strtok(NULL, deliminator);
    char * vfoB = strtok(NULL, deliminator);
    char * unknown1 = strtok(NULL, deliminator);
    char * callsign = strtok(NULL, deliminator);
    char * unknown2 = strtok(NULL, deliminator);
    char * unknown3 = strtok(NULL, deliminator);
    char * unknown4 = strtok(NULL, deliminator);
    char * operatorStyle = strtok(NULL, deliminator);
    char * transmitter = strtok(NULL, deliminator);
    char * lastStationWorked = strtok(NULL, deliminator);
    char * version = strtok(NULL, deliminator);

    //Serial.printf( "%s - %s - %s\n", station, cmd, version );

  }

}

void N1MM::processTalkPacket(char * incomingPacket) {
  // Example packet content
  // DATA__00%DESKTOP-3QCI0Q9%TALK%[DESKTOP-3QCI0Q9] Hello world!%~__DATA

  // Check we actually have an echo request.
  if (strstr(incomingPacket, "TALK")) {
    //Serial.print("TALK: ");
    //Serial.println(incomingPacket);

    // Get the first token and validate it matches the start of a normal packet.
    char *token = strtok(incomingPacket, deliminator);

    if (strcmp(token, "DATA__00") != 0) {
      Serial.println("INVALID PACKET. Discarding.");
      return;
    }

    char *station = strtok(NULL, deliminator);
    char *cmd = strtok(NULL, deliminator);
    char *msg = strtok(NULL, deliminator);

    // Check if this is an N1MM assistance request packet.
    if (strstr(msg, "needs assistance.")) {

      for( int i = 0; i < 5; i++ )
      {
        gMessageManager.queueMessage( station, 1000, MessageManager::kHighPriority, false );
        gMessageManager.queueMessage( "NEEDS ASSISTANCE!", 1000, MessageManager::kHighPriority, false );
      }

    }
    else
    {
      String message = String(msg);
      gMessageManager.queueMessage( msg, (220 * message.length()), MessageManager::kLowPriority );
      Serial.printf( "MSG: %s\n", msg );
    }

    soundBell(4);

  }

}

void N1MM::sendEchoResponse(int clientPosition) {
  // Example packet content
  // DATA__00%DESKTOP-3QCI0Q9%ECHO%2018-10-04%16:12:11%~__DATA

    Serial.printf("Sending response to echo from %s. (%s)\n", n1mmOutputClients[clientPosition] -> remoteIP().toString().c_str(), NAME);

    char date[20];
    timeClient.getFormattedDate().toCharArray(date, 20);
    date[10] = '%';
    date[19] = 0;

    String s = String("DATA__00%") + WiFi.hostname() + "%ECHO%" + date + "%~__DATA";
    //Serial.println(s);
    n1mmOutputClients[clientPosition] -> print(s);
    
    //s = String("DATA__00%") + WiFi.hostname() + "%STATUS%1420000%1420000%0%JEEVES%0%0%SB%SINGLE-OP%ONE%VK9W%1.0.7295.0%~__DATA";
    //Serial.println(s);
    //n1mmOutputClients[clientPosition] -> print(s);

    //s = String("DATA__00%") + WiFi.hostname() + "%LASTQAT%2018-10-04 08:18:41%~__DATA";
    //Serial.println(s);
    //n1mmOutputClients[clientPosition] -> print(s);

}