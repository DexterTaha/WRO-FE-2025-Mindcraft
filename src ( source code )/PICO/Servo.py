from machine import Pin, PWM

# Initialize PWM on GPIO 0
servo = PWM(Pin(0))
servo.freq(50)  # 50Hz for standard servo motors

servoCentre = 85
TurnRange = 60
servoRMx = servoCentre+(TurnRange/2)
servoLMn = servoCentre-(TurnRange/2)

def set_servo_angle(angle):
    """
    Set servo to a specific angle (0 to 180 degrees).

    Args:
        angle (int or float): The desired angle between 0 and 180.
    """
    # Clamp the angle to valid range
    angle = max(0, min(180, angle))

    # Convert angle to duty cycle (1ms to 2ms pulse width)
    min_duty = 1000  # Corresponds to 0°
    max_duty = 9000  # Corresponds to 180°
    duty = int(min_duty + (angle / 180) * (max_duty - min_duty))

    servo.duty_u16(duty)

set_servo_angle(servoCentre)
