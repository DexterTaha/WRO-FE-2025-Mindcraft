from machine import Pin, PWM

# Pin definitions (based on your setup)
PWMA = PWM(Pin(28))      # PWM control for speed
AIN1 = Pin(26, Pin.OUT)  # Direction control
AIN2 = Pin(22, Pin.OUT)  # Direction control
STBY = Pin(27, Pin.OUT)  # Standby pin

# Initialize PWM frequency
PWMA.freq(1000)  # 1kHz is typical for motors

def Run(speed):
    """Control motor speed and direction.
    speed: -100 to 100, where sign = direction
    """
    # Activate the motor driver
    STBY.value(1)

    # Clamp speed to -100 to 100
    speed = max(-100, min(100, speed))

    if speed < 0:
        AIN1.value(1)
        AIN2.value(0)
    elif speed > 0:
        AIN1.value(0)
        AIN2.value(1)
    else:
        # Stop the motor
        AIN1.value(0)
        AIN2.value(0)

    # Convert speed to duty cycle (0-65535)
    duty = int(abs(speed) * 65535 / 100)
    PWMA.duty_u16(duty)

# Example test
Run(0)    # Clockwise at 50% speed
# Run(-75) # Counter-clockwise at 75% speed
# Run(0)   # Stop

