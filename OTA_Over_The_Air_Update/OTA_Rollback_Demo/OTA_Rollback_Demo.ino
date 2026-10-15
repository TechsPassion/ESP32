/*
 * TechsPassion - ESP32 OTA Update with Automatic Rollback
 * ------------------------------------------------------
 * Shows the two app partitions (app0 / app1) and what happens when a bad update arrives:
 * the ESP32 goes back to the last code that worked.
 *
 * HOW IT WORKS:
 * - A new update boots in a "trial" state (PENDING_VERIFY).
 * - This sketch runs a health check (here: did WiFi connect?).
 * - Passed: the update is marked VALID and stays.
 * - Failed or crashed before that: the ESP32 reboots and the bootloader starts the previous code.
 *
 * DEMO:
 * 1. Upload this sketch over USB with FIRMWARE_VERSION 1 (both SIMULATE options false).
 * 2. Set FIRMWARE_VERSION 2 and upload over WiFi: it runs from the other partition and is marked VALID.
 * 3. Set FIRMWARE_VERSION 3 and SIMULATE_CRASH true, upload over WiFi:
 *    version 3 crashes, the ESP32 reboots and you are back on version 2.
 * 4. Try again with SIMULATE_CRASH false and SIMULATE_HEALTH_CHECK_FAIL true:
 *    version 3 starts, fails its check, and rolls itself back to version 2.
 *
 * Tools > Partition Scheme > "Default 4MB with spiffs" (any scheme with OTA works).
 * Open the Serial Monitor at 115200 to watch it happen.
 */

#include <WiFi.h>
#include <ArduinoOTA.h>
#include "esp_ota_ops.h"

// WiFi credentials (replace with your actual credentials)
const char* ssid = "Your Wifi Name";
const char* password = "Your Wifi Password";

const char* otaHostname = "techspassion-esp32";
const char* otaPassword = "techspassion";

// Change these for each upload in the demo
const int FIRMWARE_VERSION = 1;
const bool SIMULATE_CRASH = false;              // version crashes right after it starts
const bool SIMULATE_HEALTH_CHECK_FAIL = false;  // version starts but fails its own check

const int ledPin = 2;
const unsigned long WIFI_TIMEOUT_MS = 20000;

// The Arduino core normally marks a new update as good before setup() runs.
// Returning true here means: "wait, my sketch will decide".
// extern "C" is needed because the core calls this from C code.
extern "C" bool verifyRollbackLater() {
  return true;
}

const char* stateName(esp_ota_img_states_t state) {
  switch (state) {
    case ESP_OTA_IMG_NEW:            return "NEW";
    case ESP_OTA_IMG_PENDING_VERIFY: return "PENDING_VERIFY (trial run)";
    case ESP_OTA_IMG_VALID:          return "VALID";
    case ESP_OTA_IMG_INVALID:        return "INVALID";
    case ESP_OTA_IMG_ABORTED:        return "ABORTED";
    default:                         return "UNDEFINED";
  }
}

void printPartitionInfo() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);
  esp_ota_img_states_t state;

  Serial.print("Running from:      ");
  Serial.println(running->label);
  Serial.print("Next update goes:  ");
  Serial.println(next->label);
  Serial.print("State:             ");
  if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
    Serial.println(stateName(state));
  } else {
    Serial.println("uploaded by USB (no OTA record yet)");
  }

  // After a rollback, the rejected partition is reported here
  const esp_partition_t* rejected = esp_ota_get_last_invalid_partition();
  if (rejected != NULL) {
    Serial.print("Last rejected update was in: ");
    Serial.println(rejected->label);
  }
}

bool isTrialRun() {
  esp_ota_img_states_t state;
  return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK
         && state == ESP_OTA_IMG_PENDING_VERIFY;
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(ledPin, OUTPUT);

  Serial.println();
  Serial.println("=== TechsPassion OTA Rollback Demo ===");
  Serial.print("Firmware version:  ");
  Serial.println(FIRMWARE_VERSION);
  printPartitionInfo();

  if (SIMULATE_CRASH) {
    Serial.println("Simulating a crash in 3 seconds...");
    delay(3000);
    abort();  // Crash on purpose: the ESP32 restarts and the bootloader rolls back
  }

  // Health check: the new code must be able to reach WiFi
  bool wifiOk = connectWiFi();
  bool healthy = wifiOk && !SIMULATE_HEALTH_CHECK_FAIL;

  if (isTrialRun()) {
    if (healthy) {
      esp_ota_mark_app_valid_cancel_rollback();
      Serial.println("Health check passed: this update is now VALID and will stay.");
    } else {
      Serial.println("Health check FAILED: rolling back to the previous version...");
      delay(1000);
      esp_ota_mark_app_invalid_rollback_and_reboot();  // Does not return if a previous version exists
      Serial.println("No previous version to roll back to.");
    }
  }

  if (!wifiOk) {
    Serial.println("No WiFi, restarting in 10 seconds.");
    delay(10000);
    ESP.restart();
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(otaPassword);
  ArduinoOTA.onStart([]() {
    digitalWrite(ledPin, HIGH);
    Serial.println("OTA update started");
  });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    Serial.printf("Progress: %u%%\r", done * 100 / total);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println();
    Serial.println("Update written. Restarting into it as a trial run...");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error [%u]\n", error);
  });
  ArduinoOTA.begin();
  Serial.println("Ready for the next OTA update.");
}

void loop() {
  ArduinoOTA.handle();

  // Blink the LED as many times as the version number, then pause
  static unsigned long last = 0;
  static int step = 0;
  unsigned long wait = (step < FIRMWARE_VERSION * 2) ? 200 : 1500;
  if (millis() - last >= wait) {
    last = millis();
    if (step < FIRMWARE_VERSION * 2) {
      digitalWrite(ledPin, step % 2 == 0 ? HIGH : LOW);
      step++;
    } else {
      digitalWrite(ledPin, LOW);
      step = 0;
    }
  }
}
