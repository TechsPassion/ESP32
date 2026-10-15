/*
 * TechsPassion - ESP32 Over The Air (OTA) Upload
 * Upload once over USB, then upload new code over WiFi.
 */

#include <WiFi.h>
#include <ArduinoOTA.h>

// WiFi credentials (replace with your actual credentials)
const char* ssid = "Your Wifi Name";
const char* password = "Your Wifi Password";

const int ledPin = 2;      // Onboard LED (change if yours is on another pin)
const int blinkMs = 1000;  // Change this, then upload over WiFi to see the update

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start OTA: the board shows up in Tools > Port with this name
  ArduinoOTA.setHostname("techspassion-esp32");
  ArduinoOTA.setPassword("techspassion");  // The IDE asks for this when uploading
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();  // Listens for new code, keep loop() free of long delays

  // Blink without delay()
  digitalWrite(ledPin, (millis() / blinkMs) % 2);
}
