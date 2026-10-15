# ESP32 Over The Air (OTA) Upload

Upload code to your ESP32 over WiFi, no USB cable needed.

## Steps

1. Open `OTA_Basic/OTA_Basic.ino` and enter your WiFi name and password.
2. Select your board in **Tools > Board**.
3. In **Tools > Partition Scheme**, pick one with OTA (not "No OTA" or "Huge APP"). For example:
   - ESP32 (4MB): **Default 4MB with spiffs**
   - ESP32-S3 (16MB): **16M Flash (2MB APP/12.5MB FATFS)**
4. Upload once over USB.
5. Open the Serial Monitor (115200) and note the IP address.
6. Change `blinkMs` (for example `1000` to `200`).
7. In **Tools > Port**, pick **techspassion-esp32** under Network ports.
8. Click **Upload** and enter the password `techspassion`.

The LED now blinks faster: the new code arrived over WiFi.

## Notes

- Your computer and the ESP32 must be on the same WiFi.
- Every sketch you upload must include the OTA code, or the next upload needs USB again.
- ESP32-S3 with an empty Serial Monitor? Set **Tools > USB CDC On Boot > Enabled**.
- Network port not showing? Restart the Arduino IDE, and on Windows allow it through the firewall.

You can use this code however you like. Please consider liking and subscribing to my YouTube channel: https://www.youtube.com/@techs.passion
