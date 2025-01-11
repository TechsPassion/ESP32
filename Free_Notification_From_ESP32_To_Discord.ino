#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "Wifi Name"; // Your Wi-Fi SSID
const char* password = "Wifi Passowrd"; // Your Wi-Fi Password

// Discord Webhook URL
const char* webhookURL = "WebHook URL";

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

void loop() {
  sendDiscordNotification("ESP32 Notification");
  delay(10000); // Send every 10 seconds
}

void sendDiscordNotification(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(webhookURL);
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"content\":\"" + message + "\"}";
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
      Serial.println("Error sending message");
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }
}
