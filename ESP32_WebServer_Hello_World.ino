#include <WiFi.h>       // Include the WiFi library for network connectivity
#include <WebServer.h>  // Include the WebServer library for creating a web server

// WiFi credentials (replace with your actual credentials)
const char* ssid = "Your Wifi Name";     // Your WiFi network's SSID (name)
const char* password = "Your Wifi Password"; // Your WiFi network's password

// Create a WebServer object that listens on port 80 (HTTP port)
WebServer server(80);

// Function to handle requests to the root URL ("/")
void handleRoot() {
  // Send an HTTP response with status code 200 (OK), content type "text/html", and the HTML content
  server.send(200, "text/html", "<h1>Hello World!</h1>");
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);

  // Connect to the WiFi network
  WiFi.begin(ssid, password);

  // Wait until the ESP32 is connected to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); // Wait for 1 second
    Serial.println("Connecting to WiFi...");
  }

  // Print a message to the serial monitor when connected
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Print the ESP32's IP address

  // Register the handleRoot function to be called when the root URL is requested
  server.on("/", handleRoot);

  // Start the web server
  server.begin();

  // Print a message to the serial monitor when the server starts
  Serial.println("HTTP server started");
}

void loop() {
  // Handle incoming client requests
  server.handleClient();
}
