#ifndef N1MM_h
#define N1MM_h

#include "Arduino.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>

class N1MM
{
  public:
    N1MM();
    void startListening();
    void stopListening();
    void service();
  private:
    const char* NAME = "N1MM";
    WiFiUDP _wifiUdp;
    
    void processPacket(char * incomingPacket);
    String xmlTakeParam(String inStr, String needParam);
};

#endif
