#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display
//VCC → 5V on ESP32
//GND → GND on ESP32
//SCL → GPIO 22 (SCL) on ESP32
//SDA → GPIO 21 (SDA) on ESP32

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// BLE Service and Characteristic UUIDs
// Generate your own unique UUIDs from a site like uuidgenerator.net
#define SERVICE_UUID        "5f086625-f902-4dc6-896b-8854ead7cbdf"
#define CHARACTERISTIC_UUID "0cd77ecd-7085-4ad8-bfa3-69703043e368"

// A class to handle incoming BLE messages
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.println("*********");
            Serial.print("New Value: ");
            for (int i = 0; i < value.length(); i++)
                Serial.print(value[i]);
            Serial.println();
            Serial.println("*********");

            // Display the received text on the OLED
            display.clearDisplay();
            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(0,0);
            display.println("New Notification:");
            display.println(value.c_str()); // Convert std::string to const char*
            display.display();
        }
    }
};

void setup() {
    Serial.begin(115200);

    // Initialize OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("BLE Notifier Ready!");
    display.println("Waiting for phone...");
    display.display();

    // Create the BLE Device
    BLEDevice::init("ESP32_Notifier"); // Set the name that will appear during scanning
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

    pCharacteristic->setCallbacks(new MyCallbacks());

    pService->start();
    
    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    // Nothing needed here, all the work is done in the callback
    delay(2000);
}
