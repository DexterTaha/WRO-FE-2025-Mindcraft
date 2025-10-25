#ifndef functions_h
#define functions_h

#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ============================================================
// ================ Test Basic ESP32 function =================
// ============================================================

inline void test_esp() {
  Serial.println("Starting test_esp: printing numbers 0..9");
  for (int i = 0; i < 10; ++i) {
    Serial.println(i);
    delay(200);
  }
  Serial.println("test_esp finished.");
}

// ============================================================
// ===================== I2C Communication ====================
// ============================================================

#define I2C_ADDRESS 0x08
String i2cBuffer = "";
int motorSpeedPercent = 50;  // updated by I2C

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
        if (angle >= 0 && angle <= 180) {
          Serial.printf("[I2C] Servo angle command: %d°\n", angle);
        }
      }

      i2cBuffer = "";
    } else {
      i2cBuffer += c;
    }
  }
}

inline void initI2C() {
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(onI2CReceive);
  Serial.printf("I2C initialized at address 0x%02X\n", I2C_ADDRESS);
}

// ============================================================
// ======================= Servo Control ======================
// ============================================================

#define SERVO_PIN 27
Servo myServo;
int servoMin = 35;
int servoMax = 145;
int servoCurrent = -1;
int servoMid = (servoMin + servoMax) / 2;

inline void initServo() {
  myServo.attach(SERVO_PIN);
  servoCurrent = servoMid;
  myServo.write(servoMid);
  Serial.printf("Servo initialized at %d° (min=%d, max=%d)\n", servoMid, servoMin, servoMax);
}

inline void setServo(int angle) {
  if (angle < servoMin) angle = servoMin;
  if (angle > servoMax) angle = servoMax;
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

// ============================================================
// ======================== Motor Control =====================
// ============================================================

#define PIN_PWB 14
#define PIN_BI2 12
#define PIN_BI1 26
#define PIN_STBY 25
#define MOTOR_LEDC_CHANNEL 1
#define MOTOR_LEDC_FREQ 2000
#define MOTOR_LEDC_RES 8

inline void initMotor() {
  pinMode(PIN_PWB, OUTPUT);
  pinMode(PIN_BI1, OUTPUT);
  pinMode(PIN_BI2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);
  ledcSetup(MOTOR_LEDC_CHANNEL, MOTOR_LEDC_FREQ, MOTOR_LEDC_RES);
  ledcAttachPin(PIN_PWB, MOTOR_LEDC_CHANNEL);
  digitalWrite(PIN_STBY, LOW);
  Serial.println("Motor initialized");
}

inline void motorStandby(bool on) {
  digitalWrite(PIN_STBY, on ? HIGH : LOW);
}

inline void setMotorDirectionForward() {
  digitalWrite(PIN_BI1, HIGH);
  digitalWrite(PIN_BI2, LOW);
}

inline void setMotorDirectionReverse() {
  digitalWrite(PIN_BI1, LOW);
  digitalWrite(PIN_BI2, HIGH);
}

inline void setMotorSpeedPercent(int percent) {
  if (percent <= 0) {
    ledcWrite(MOTOR_LEDC_CHANNEL, 0);
    return;
  }
  if (percent > 100) percent = 100;
  const int maxDuty = (1 << MOTOR_LEDC_RES) - 1;  // For 8-bit, maxDuty = 255
  int duty = (percent * maxDuty) / 100;
  ledcWrite(MOTOR_LEDC_CHANNEL, duty);
  Serial.printf("Motor speed set to %d%%\n", percent);
}


// ============================================================
// ======================= Encoder Reading ====================
// ============================================================

#define PIN_ENC_A 33
#define PIN_ENC_B 32
const float WHEEL_DIAMETER_MM = 0.056;
const int ENCODER_PULSES_PER_REV = 360;

volatile long encoderCount = 0;
volatile bool encUpdated = false;
float pulsesPerMm;

// Declare ISR
void IRAM_ATTR encoderISR();

inline void initEncoder() {
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
  pulsesPerMm = (float)ENCODER_PULSES_PER_REV / (PI * WHEEL_DIAMETER_MM);
  Serial.printf("Encoder initialized: pulsesPerMm=%.4f\n", pulsesPerMm);
}

inline long readEncoderCount() {
  noInterrupts();
  long c = encoderCount;
  interrupts();
  return c;
}

// ============================================================
// ===================== Speed PID Control ====================
// ============================================================

float speedTargetPercent = 0; // -100..100
float speedKp = 5.0;          // increase for more aggressive response
float speedKi = 0.5;
float speedKd = 0.05;

float speedIntegral = 0;
float speedPrevError = 0;
unsigned long speedPrevTime = 0;
float currentSpeedPercent = 0;   // measured speed in % of max
float currentSpeedPulsesPerSec = 0;
long prevEnc = 0;

const float MOTOR_MAX_PPS = 3720.0; // max pulses/sec


inline void initMotorPID() {
    speedIntegral = 0;
    speedPrevError = 0;
    speedPrevTime = micros();
    Serial.println("Motor PID initialized");
}

inline void updateMotorPID() {
    static unsigned long lastTime = 0;
    unsigned long now = millis();
    if (now - lastTime < 25) return; // 40 Hz PID
    lastTime = now;

    long enc = readEncoderCount();
    long deltaEnc = enc - prevEnc;
    prevEnc = enc;

    float deltaT = 0.025; // PID period 25 ms
    float speedPPS = deltaEnc / deltaT;

    // convert target % to pulses/sec
    float targetPPS = (abs(speedTargetPercent)/100.0) * MOTOR_MAX_PPS;

    float error = targetPPS - speedPPS;
    speedIntegral += error * deltaT;
    float deriv = (error - speedPrevError) / deltaT;

    float pwmOutput = speedKp * error + speedKi * speedIntegral + speedKd * deriv;

    if (pwmOutput > 255) pwmOutput = 255;
    if (pwmOutput < 0) pwmOutput = 0;

    if (speedTargetPercent >= 0) setMotorDirectionForward();
    else setMotorDirectionReverse();

    ledcWrite(MOTOR_LEDC_CHANNEL, (int)pwmOutput);

    speedPrevError = error;

    // debug
    float mmPerSec = speedPPS / pulsesPerMm;
    Serial.printf("Target:%d%% | PWM:%.1f | PPS:%.1f | mm/s:%.1f\n",
                  (int)speedTargetPercent, pwmOutput, speedPPS, mmPerSec);
}


// ============================================================
// ======================== Button input ======================
// ============================================================

#define PIN_BUTTON 19
const unsigned long DEBOUNCE_MS = 50;
unsigned long lastButtonMillis = 0;
int speedPercent = 50;

inline void initButton() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  Serial.println("Button initialized");
}

