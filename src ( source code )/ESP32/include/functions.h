#ifndef functions_h
#define functions_h

#include <Arduino.h>
#include <ESP32Servo.h>


// -----------------------------------------------------------
// ---------------- Test ESP32 basic functions ---------------  
// -----------------------------------------------------------

inline void test_esp() {
  Serial.println("Starting test_esp: printing numbers 0..9");
  for (int i = 0; i < 10; ++i) {
    Serial.println(i);
    delay(200);
  }
  Serial.println("test_esp finished.");
}

// -----------------------------------------------------------
// ---------------- Servo motor control ----------------------
// -----------------------------------------------------------

#define SERVO_PIN 27

extern Servo myServo;
extern int servoMin;
extern int servoMax;
extern int servoStep;
extern int checkDelayMs;
extern int servoMid;  
extern int servoCurrent;


inline void initServo() {
  myServo.attach(SERVO_PIN);
  servoMid = (servoMin + servoMax) / 2;
  myServo.write(servoMid);
  Serial.printf("Servo initialized at %d° (min=%d max=%d).\n", servoMid, servoMin, servoMax);
}

inline void setServo(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  if (angle == servoCurrent) return;
  myServo.write(angle);
  servoCurrent = angle;
  // Serial.printf("Angle: %d°\n", angle);
}


inline void steer(int percent) {
  if (percent < -100) percent = -100;
  if (percent > 100) percent = 100;

  float t = (percent + 100) / 200.0f;
  int angle = servoMin + (int)round(t * (servoMax - servoMin));
  setServo(angle);
}

inline void checkServo() {
  Serial.printf("checkServo: Smooth sweep from %d to %d\n", servoMin, servoMax);

  int step = (servoStep > 0) ? servoStep : 1;
  int delayMs = (checkDelayMs > 0) ? checkDelayMs : 15;

  for (int angle = servoMin; angle <= servoMax; angle += step) {
    setServo(angle);
    delay(delayMs);
  }

  for (int angle = servoMax; angle >= servoMin; angle -= step) {
    setServo(angle);
    delay(delayMs);
  }

  Serial.println("checkServo: Smooth sweep complete.");
}


// -----------------------------------------------------------
// ---------------------- BUZZER control----------------------
// -----------------------------------------------------------

#define BUZZER_PIN 4

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
  delay(30);
}

inline void buzzerStart() {
  Serial.println("buzzerStart: playing start melody");
  playTone(440, 150);
  playTone(660, 150);
  playTone(880, 250);
}

inline void buzzerEnd() {
  Serial.println("buzzerEnd: playing end melody");
  playTone(880, 200);
  playTone(660, 200);
  playTone(440, 300);
}

inline void buzzerCheck() {
  Serial.println("buzzerCheck: playing check melody");
  playTone(1000, 160);
  delay(80);
  playTone(1000, 160);
  delay(80);
  playTone(1000, 160);
}

inline void buzzerError() {
  Serial.println("buzzerError: playing error melody");
  playTone(200, 400);
  delay(100);
  playTone(1000, 140);
  delay(80);
  playTone(1000, 140);
}

// -----------------------------------------------------------
// ---------------- Motor control (TB6612FNG) ----------------
// -----------------------------------------------------------

// Motor pin mapping (from README)
#define MOTOR_PWB_PIN 14
#define MOTOR_BI1_PIN 26
#define MOTOR_BI2_PIN 12
#define MOTOR_STBY_PIN 25

const int MOTOR_LEDC_CHANNEL = 1;
const int MOTOR_LEDC_FREQ = 2000;

// Motor implementations (inline in header)
inline void motorInit() {
  pinMode(MOTOR_BI1_PIN, OUTPUT);
  pinMode(MOTOR_BI2_PIN, OUTPUT);
  pinMode(MOTOR_STBY_PIN, OUTPUT);

  digitalWrite(MOTOR_STBY_PIN, HIGH);

  ledcSetup(MOTOR_LEDC_CHANNEL, MOTOR_LEDC_FREQ, 8);
  ledcAttachPin(MOTOR_PWB_PIN, MOTOR_LEDC_CHANNEL);

  ledcWrite(MOTOR_LEDC_CHANNEL, 0);
}

inline void drive(int percent) {
  if (percent > 100) percent = 100;
  if (percent < -100) percent = -100;
\-
  if (percent == 0) {
    digitalWrite(MOTOR_BI1_PIN, HIGH);
    digitalWrite(MOTOR_BI2_PIN, HIGH);
    ledcWrite(MOTOR_LEDC_CHANNEL, 255);
    return;
  }

  digitalWrite(MOTOR_STBY_PIN, HIGH);

  int duty = (int)((abs(percent) * 255) / 100);
  if (percent > 0) {
    digitalWrite(MOTOR_BI1_PIN, LOW);
    digitalWrite(MOTOR_BI2_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_BI1_PIN, HIGH);
    digitalWrite(MOTOR_BI2_PIN, LOW);
  }
  ledcWrite(MOTOR_LEDC_CHANNEL, duty);
}

inline void holdRobot() {
  digitalWrite(MOTOR_BI1_PIN, HIGH);
  digitalWrite(MOTOR_BI2_PIN, HIGH);
  ledcWrite(MOTOR_LEDC_CHANNEL, 255);
}

inline void stopMotor() {
  ledcWrite(MOTOR_LEDC_CHANNEL, 0);
  digitalWrite(MOTOR_STBY_PIN, LOW);
}


#endif