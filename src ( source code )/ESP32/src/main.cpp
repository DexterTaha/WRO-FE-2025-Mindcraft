#include <Arduino.h>
#include <ESP32Servo.h>
#include <math.h>
#include <Wire.h>

#define I2C_ADDRESS 0x08
String i2cBuffer = "";

constexpr int PIN_PWB      = 14;
constexpr int PIN_BI2      = 12;
constexpr int PIN_BI1      = 26;
constexpr int PIN_STBY     = 25;
constexpr int PIN_SERVO    = 27;
constexpr int PIN_ENC_B    = 32;
constexpr int PIN_ENC_A    = 33;
constexpr int PIN_BUTTON   = 19;
constexpr int PIN_BUZZER   = 4;

const float WHEEL_DIAMETER_MM = 0.065;
const int   ENCODER_PULSES_PER_REV = 360;

float pulsesPerMm;
  
volatile long encoderCount = 0;
volatile int speedPercent = 50;
const int SPEED_STEP = 25;

unsigned long lastButtonMillis = 0;
const unsigned long DEBOUNCE_MS = 50;

const uint32_t MOTOR_LEDC_FREQ = 2000;
const uint8_t  MOTOR_LEDC_RES  = 8;
const uint32_t BUZZER_LEDC_FREQ = 2000;
const uint8_t  BUZZER_LEDC_RES  = 8;

const uint16_t TONE_START = 1000;
const uint16_t TONE_END   = 1500;
const uint16_t TONE_ERR   = 2000;
const unsigned long TONE_MS = 120;

Servo steeringServo;
const int SERVO_MIN = 35;
const int SERVO_MAX = 145;
const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;

volatile bool encUpdated = false;

void IRAM_ATTR encoderISR() {
  int a = digitalRead(PIN_ENC_A);
  int b = digitalRead(PIN_ENC_B);
  if (a == b) encoderCount--; else encoderCount++;
  encUpdated = true;
}

long readEncoderCount() {
  noInterrupts();
  long c = encoderCount;
  interrupts();
  return c;
}

// void setMotorDirectionForward() {
//   digitalWrite(PIN_BI1, LOW);
//   digitalWrite(PIN_BI2, HIGH);
// }

// void setMotorDirectionReverse() {
//   digitalWrite(PIN_BI1, HIGH);
//   digitalWrite(PIN_BI2, LOW);
// }

// void motorStandby(bool on) {
//   digitalWrite(PIN_STBY, on ? HIGH : LOW);
// }

// void setMotorSpeedPercent(int percent) {
//     if(percent == 0) { 
//         ledcWrite(0, 0);
//         return; 
//     }

//     // Determine direction
//     if(percent > 0) {
//         setMotorDirectionForward();
//     } else {
//         setMotorDirectionReverse();
//         percent = -percent; // make percent positive for PWM
//     }

//     if(percent > 100) percent = 100;
//     int maxDuty = (1 << MOTOR_LEDC_RES) - 1;
//     int duty = (percent * maxDuty) / 100;
//     ledcWrite(0, duty);
// }

// // --- Motor control helpers ---
// void motorForward(int percent) {
//     if (percent < 0) percent = 0;
//     if (percent > 100) percent = 100;
//     setMotorDirectionForward();
//     setMotorSpeedPercent(percent);
// }

// void motorBackward(int percent) {
//     if (percent < 0) percent = 0;
//     if (percent > 100) percent = 100;
//     setMotorDirectionReverse();
//     setMotorSpeedPercent(percent);
// }

// void motorStop() {
//     setMotorSpeedPercent(0);
//     motorStandby(false); // optional: fully stop driver
// }


void playTone(uint32_t freq, unsigned long ms) {
  ledcWriteTone(PIN_BUZZER, freq);
  Serial.print("Buzzer tone start freq=");
  Serial.print(freq);
  Serial.print("ms=");
  Serial.println(ms);
  delay(ms);
  ledcWriteTone(PIN_BUZZER, 0);
  Serial.println("Buzzer tone stop");
}

// void moveDistanceMm(float mm) {
//   if (mm == 0.0f) return;
//   bool forward = mm > 0.0f;
//   long startCount = readEncoderCount();
//   float targetPulsesF = fabs(mm) * pulsesPerMm;
//   long targetPulses = (long)(targetPulsesF + 0.5f);
//   long targetCount = forward ? (startCount + targetPulses) : (startCount - targetPulses);
//   Serial.print("Move start mm=");
//   Serial.print(mm);
//   Serial.print(" targetPulses=");
//   Serial.print(targetPulses);
//   Serial.print(" startCount=");
//   Serial.print(startCount);
//   Serial.print(" targetCount=");
//   Serial.println(targetCount);
//   playTone(TONE_START, TONE_MS);
//   motorStandby(true);
//   if (forward) setMotorDirectionForward(); else setMotorDirectionReverse();
//   setMotorSpeedPercent(speedPercent);
//   if (forward) {
//     while (readEncoderCount() < targetCount) {
//       if (encUpdated) {
//         encUpdated = false;
//         Serial.print("Encoder=");
//         Serial.println(readEncoderCount());
//       }
//       delay(2);
//     }
//   } else {
//     while (readEncoderCount() > targetCount) {
//       if (encUpdated) {
//         encUpdated = false;
//         Serial.print("Encoder=");
//         Serial.println(readEncoderCount());
//       }
//       delay(2);
//     }
//   }
//   setMotorSpeedPercent(0);
//   motorStandby(false);
//   playTone(TONE_END, TONE_MS);
//   Serial.println("Move complete");
// }

