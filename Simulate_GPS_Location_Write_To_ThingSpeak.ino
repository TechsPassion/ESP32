#include <WiFi.h>
#include "ThingSpeak.h"

// Replace with your Wi-Fi credentials
const char* ssid = "TechsPassion";       
const char* password = "Subscribe";

// Replace with your ThingSpeak Channel ID and Write API Key
unsigned long myChannelNumber = Your Channel ID Here; 
const char* myWriteAPIKey = "API Write Key"; 

// --- Random GPS Range (Las Vegas Area) ---
const float BASE_LAT = 36.1126;
const float BASE_LNG = -115.1767;

// Maximum distance to "wander" in degrees (approx 0.005 degrees = 550 meters)
const float WANDER_RANGE = 0.005; 

// Sending interval (ThingSpeak Free Tier minimum is 15 seconds)
const long postingInterval = 20000; // 20 seconds (20000 ms)
unsigned long lastUpdateTime = 0; 
//______________
WiFiClient client;

// --- Function to Generate Random Float within a Range ---
float generateRandomFloat(float base, float range) {
  // ESP32's random() function generates a large integer.
  // We divide it to get a small float and scale it to the desired range.
  float deviation = ((float)random(-10000, 10000) / 10000.0) * range;
  return base + deviation;
}

void setup() {
  Serial.begin(115200);

  // Initialize the random number generator using the chip's unique ID
  // This ensures the sequence of random numbers is different each time the ESP32 restarts.
  randomSeed(ESP.getEfuseMac()); 

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);
}

void loop() {
  // Check if the posting interval has passed
  if (millis() - lastUpdateTime > postingInterval) {
    
    // 1. Generate the Random Coordinates
    float randomLatitude = generateRandomFloat(BASE_LAT, WANDER_RANGE);
    float randomLongitude = generateRandomFloat(BASE_LNG, WANDER_RANGE);
    
    // 2. Set the fields (Field 1 = Latitude, Field 2 = Longitude)
    ThingSpeak.setField(1, randomLatitude);
    ThingSpeak.setField(2, randomLongitude);

    Serial.print("Sending random data...");
    Serial.print("Lat: "); Serial.print(randomLatitude, 6);
    Serial.print(", Lng: "); Serial.print(randomLongitude, 6);

    // 3. Write the fields to the ThingSpeak channel
    int httpResponseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if (httpResponseCode == 200) {
      Serial.println(" SUCCESS. Data sent.");
    } else {
      Serial.print(" FAILED. HTTP Error code: ");
      Serial.println(httpResponseCode);
    }
    
    lastUpdateTime = millis();
  }
}
