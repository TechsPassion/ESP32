#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "WIFI Name";
const char* password = "Password";

// Create a web server object
WebServer server(80);

// Pin where the magnetic sensor is connected
const int sensorPin = 4; // Adjust based on your setup
const int ledPin = 2;    // Pin for the LED (GPIO 2 on most ESP32 boards)

// Variable to hold the door status
String doorStatus = "Unknown";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set the sensor pin as input and LED pin as output
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP Address: ");
  Serial.println(WiFi.localIP());

  // Define the web server route
  server.on("/", handleRoot);

  // Start the web server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Check the door status
  int sensorValue = digitalRead(sensorPin);
  if (sensorValue == LOW) {
    doorStatus = "Closed";
    digitalWrite(ledPin, LOW); // Turn LED off
  } else {
    doorStatus = "Open";
    digitalWrite(ledPin, HIGH); // Turn LED on
  }

  // Handle client requests
  server.handleClient();

  // Wait 5 second before the next check
  delay(5000);
}

void handleRoot() {
  // Create a webpage showing the door status
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta http-equiv='refresh' content='1'><title>Door Status</title></head>";
  html += "<body><h1>Door Status</h1>";
  html += "<p>The door is currently: <strong>" + doorStatus + "</strong></p>";
  html += "</body></html>";

  // Send the webpage to the client
  server.send(200, "text/html", html);
}
