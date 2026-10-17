// WiFi details for Servo_OTA. The sketch first reuses the network already saved
// on the ESP32 from the previous upload; fill these in only if you need them.
#pragma once

#define WIFI_SSID     "YOUR_WIFI_NAME"       // 2.4 GHz network
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Hotspot the board opens when it cannot join any WiFi (OTA then works at 192.168.4.1)
#define AP_SSID       "techspassion-esp32"
#define AP_PASSWORD   "techspassion"
