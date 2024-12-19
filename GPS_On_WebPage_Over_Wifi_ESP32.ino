#include <WiFi.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// GPS setup
TinyGPSPlus gps;
HardwareSerial SerialGPS(1); // Use hardware serial for GPS

// WiFi credentials for Access Point
const char* ssid = "TechsPassion";
const char* password = "subscribe";

// GPS Data variables
double latitude = 0.0, longitude = 0.0;
int satellites = 0;
int hdop = 0;

// Create a WiFi server on port 80
WiFiServer server(80);

void setup() {
  Serial.begin(115200); // Debugging
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17); // GPS RX2 (GPIO 16), TX2 (GPIO 17)

  // Start WiFi as Access Point
  WiFi.softAP(ssid, password);
  Serial.print("Access Point Started. Connect to: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start the server
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  // Update GPS data
  while (SerialGPS.available() > 0) {
    char c = SerialGPS.read();
    if (gps.encode(c)) {
      if (gps.location.isUpdated()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
        satellites = gps.satellites.value();
        hdop = gps.hdop.value();
      }
    }
  }

  // Handle client requests
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    // Determine if this is a webpage or data request
    if (request.indexOf("/gpsdata") >= 0) {
      // Send GPS data as JSON
      String gpsData = "{";
      gpsData += "\"latitude\": " + String(latitude, 6) + ",";
      gpsData += "\"longitude\": " + String(longitude, 6) + ",";
      gpsData += "\"satellites\": " + String(satellites) + ",";
      gpsData += "\"hdop\": " + String(hdop);
      gpsData += "}";
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.println(gpsData);
    } else {
      // Send the HTML webpage
      String webpage = "<!DOCTYPE html><html>";
      webpage += "<head><title>TechsPassion GPS Data</title>";
      webpage += "<style>body { font-family: Arial; text-align: center; }</style>";
      webpage += "<script>";
      webpage += "function updateGPS() {";
      webpage += "  fetch('/gpsdata').then(response => response.json()).then(data => {";
      webpage += "    document.getElementById('latitude').innerText = data.latitude;";
      webpage += "    document.getElementById('longitude').innerText = data.longitude;";
      webpage += "    document.getElementById('satellites').innerText = data.satellites;";
      webpage += "    document.getElementById('hdop').innerText = data.hdop;";
      webpage += "  });";
      webpage += "}";
      webpage += "setInterval(updateGPS, 2000);"; // Update every 2 seconds
      webpage += "</script>";
      webpage += "</head>";
      webpage += "<body onload='updateGPS()'>";
      webpage += "<h1>ESP32 GPS Data</h1>";
      webpage += "<p><b>Latitude:</b> <span id='latitude'>Loading...</span></p>";
      webpage += "<p><b>Longitude:</b> <span id='longitude'>Loading...</span></p>";
      webpage += "<p><b>Satellites:</b> <span id='satellites'>Loading...</span></p>";
      webpage += "<p><b>HDOP:</b> <span id='hdop'>Loading...</span></p>";
      webpage += "<p><i>GPS data updates automatically every 2 seconds.</i></p>";
      webpage += "</body></html>";
      
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println(webpage);
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
}
