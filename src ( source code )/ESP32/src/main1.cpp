#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

#define I2C_ADDRESS 0x08

// ==================== GLOBALS ====================
String i2cBuffer = "";
String pendingCommand = "";
bool newCommandAvailable = false;

volatile long encoderCount = 0;
volatile int speedPercent = 0;

volatile bool buttonPressedEvent = false;

// ==== Pins ====
constexpr int PIN_SERVO  = 27;
constexpr int PIN_PWB    = 14;  // motor PWM
constexpr int PIN_BI1    = 26;  // motor direction 1
constexpr int PIN_BI2    = 12;  // motor direction 2
constexpr int PIN_STBY   = 25;  // motor standby
constexpr int PIN_BUTTON   = 19;
constexpr int PIN_BUZZER    = 4;  // motor direction 1

unsigned long lastButtonMillis = 0;
const unsigned long DEBOUNCE_MS = 50;

// ==== PWM configuration ====
constexpr int MOTOR_LEDC_CHANNEL = 0;   // PWM channel 0..15
constexpr int MOTOR_LEDC_FREQ    = 500; // 500 Hz
constexpr int MOTOR_LEDC_RES     = 8;   // 8-bit resolution (0-255)

Servo steeringServo;
const int SERVO_MIN = 35;
const int SERVO_MAX = 145;
const int SERVO_MID = (SERVO_MIN + SERVO_MAX) / 2;

const uint16_t TONE_START = 1000;
const uint16_t TONE_END   = 1500;
const uint16_t TONE_ERR   = 2000;
const unsigned long TONE_MS = 120;

// ==================== MOTOR CONTROL ====================
inline void motorStandby(bool on) { digitalWrite(PIN_STBY, on ? HIGH : LOW); }
inline void setMotorDirectionForward() { digitalWrite(PIN_BI1,HIGH); digitalWrite(PIN_BI2,LOW); }
inline void setMotorDirectionReverse() { digitalWrite(PIN_BI1,LOW); digitalWrite(PIN_BI2,HIGH); }
inline void fullstop() { digitalWrite(PIN_BI1,HIGH); digitalWrite(PIN_BI2,HIGH); }

