#include <Arduino.h>
#include <functions.h>
#include <ESP32Servo.h>

// ----------------- Global variable definitions -----------------
Servo myServo;
int servoMin = 35;
int servoMax = 145;
int servoStep = 1;
int checkDelayMs = 15;
int servoMid = 90;
int servoCurrent = -1;
int motorSpeedPercent = 50;
String i2cBuffer = "";

// ---------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("===========================================================");
  Serial.println("   WRO Future Engineers 2025 - Mindcraft Engineers");
  Serial.println("        International Finals | Singapore");
  Serial.println("===========================================================");

  initServo();
  initBuzzer();
  initI2C();

  buzzerStart();
}

void loop() {
  // Just a status print every second
  static unsigned long lastReport = 0;
  unsigned long now = millis();
  if (now - lastReport > 1000) {
    lastReport = now;
    Serial.printf("Status: speed=%d%% | servo=%d°\n", motorSpeedPercent, servoCurrent);
  }
}
