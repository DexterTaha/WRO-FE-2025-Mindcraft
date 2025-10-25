#include "../include/functions.h"

void motorInit() {
  pinMode(MOTOR_BI1_PIN, OUTPUT);
  pinMode(MOTOR_BI2_PIN, OUTPUT);
  pinMode(MOTOR_STBY_PIN, OUTPUT);

  digitalWrite(MOTOR_STBY_PIN, HIGH);

  ledcSetup(MOTOR_LEDC_CHANNEL, MOTOR_LEDC_FREQ, 8);
  ledcAttachPin(MOTOR_PWB_PIN, MOTOR_LEDC_CHANNEL);

  ledcWrite(MOTOR_LEDC_CHANNEL, 0);
}

void drive(int percent) {
  if (percent > 100) percent = 100;
  if (percent < -100) percent = -100;

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

void holdRobot() {
  digitalWrite(MOTOR_BI1_PIN, HIGH);
  digitalWrite(MOTOR_BI2_PIN, HIGH);
  ledcWrite(MOTOR_LEDC_CHANNEL, 255);
}

void stopMotor() {
  ledcWrite(MOTOR_LEDC_CHANNEL, 0);
  digitalWrite(MOTOR_STBY_PIN, LOW);
}
