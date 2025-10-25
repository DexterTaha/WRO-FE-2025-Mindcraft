#include "functions.h"

void IRAM_ATTR encoderISR() {
  int a = digitalRead(PIN_ENC_A);
  int b = digitalRead(PIN_ENC_B);
  if (a == b) encoderCount--; else encoderCount++;
  encUpdated = true;
}

void setup() {
  Serial.begin(115200);

  // ----------------- Initialize modules -----------------
  initMotor();
  motorStandby(true);           // enable motor driver
  setMotorDirectionForward();   // default forward
  setMotorSpeedPercent(0);      // start stopped

  initServo();
  steer(0);                     // start straight

  initEncoder();
  initI2C();                     // start listening for I2C commands
}

void loop() {
  // ----------------- Motor speed -----------------
  // speedTargetPercent is updated by I2C (0..100 forward, negative backward)
  if (speedTargetPercent >= 0) setMotorDirectionForward();
  else setMotorDirectionReverse();

  setMotorSpeedPercent(abs((int)speedTargetPercent));

  // ----------------- Steering -----------------
  // i2cBuffer can send "SERVO_ANG:90" for example
  // The actual angle is already applied inside onI2CReceive() if you implemented it
  // So just make sure servoCurrent is updated there
  setServo(servoCurrent);

  delay(50); // small delay to reduce I2C spam
}
