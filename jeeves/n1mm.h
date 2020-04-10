#ifndef N1MM_h
#define N1MM_h

#include "Arduino.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>

class N1MM
{
  public:

    //N1MM();

    void startListening();
    void stopListening();
    void service();

  private:
    const char* NAME = "N1MM";
    const char deliminator[2] = "%";

    WiFiUDP _publicUdp;
    WiFiUDP _privateUdp;

    int mMillisNextScorePacketAllowed = 0;
    int mMillisNextInternalBroadcastAllowed = 0;

    void processBeaconPacket(char *incomingPacket);
    void processEchoRequestPacket(int clientPosition, char *incomingPacket);
    void processMasterPacket(char *incomingPacket);
    void processQsoPacket(char *incomingPacket);
    void processRadioInfoPacket(char *incomingPacket);
    void processScorePacket(char *incomingPacket);
    void processSpotPacket(char *incomingPacket);
    void processStatusPacket(char *incomingPacket);
    void processTalkPacket(char *incomingPacket);
    void routeUdpPacket(char *incomingPacket);
    void routeTcpPacket(int clientPosition, char * incomingPacket);
    void sendInternalBroadcast();
    void sendEchoResponse(int clientPosition);

    void serviceTcpConnections();

    String xmlTakeParam(String inStr, String needParam);
    String xmlTakeParamWithAttributes(String inStr, String needParam);
    String xmlTakeAttribute(String inStr, String needAttribute);
    int countOccurances(char *str, char character);

};

#endif