inline void initMotor() {
  pinMode(PIN_PWB, OUTPUT);
  pinMode(PIN_BI1, OUTPUT);
  pinMode(PIN_BI2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // Motor PWM setup
  ledcSetup(MOTOR_LEDC_CHANNEL, MOTOR_LEDC_FREQ, MOTOR_LEDC_RES);
  ledcAttachPin(PIN_PWB, MOTOR_LEDC_CHANNEL);

  digitalWrite(PIN_STBY, HIGH);
  Serial.println("Motor initialized");
}

inline void setMotorSpeedPercent(int percent) {
  percent = constrain(percent, 0, 100);
  motorStandby(true);
  int maxDuty = (1 << MOTOR_LEDC_RES) - 1;
  int duty = (percent * maxDuty) / 100;
  ledcWrite(MOTOR_LEDC_CHANNEL, duty);
  speedPercent = percent;
  Serial.printf("Motor speed set to %d%% (PWM duty=%d)\n", percent, duty);
}

inline void motorStop() {
  setMotorSpeedPercent(0);
  motorStandby(false);
}

// ==================== SERVO ====================
inline void initServo() {
  // Allocate a free timer for servo PWM
  ESP32PWM::allocateTimer(1);  // Timer 1 also free if you add more servos
  steeringServo.setPeriodHertz(50);           // 50 Hz servo
  steeringServo.attach(PIN_SERVO, 500, 2500); // min/max pulse width in us
  steeringServo.write(SERVO_MID);
}

inline void steer(int percent) {
  percent = constrain(percent, -100, 100);
  float t = (percent + 100) / 200.0f;
  int angle = SERVO_MIN + round(t * (SERVO_MAX - SERVO_MIN));
  steeringServo.write(angle);
}

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


// ==================== I2C HANDLERS ====================
void onI2CReceive(int len) {
  while(Wire.available()) {
    char c = Wire.read();
    if(c == '\n' || c == '\r') {
      i2cBuffer.trim();
      if(i2cBuffer.length() > 0) {
        pendingCommand = i2cBuffer;
        newCommandAvailable = true;
      }
      i2cBuffer = "";
    } else {
      i2cBuffer += c;
    }
  }
}

void onI2CRequest() {
    static char buffer[32];

    // --- Send PRESSED once ---
    if (buttonPressedEvent) {
        buttonPressedEvent = false;     // consume event
        strcpy(buffer, "PRESSED");
        Wire.write((const uint8_t*)buffer, strlen(buffer));
        return;
    }

    // --- Normal status reply ---
    int n = snprintf(buffer, sizeof(buffer),
                     "SPD:%d ENC:%ld", speedPercent, encoderCount);

    Wire.write((const uint8_t*)buffer, n);
}



void handleButton() {
  static bool lastState = HIGH;
  bool pressed = (digitalRead(PIN_BUTTON) == LOW);
  unsigned long now = millis();

  if (pressed && lastState == HIGH && (now - lastButtonMillis) > DEBOUNCE_MS) {
    lastButtonMillis = now;

    // Only set the event when a fresh physical press happens
    buttonPressedEvent = true;
    Serial.println("BUTTON PRESSED - event stored");
  }

  lastState = pressed;
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


// ==================== SETUP ====================
void setup() {

  Serial.begin(115200);

  initMotor();
  initServo();
  motorStandby(true);

  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  buttonPressedEvent = false;

  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(onI2CReceive);
  Wire.onRequest(onI2CRequest);

  playStartupTone();

  Serial.println("Setup complete. Ready to receive I2C commands.");


}

void testMotorSpeed(int dly) {
  Serial.println("Starting motor speed test...");
  setMotorDirectionForward();

  for (int speed = 0; speed <= 100; speed += 10) {
    setMotorSpeedPercent(speed);
    Serial.printf("Motor speed: %d%%\n", speed);
    delay(dly);
  }

  for (int speed = 100; speed >= 0; speed -= 10) {
    setMotorSpeedPercent(speed);
    Serial.printf("Motor speed: %d%%\n", speed);
    delay(dly);
  }

  motorStop();
  Serial.println("Motor test complete.\n");
}

void testSteering(int dly) {
  Serial.println("Starting servo steering test...");

  for (int percent = -100; percent <= 100; percent += 10) {
    steer(percent);
    Serial.printf("Steering percent: %d\n", percent);
    delay(dly);
  }

  for (int percent = 100; percent >= -100; percent -= 10) {
    steer(percent);
    Serial.printf("Steering percent: %d\n", percent);
    delay(dly);
  }

  steer(0);
  Serial.println("Servo test complete.\n");
}

void T_M_S(int dly, int mdly, int sdly){
  testMotorSpeed(mdly);   // run motor test
  delay(dly);        // short pause
  testSteering(sdly);     // run servo test
  delay(dly); 
}

// void loop() {
//   T_M_S(50, 500, 60);
// }
void loop() {
    handleButton();
    // ---- Handle new I2C commands ----
    if(newCommandAvailable) {
        newCommandAvailable = false;
        String cmd = pendingCommand;
        pendingCommand = "";

        if(cmd.startsWith("M_SPEED:")) {
            int val = cmd.substring(8).toInt();  // can be negative
            bool forward = val >= 0;
            val = abs(val);

            motorStandby(true);                  // ensure motor driver enabled
            if(forward) setMotorDirectionForward();
            else setMotorDirectionReverse();

            // Update PWM using your function (handles 0-100 scaling)
            setMotorSpeedPercent(val);

            Serial.printf("Motor command: val=%d dir=%s speedPercent=%d\n",
                          val, forward ? "F" : "R", speedPercent);
        }
        else if(cmd.startsWith("M_STOP")) {
            fullstop();
            Serial.println("Motor stopped");
        }
        else if(cmd.startsWith("SERVO_ANG:")) {
            int angle = cmd.substring(10).toInt();
            int percent = map(angle, 0, 180, -100, 100);
            steer(percent);
            Serial.printf("Servo angle=%d percent=%d\n", angle, percent);
        }
    }

    // ---- Periodic status report ----
    static unsigned long lastReport = 0;
    unsigned long now = millis();
    if(now - lastReport > 1000) {
        lastReport = now;
        Serial.printf("Status enc=%ld speed=%d\n", encoderCount, speedPercent);
    }

    delay(10);
}
