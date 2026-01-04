#include <WiFiManager.h>
#include <WiFi.h>  // ESP32 WiFi
#include <ESPmDNS.h>

WiFiServer server(73);

// Pin assignments
const int RELAY_PIN = 16;  // Relay output
const int LED_PIN = 23;    // LED output
const int RESET_PIN = 22;  // Reset input

void pulseRelayAndLED() {
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(50);  // 50ms pulse
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(RESET_PIN, INPUT_PULLUP);

  // WiFiManager handles captive portal setup
  WiFiManager wm;

  // Check if button is held during boot
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("WiFi reset button held at boot — clearing WiFi settings");
    wm.resetSettings();
    delay(500);
    ESP.restart();
  }

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  wm.autoConnect("Jeeves");

  Serial.println("Connected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("jeeves")) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println("mDNS responder started: jeeves.local");
  }

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    Serial.println("TCP connection received on port 73");

    // Trigger relay + LED pulse
    pulseRelayAndLED();

    // Clear incoming data
    while (client.available()) client.read();

    client.stop();
  }
}