void handleButton() {
  bool pressed = digitalRead(PIN_BUTTON) == LOW;
  unsigned long now = millis();
  static bool lastState = HIGH;
  if (pressed && lastState == HIGH && (now - lastButtonMillis) > DEBOUNCE_MS) {
    lastButtonMillis = now;
    speedPercent += SPEED_STEP;
    if (speedPercent > 100) speedPercent = 0;
    Serial.print("Button pressed. speedPercent=");
    Serial.println(speedPercent);
    playTone(900, 80);
  }
  lastState = pressed;
}






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

inline void motorStop() {
  setMotorSpeedPercent(0);
  motorStandby(false); // optional: fully stop driver
}





void setupPins() {
  pinMode(PIN_BI1, OUTPUT);
  pinMode(PIN_BI2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  // Motor setup
  const int MOTOR_CHANNEL = 0;
  ledcSetup(MOTOR_CHANNEL, MOTOR_LEDC_FREQ, MOTOR_LEDC_RES);
  ledcAttachPin(PIN_PWB, MOTOR_CHANNEL);

  // Buzzer setup
  const int BUZZER_CHANNEL = 1;
  ledcSetup(BUZZER_CHANNEL, BUZZER_LEDC_FREQ, BUZZER_LEDC_RES);
  ledcAttachPin(PIN_BUZZER, BUZZER_CHANNEL);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);

  // initServo();

  // ✅ Servo setup (corrected)
  steeringServo.setPeriodHertz(50);
  steeringServo.attach(PIN_SERVO, 500, 2500);
  steeringServo.write(90);
}





void playStartupTone() {
    playTone(523, 150); // C5
    playTone(659, 150); // E5
    playTone(784, 300); // G5
}

// ---- Unique Power-Off Tone ----
void playPoweroffTone() {
    playTone(784, 150); // G5
    playTone(659, 150); // E5
    playTone(523, 300); // C5
}

// ---- Unique Error Tone ----
void playErrorTone() {
    playTone(220, 200); // A3 low tone
    playTone(196, 200); // G3
    playTone(220, 200); // A3
}

void onI2CReceive(int len) {
    while (Wire.available()) {
        char c = Wire.read();
        if (c == '\n' || c == '\r') {
            i2cBuffer.trim();

            if (i2cBuffer.startsWith("M_SPEED:")) {
                int val = i2cBuffer.substring(8).toInt(); // can be negative
                if (val < -100) val = -100;
                if (val > 100) val = 100;

                speedPercent = abs(val); // magnitude for PWM

                motorStandby(true);

                if (val > 0) {
                    setMotorDirectionForward();
                } else if (val < 0) {
                    setMotorDirectionReverse();
                } else {
                    // val == 0, stop motor
                    setMotorSpeedPercent(0);
                    motorStandby(false);
                    Serial.println("Motor stopped");
                    i2cBuffer = "";
                    return;
                }

                setMotorSpeedPercent(speedPercent);

                Serial.print("Motor direction: ");
                Serial.print(val > 0 ? "Forward" : "Reverse");
                Serial.print(", speedPercent=");
                Serial.println(speedPercent);
            }
            else if (i2cBuffer.startsWith("M_STOP")) {
                Serial.println("Stop command received");
                motorStop();
            } 
            else if (i2cBuffer.startsWith("SERVO_ANG:")) {
                int angle = i2cBuffer.substring(10).toInt();

                // Convert angle 0–180 to percent -100 to 100
                int percent = map(angle, 0, 180, -100, 100);

                // Use your new steer() function
                steer(percent);

                Serial.printf("Steering set via I2C: angle=%d, percent=%d\n", angle, percent);
            }

            i2cBuffer = ""; // reset buffer
        } else {
            i2cBuffer += c;
        }
    }
}



void onI2CRequest() {
    // Master (Raspberry Pi) requests data
    String data = String(encoderCount);
    Wire.write(data.c_str()); // send encoder count as string

}


void setup() {
  Serial.begin(115200);
  pulsesPerMm = (float)ENCODER_PULSES_PER_REV / (PI * WHEEL_DIAMETER_MM);
  initMotor();
  initServo();
  encoderCount = 0;
  speedPercent = 50;
  motorStandby(true);


  Wire.begin(I2C_ADDRESS);       // ESP32 as I2C slave
  Wire.onReceive(onI2CReceive);  // register receive handler
  Wire.onRequest(onI2CRequest);  // register request handler


  playStartupTone();
  delay(1000);
  playErrorTone();
  delay(1000);
  playPoweroffTone();

  Serial.println("Setup complete");
  Serial.print("pulsesPerMm=");
  Serial.println(pulsesPerMm, 6);
  Serial.print("Initial speedPercent=");
  Serial.println(speedPercent);
  Serial.print("Servo mid=");
  Serial.println(SERVO_MID);
  Serial.println("Ready. Use moveDistanceMm(mm) to move and steeringServo.write(angle) to steer.");

}


void loop() {
  handleButton();


  static unsigned long lastReport = 0;
  unsigned long now = millis();
  if (now - lastReport > 1000) {
    lastReport = now;
    Serial.print("Status enc=");
    Serial.print(readEncoderCount());
    Serial.print(" speed=");
    Serial.println(speedPercent);
  }
  delay(10);

}

