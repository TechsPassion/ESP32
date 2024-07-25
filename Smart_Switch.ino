#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-Access-Point";
const char* password = "123456789";

// Initialize the Web server on port 80
WebServer server(80);

// Define the LED pin
const int ledPin = 2; //Using pin 2 for the On/Off
bool ledState = false;

void setup() {
  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // Start the access point
  WiFi.softAP(ssid, password);
  Serial.begin(115200);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Route to handle the root web page
  server.on("/", handleRoot);
  // Route to handle the LED on/off button
  server.on("/toggleLED", handleToggleLED);

  // Start the web server
  server.begin();
}

void loop() {
  // Listen for client requests
  server.handleClient();
}

void handleRoot() {
  String html = "<html><body><h1>Smart Switch</h1><p>Switch is ";
  html += (ledState) ? "ON" : "OFF";
  html += "</p><p><a href=\"/toggle\"><button>Toggle</button></a></p></body></html>";
  server.send(200, "text/html", html);
}

void handleToggleLED() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  handleRoot();
}

void stopAccessPoint() {
  // Stop the access point
  WiFi.softAPdisconnect(true);
  // Connect to the previously set up WiFi network (can be replaced with saved SSID and password)
  WiFi.begin("yourSSID", "yourPASSWORD");

  // Wait until the connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Restart the web server
  server.begin();
}
