#include <functions.h>
#include <Arduino.h>
#include <ESP32Servo.h>

Servo myServo;

int servoMin = 35;
int servoMax = 145;
int servoStep = 1;
int checkDelayMs = 15;
int servoMid = 90;
int servoCurrent = -1;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("===========================================================");
  Serial.println("   WRO Future Engineers 2025 - Mindcraft Engineers");
  Serial.println("        International Finals | Singapore");
  Serial.println("===========================================================");
  initServo();
  initBuzzer();
  motorInit();
}

void loop() {
  drive(50);
  delay(2000);
  holdRobot();
  delay(1000);
  drive(-50);
  delay(2000);
  stopMotor();
  delay(1000);
}