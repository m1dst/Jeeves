#ifndef configuration_h
#define configuration_h

#define ENABLE_WINTEST                            // Comment out of you don't require Win-Test support.
#define ENABLE_N1MM                               // Comment out of you don't require N1MM+ support.

#define ENABLE_LEDS                               // Comment out of you don't require a visual alert (LEDs).
#define ENABLE_AUDIBLE                            // Comment out of you don't require an audible alert (bell).

#define CONFIG_PIN              0                // This is the pin the to enter config state. (Tactile Switch)
#define INTERNAL_LED_PIN        13               // This is the pin connected to the internal LED.

#ifdef ENABLE_LEDS
#define LED_COUNT               300               // Change this to the number of LEDs in your string.
#define LED_PIN                 14                // This is the pin the data line is connected to. (J1 Pin 5)
#define LED_TIMER_MS            2500              // The duration in ms that the pattern displays for.
#endif

#ifdef ENABLE_AUDIBLE
#define RELAY_PIN               12                // This is the pin the relay line is connected to. (D1 to be compatible with the matching relay shield)
#define RELAY_TIMER_MS          25                // The duration in ms that the relay operates for.
#endif

#define DISPLAY_EVERY_QSO                         // Comment out this line if you DO NOT want a visual alert when a QSO is logged.
//#define SOUND_EVERY_QSO                         // Comment out this line if you DO NOT want an audible alert when a QSO is logged.

#define  N1MM_UDP_PORT          12060             // The default N1MM+ UDP broadcast port.  Change if you are not using defaults.
#define  WINTEST_UDP_PORT       9871              // The default Win-Test UDP broadcast port.  Change if you are not using defaults.

#define  CONFIG_PORTAL_TIMEOUT_SECONDS       180  // Set the number of seconds that a user has to connect to the configuration portal before a reset occurs.
#define  WIFI_TIMEOUT_SECONDS                60   // Set the number of seconds allowed for WiFi to connect before the configuration portal is presented.

//#define  STATIC_IP                              // Enable this if you want to use a static ip address.  You will need to configure the addresses in the main .ino file.

#endif
