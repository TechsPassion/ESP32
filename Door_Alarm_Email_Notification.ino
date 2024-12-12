#include <WiFi.h>
#include <ESP_Mail_Client.h>

// Wi-Fi credentials
#define WIFI_SSID "WIFI"
#define WIFI_PASSWORD "Password"

// Email credentials and server settings
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "YourEmail@gmail.com"
#define AUTHOR_PASSWORD "Your App Password"
#define RECIPIENT_EMAIL "RECIPIENT_EMAIL@RECIPIENT_EMAIL.COM"

// Magnet sensor pin
const int sensorPin = 4; // Adjust based on your setup
const int ledPin = 2;    // Pin for the LED (GPIO 2 on most ESP32 boards)

bool doorPreviouslyOpen = false; // Track the previous door state

void sendEmail() {
  SMTPSession smtp;
  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;

  SMTP_Message message;
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Door Open Alert";
  message.text.content = "The door has been opened!";
  message.addRecipient("Email", RECIPIENT_EMAIL);

  if (!smtp.connect(&config) || !MailClient.sendMail(&smtp, &message)) {
    Serial.println("Failed to send email");
  } else {
    Serial.println("Email sent successfully!");
  }

  smtp.closeSession();
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set the sensor pin as input and LED pin as output
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check the current door status
  int sensorValue = digitalRead(sensorPin);
  bool doorOpen = (sensorValue == HIGH);

  // Turn the LED on if the door is open
  digitalWrite(ledPin, doorOpen ? HIGH : LOW);

  // Send an email only when the door transitions from closed to open
  if (doorOpen && !doorPreviouslyOpen) {
    Serial.println("Door opened! Sending email...");
    sendEmail();
  }

  // Update the previous door state
  doorPreviouslyOpen = doorOpen;

  delay(50); // Small delay to avoid excessive processing
}
