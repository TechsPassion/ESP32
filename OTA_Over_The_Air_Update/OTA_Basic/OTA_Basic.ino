/*
 * TechsPassion - ESP32 Over The Air (OTA) Update, Basic
 * ----------------------------------------------------
 * Upload new code to your ESP32 over WiFi, no USB cable needed.
 *
 * HOW TO USE:
 * 1. Put your WiFi name and password below.
 * 2. Tools > Partition Scheme > "Default 4MB with spiffs" (any scheme with OTA works).
 * 3. Upload this sketch ONCE over USB, open the Serial Monitor (115200) and note the IP address.
 * 4. Change BLINK_MS (for example 1000 to 150) so you can see the new code arrive.
 * 5. Tools > Port > pick "techspassion-esp32 at 192.168.x.x" (Network ports), then Upload.
 *    The IDE asks for the OTA password below.
 *
 * Every sketch you upload must include the OTA code, or the next update needs USB again.
 */

#include <WiFi.h>        // WiFi connection
#include <ArduinoOTA.h>  // Receives new code over WiFi
#include "esp_ota_ops.h" // Tells us which partition (app0 or app1) is running

// WiFi credentials (replace with your actual credentials)
const char* ssid = "Your Wifi Name";
const char* password = "Your Wifi Password";

// Name and password the Arduino IDE shows / asks for when uploading over WiFi
const char* otaHostname = "techspassion-esp32";
const char* otaPassword = "techspassion";

// Change this between uploads to see the update happen
const char* FIRMWARE_VERSION = "1.0";
const int BLINK_MS = 1000;   // LED on/off time in milliseconds

const int ledPin = 2;        // Onboard LED on most ESP32 dev boards
unsigned long lastBlink = 0;
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(ledPin, OUTPUT);

  Serial.println();
  Serial.println("=== TechsPassion OTA Basic ===");
  Serial.print("Firmware version: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("Running from partition: ");
  Serial.println(esp_ota_get_running_partition()->label);  // app0 or app1

  // Connect to the WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // OTA setup
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(otaPassword);

  ArduinoOTA.onStart([]() {
    // Stop anything important here (motors, relays) before the update starts
    digitalWrite(ledPin, HIGH);
    Serial.println("OTA update started");
  });

  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    Serial.printf("Progress: %u%%\r", done * 100 / total);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println();
    Serial.println("OTA update finished, restarting into the new code...");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("wrong password");
    else if (error == OTA_BEGIN_ERROR) Serial.println("could not start (is the partition scheme OTA?)");
    else if (error == OTA_CONNECT_ERROR) Serial.println("connection failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("receive failed");
    else if (error == OTA_END_ERROR) Serial.println("file check failed");
  });

  ArduinoOTA.begin();
  Serial.print("Ready for OTA. In the Arduino IDE pick the port: ");
  Serial.println(otaHostname);
}

void loop() {
  // Must run often, so keep loop() free of long delay() calls
  ArduinoOTA.handle();

  // Blink without delay()
  if (millis() - lastBlink >= BLINK_MS) {
    lastBlink = millis();
    ledOn = !ledOn;
    digitalWrite(ledPin, ledOn ? HIGH : LOW);
  }
}
