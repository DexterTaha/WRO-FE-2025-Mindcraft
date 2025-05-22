#include <Servo.h>  // Include the Servo library

Servo STEERING;;  // Create Servo object for the STEERING



void setup() {
  // Initialize the serial communication
  Serial.begin(9600);

  // Attach the servos to pins 9 and 10
  STEERING.attach(10);

  // Set the servo position to 90 degree
  STEERING.write(50);
  delay(500);
  STEERING.write(130);

}

void loop() {
}
