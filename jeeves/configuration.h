#ifndef configuration_h
#define configuration_h

#define ENABLE_WINTEST                            // Comment out of you don't require Win-Test support.
#define ENABLE_N1MM                               // Comment out of you don't require N1MM+ support.

#define ENABLE_LEDS                               // Comment out of you don't require a visual alert (LEDs).
#define ENABLE_AUDIBLE                            // Comment out of you don't require an audible alert (bell).

#ifdef ENABLE_LEDS
#define LED_COUNT               300               // Change this to the number of LEDs in your string.
#define LED_PIN                 2                 // This is the pin the data line is connected to. (D4)
#define LED_TIMER_MS            2500              // The duration in ms that the pattern displays for.
#endif

#ifdef ENABLE_AUDIBLE
#define RELAY_PIN               5                 // This is the pin the relay line is connected to. (D1 to be compatible with the matching relay shield)
#define RELAY_TIMER_MS          25                // The duration in ms that the relay operates for.
#define RELAY_INVERTED                            // If you are driving a relay make sure you use a transistor!  If you are using a relay this should not be commented out.
                                                  // Comment out if you don't require the inversion such as if you are controlling a buzzer.
#endif

#define DISPLAY_EVERY_QSO                         // Comment out this line if you DO NOT want a visual alert when a QSO is logged.
//#define SOUND_EVERY_QSO                         // Comment out this line if you DO NOT want an audible alert when a QSO is logged.


#define WIFI_SSID               "yourSSID"          // Change this to match your SSID.
#define WIFI_PASSWORD           "yourPASSWORD"      // Change this to the passwod of your wifi network.

#define  N1MM_UDP_PORT          12060             // The default N1MM+ UDP broadcast port.  Change if you are not using defaults.
#define  WINTEST_UDP_PORT       9871              // The default Win-Test UDP broadcast port.  Change if you are not using defaults.

#endif
