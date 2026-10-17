/*
 * TechsPassion - Servo straight from an ESP32, no driver board
 *
 * Upload over USB. Sweeps the servo 0 -> 180 -> 0, or type an angle
 * (0-180) in the Serial Monitor at 115200 to move it there. Type "s" to
 * pause or resume the sweep.
 *
 * Wiring (the servo gets its power from its own 5-6 V supply, not the board):
 *   servo brown/black -> supply (-) AND an ESP32 GND pin
 *   servo red         -> supply (+)
 *   servo orange      -> GPIO 14
 *
 * Uses the ESP32's own PWM hardware (LEDC), so no library is needed and it
 * also works next to the camera driver on the ESP32-S3-CAM.
 */

const int servoPin = 14;

// Pulse width at the two end stops. 500-2400 us suits SG90/MG90S; use
// 1000-2000 if your servo buzzes at the ends.
const int servoMinUs = 500;
const int servoMaxUs = 2400;

const int servoFreq = 50;   // Hz, one pulse every 20 ms
const int servoRes  = 14;   // bits of resolution for the duty cycle

int  angle    = 90;
int  target   = 180;
bool sweeping = true;
const int stepMs = 15;      // ms per degree: 15 ms = 2.7 s for a full sweep
unsigned long lastStep = 0;

void servoWrite(int deg) {
  deg = constrain(deg, 0, 180);
  int us = map(deg, 0, 180, servoMinUs, servoMaxUs);
  uint32_t duty = (uint32_t)us * ((1UL << servoRes) - 1) / (1000000UL / servoFreq);
  ledcWrite(servoPin, duty);
}

void setup() {
  Serial.begin(115200);
  ledcAttach(servoPin, servoFreq, servoRes);
  servoWrite(angle);
  Serial.println("Servo on GPIO 14. Type 0-180 for an angle, s to pause/resume the sweep.");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "s") {
      sweeping = !sweeping;
      Serial.println(sweeping ? "sweep on" : "sweep off");
    } else if (cmd.length()) {
      sweeping = false;
      angle = constrain(cmd.toInt(), 0, 180);
      servoWrite(angle);
      Serial.printf("angle %d\n", angle);
    }
  }

  if (sweeping && millis() - lastStep >= stepMs) {
    lastStep = millis();
    angle += (target > angle) ? 1 : -1;
    servoWrite(angle);
    if (angle == target) target = (target == 180) ? 0 : 180;
  }
}
