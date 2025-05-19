from machine import Pin, PWM
import time

# RGB LED Setup
red = Pin(13, Pin.OUT)
green = Pin(14, Pin.OUT)
blue = Pin(15, Pin.OUT)

# Button and Buzzer Setup
button = Pin(20, Pin.IN, Pin.PULL_UP)  # Button connected to GND
buzzer = PWM(Pin(21))

# Pin definitions (based on your setup)
PWMA = PWM(Pin(28))      # PWM control for speed
AIN1 = Pin(26, Pin.OUT)  # Direction control
AIN2 = Pin(22, Pin.OUT)  # Direction control
STBY = Pin(27, Pin.OUT)  # Standby pin

# Initialize PWM frequency
PWMA.freq(1000)  # 1kHz is typical for motors
# Function to produce a tone for a given frequency and duration
def play_tone(frequency, duration):
    buzzer.freq(frequency)
    buzzer.duty_u16(30000)  # Medium volume
    time.sleep(duration)
    buzzer.duty_u16(0)  # Turn off buzzer after sound

# Buzzer for Robot Start
def BuzzerRobotStart():
    for _ in range(3):
        play_tone(1000, 0.2)  # 1000 Hz for 200ms
        time.sleep(0.1)        # Short pause between tones

# Buzzer for Robot End
def BuzzerRobotEnd():
    play_tone(500, 1)  # 500 Hz for 1 second

# Buzzer for Robot Error
def BuzzerRobotError():
    for _ in range(5):
        play_tone(1500, 0.1)  # 1500 Hz for 100ms
        time.sleep(0.1)        # Short pause between tones

# Buzzer for Robot Special (Melody)

def BuzzerRobotSpecial():
    melody = [262, 294, 330, 349]  # Notes for C, D, E, F
    for note in melody:
        play_tone(note, 0.3)  # Each note lasts for 300ms
        time.sleep(0.1)        # Short pause between notes
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