inline void handleButton() {
  bool pressed = digitalRead(PIN_BUTTON) == LOW;
  unsigned long now = millis();
  static bool lastState = HIGH;

  if (pressed && lastState == HIGH && (now - lastButtonMillis) > DEBOUNCE_MS) {
    lastButtonMillis = now;
    speedPercent += 25;
    if (speedPercent > 100) speedPercent = 0;
    Serial.printf("Button pressed → speedPercent=%d\n", speedPercent);
  }

  lastState = pressed;
}

// ============================================================
// ======================== Buzzer control ====================
// ============================================================

#define BUZZER_PIN 4
const int BUZZER_LEDC_CHANNEL = 0;
const int BUZZER_LEDC_FREQ = 2000;

inline void initBuzzer() {
  ledcSetup(BUZZER_LEDC_CHANNEL, BUZZER_LEDC_FREQ, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
  Serial.println("Buzzer initialized");
}

inline void playTone(int freq, int ms) {
  if (freq <= 0) return;
  ledcWriteTone(BUZZER_LEDC_CHANNEL, freq);
  delay(ms);
  ledcWriteTone(BUZZER_LEDC_CHANNEL, 0);
  delay(20);
}

inline void buzzerStart() {
  playTone(523, 150); // C5
  playTone(659, 150); // E5
  playTone(784, 300); // G5
}

inline void buzzerEnd() {
  playTone(784, 150); // G5
  playTone(659, 150); // E5
  playTone(523, 300); // C5
}

inline void buzzerError() {
  playTone(220, 200);
  playTone(196, 200);
  playTone(220, 200);
}

#endif
