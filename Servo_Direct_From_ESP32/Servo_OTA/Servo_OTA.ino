/*
 * TechsPassion - Servo straight from an ESP32, uploaded Over The Air
 *
 * Same servo code as Servo_Basic plus ArduinoOTA, so after the first USB
 * upload every new version goes over WiFi (Tools > Port > techspassion-esp32).
 * Builds on the OTA_Over_The_Air_Update project in this repo.
 *
 * Wiring (the servo gets its power from its own 5-6 V supply, not the board):
 *   servo brown/black -> supply (-) AND an ESP32 GND pin
 *   servo red         -> supply (+)
 *   servo orange      -> GPIO 14
 *
 * WiFi: the ESP32 remembers the last network it joined, so WiFi.begin() with
 * no name or password reconnects to it. If that fails it tries secrets.h, and
 * if that fails too it opens its own hotspot so OTA still works (192.168.4.1).
 *
 * Onboard LED (GPIO 48 on ESP32-S3 boards): green = on WiFi, blue = hotspot,
 * amber while an update is being written, red if an update failed.
 */

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h"

const char* otaName     = "techspassion-esp32";
const char* otaPassword = "techspassion";

const int servoPin = 14;
const int ledPin   = 48;

// Pulse width at the two end stops. 500-2400 us suits SG90/MG90S; use
// 1000-2000 if your servo buzzes at the ends.
const int servoMinUs = 500;
const int servoMaxUs = 2400;

const int servoFreq = 50;   // Hz, one pulse every 20 ms
const int servoRes  = 14;   // bits of resolution for the duty cycle

int  angle    = 90;
int  target   = 180;
bool sweeping = true;
const int stepMs = 15;      // ms per degree: 15 ms = 2.7 s for a full sweep
unsigned long lastStep = 0;

void servoWrite(int deg) {
  deg = constrain(deg, 0, 180);
  int us = map(deg, 0, 180, servoMinUs, servoMaxUs);
  uint32_t duty = (uint32_t)us * ((1UL << servoRes) - 1) / (1000000UL / servoFreq);
  ledcWrite(servoPin, duty);
}

bool waitForWiFi() {
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(otaName);

  Serial.print("WiFi: trying the network saved on the board");
  WiFi.begin();                               // stored name and password
  bool ok = waitForWiFi();

  if (!ok && strcmp(WIFI_SSID, "YOUR_WIFI_NAME") != 0) {
    Serial.printf("WiFi: trying %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    ok = waitForWiFi();
  }

  if (ok) {
    Serial.printf("WiFi: connected to %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("WiFi: no network found. Hotspot %s at %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nServo on GPIO 14 with OTA");

  ledcAttach(servoPin, servoFreq, servoRes);
  servoWrite(angle);

  connectWiFi();

  ArduinoOTA.setHostname(otaName);
  ArduinoOTA.setPassword(otaPassword);
  ArduinoOTA.onStart([]() { sweeping = false; servoWrite(90); rgbLedWrite(ledPin, 40, 20, 0); });
  ArduinoOTA.onEnd([]()   { rgbLedWrite(ledPin, 0, 40, 0); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("OTA error %u\n", e); rgbLedWrite(ledPin, 40, 0, 0); });
  ArduinoOTA.begin();
  Serial.println("OTA ready. Type 0-180 for an angle, s to pause/resume the sweep.");
}

void loop() {
  ArduinoOTA.handle();      // keep loop() free of long delays so uploads get through

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "s") {
      sweeping = !sweeping;
      Serial.println(sweeping ? "sweep on" : "sweep off");
    } else if (cmd.length()) {
      sweeping = false;
      angle = constrain(cmd.toInt(), 0, 180);
      servoWrite(angle);
      Serial.printf("angle %d\n", angle);
    }
  }

  if (sweeping && millis() - lastStep >= stepMs) {
    lastStep = millis();
    angle += (target > angle) ? 1 : -1;
    servoWrite(angle);
    if (angle == target) target = (target == 180) ? 0 : 180;
  }

  // LED: green on WiFi, blue on the hotspot; blinks while the servo sweeps
  bool on = !sweeping || (millis() / 500) % 2;
  if (WiFi.status() == WL_CONNECTED) rgbLedWrite(ledPin, 0, on ? 30 : 0, 0);
  else                               rgbLedWrite(ledPin, 0, 0, on ? 30 : 0);
}
