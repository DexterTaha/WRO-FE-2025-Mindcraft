#ifndef functions_h
#define functions_h

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

// -----------------------------------------------------------
// ------------------- GLOBAL DEFINITIONS --------------------
// -----------------------------------------------------------

#define I2C_ADDRESS 0x08
#define SERVO_PIN   27
#define BUZZER_PIN  4

// -----------------------------------------------------------
// -------------------- GLOBAL VARIABLES ---------------------
// -----------------------------------------------------------

extern Servo myServo;
extern int servoMin;
extern int servoMax;
extern int servoStep;
extern int checkDelayMs;
extern int servoMid;
extern int servoCurrent;
extern int motorSpeedPercent;

extern String i2cBuffer;

// -----------------------------------------------------------
// ---------------- Servo motor control ----------------------
// -----------------------------------------------------------

inline void initServo() {
  myServo.attach(SERVO_PIN);
  servoMid = (servoMin + servoMax) / 2;
  myServo.write(servoMid);
  servoCurrent = servoMid;
  Serial.printf("Servo initialized at %d° (min=%d max=%d)\n", servoMid, servoMin, servoMax);
}

inline void setServo(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  if (angle == servoCurrent) return;
  myServo.write(angle);
  servoCurrent = angle;
  Serial.printf("Servo angle set to %d°\n", angle);
}

inline void steer(int percent) {
  if (percent < -100) percent = -100;
  if (percent > 100) percent = 100;
  float t = (percent + 100) / 200.0f;
  int angle = servoMin + (int)round(t * (servoMax - servoMin));
  setServo(angle);
}

// -----------------------------------------------------------
// ---------------------- BUZZER control----------------------
// -----------------------------------------------------------

const int BUZZER_LEDC_CHANNEL = 0;
const int BUZZER_LEDC_FREQ = 2000;

inline void initBuzzer() {
  ledcSetup(BUZZER_LEDC_CHANNEL, BUZZER_LEDC_FREQ, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
}

inline void playTone(int freq, int ms) {
  if (freq <= 0) return;
  ledcWrite(BUZZER_LEDC_CHANNEL, 255);
  ledcWriteTone(BUZZER_LEDC_CHANNEL, freq);
  delay(ms);
  ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
  ledcWrite(BUZZER_LEDC_CHANNEL, 0);
  delay(20);
}

inline void buzzerStart() {
  playTone(523, 150);
  playTone(659, 150);
  playTone(784, 300);
}

inline void buzzerEnd() {
  playTone(784, 150);
  playTone(659, 150);
  playTone(523, 300);
}

inline void buzzerError() {
  playTone(220, 200);
  playTone(196, 200);
  playTone(220, 200);
}

// -----------------------------------------------------------
// ---------------------- I2C Handling -----------------------
// -----------------------------------------------------------

inline void onI2CReceive(int len) {
  while (Wire.available()) {
    char c = Wire.read();
    if (c == '\n' || c == '\r') {
      i2cBuffer.trim();

      if (i2cBuffer.startsWith("M_SPEED:")) {
        int val = i2cBuffer.substring(8).toInt();
        if (val >= 0 && val <= 100) {
          motorSpeedPercent = val;
          Serial.printf("[I2C] Motor speed set to %d%%\n", motorSpeedPercent);
        }
      } 
      else if (i2cBuffer.startsWith("SERVO_ANG:")) {
        int angle = i2cBuffer.substring(10).toInt();
        if (angle >= servoMin && angle <= servoMax) {
          setServo(angle);
        }
      }

      i2cBuffer = "";
    } else {
      i2cBuffer += c;
    }
  }
}

inline void initI2C() {
  Wire.begin(I2C_ADDRESS); // ESP32 as slave
  Wire.onReceive(onI2CReceive);
  Serial.printf("I2C slave initialized at address 0x%02X\n", I2C_ADDRESS);
}

#endif
