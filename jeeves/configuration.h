#ifndef configuration_h
#define configuration_h

#include <Arduino.h>

#define ENABLE_LED_MATRIX                         // Comment out of you don't require an LED display matrix.

#define ENABLE_WINTEST                            // Comment out of you don't require Win-Test support.
#define ENABLE_N1MM                               // Comment out of you don't require N1MM+ support.

#define ENABLE_LEDS                               // Comment out of you don't require a visual alert (LEDs).
#define ENABLE_AUDIBLE                            // Comment out of you don't require an audible alert (bell).

#define CONFIG_PIN              0                // This is the pin the to enter config state. (Tactile Switch)
#define INTERNAL_LED_PIN        2               // This is the pin connected to the internal LED (D4).

#ifdef ENABLE_LEDS
#define LED_COUNT               300               // Change this to the number of LEDs in your string.
#define LED_PIN                 4                 // This is the pin the data line is connected to. (D2)
#define LED_TIMER_MS            2500              // The duration in ms that the pattern displays for.
#endif

#ifdef ENABLE_LED_MATRIX
	#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
	#define MAX_DEVICES 16

	#define LED_MATRIX_CLK_PIN      14                // SCK D5 BLACK
	#define LED_MATRIX_DATA_PIN     13                // MOSI D7 WHITE
	#define LED_MATRIX_CS_PIN       15                // SS D8 GREY
	#define CHAR_SPACING			1					// pixels between characters
#endif

#ifdef ENABLE_AUDIBLE
#define RELAY_PIN               5                 // This is the pin the relay line is connected to. (D1 to be compatible with the matching relay shield)
#define RELAY_TIMER_MS          100                // The duration in ms that the relay operates for.
//#define RELAY_INVERTED                            // If you are driving a relay make sure you use a transistor!  If you are using a relay this should not be commented out.
                                                  // Comment out if you don't require the inversion such as if you are controlling a buzzer.
#endif

#define DISPLAY_EVERY_QSO                         // Comment out this line if you DO NOT want a visual alert when a QSO is logged.
//#define SOUND_EVERY_QSO                         // Comment out this line if you DO NOT want an audible alert when a QSO is logged.

#define  MINIMUM_INTERVAL_BETWEEN_SCORE_UPDATES		600000		// The default number of seconds which must have passed before a score packet is reacted to.  600,000 = 10 minutes.
#define  WINTEST_UDP_PORT       					9871        // The default Win-Test UDP broadcast port.  Change if you are not using defaults.

// = N1MM Settings =================================================
#define  N1MM_PUBLIC_PORT   						12060   	// The default N1MM+ UDP broadcast port.  Change if you are not using defaults.
#define  N1MM_PRIVATE_PORT  						12070       // The default N1MM+ UDP broadcast port.  Change if you are not using defaults.
#define  N1MM_BROADCAST_INTERVAL_MS					20000       // The interval in milliseconds between UDP broadcasts.  The N1MM default is 20000 (20s)
#define  N1MM_MAX_CLIENTS							10			// The maximum number of N1MM instances you will be running on the network.
// =================================================================


#define  CONFIG_PORTAL_TIMEOUT_SECONDS       		180			// Set the number of seconds that a user has to connect to the configuration portal before a reset occurs.
#define  WIFI_TIMEOUT_SECONDS                		60   		// Set the number of seconds allowed for WiFi to connect before the configuration portal is presented.

//#define  STATIC_IP                              // Enable this if you want to use a static ip address.  You will need to configure the addresses in the main .ino file.

#endif
