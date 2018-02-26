#ifndef WinTest_h
#define WinTest_h

#include "Arduino.h"
#include "configuration.h"
#include "globals.h"
#include <WiFiUdp.h>

class WinTest
{
  public:
    WinTest();
    void startListening();
    void stopListening();
    void service();
  private:

    const char* NAME = "Win-Test";
    WiFiUDP _wifiUdp;

    bool isWarm = false;

    unsigned int previousCountOfQsos = 0;
    unsigned int previousCountOfMult1 = 0;
    unsigned int previousCountOfMult2 = 0;
    unsigned int previousCountOfMult3 = 0;
    unsigned int previousCountOfDupe = 0;
    unsigned int previousPointsNet = 0;

    int mult1Position = 0;   // Position within the data array of the first multiplier.
    int mult2Position = 0;   // Position within the data array of the first multiplier.
    int mult3Position = 0;   // Position within the data array of the first multiplier.

    // An array of valid identifiers which Win-Test see as a multiplier.
    unsigned int validMultipliers [31] = {7, 8, 9, 10, 39, 40, 11, 12, 13, 20, 23, 24, 46, 47, 25, 28, 30, 31, 32, 33, 43, 44, 41, 42, 54, 48, 50, 51, 52, 53, 55};

    void processPacket(char * incomingPacket);
    bool isHeaderMultiplier(int id);
};

#endif
