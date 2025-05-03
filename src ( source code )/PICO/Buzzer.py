from machine import Pin, PWM
import time

# Buzzer Setup
buzzer = PWM(Pin(21))
buzzer.duty_u16(0)  # Start with buzzer off

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

# Test Functions
BuzzerRobotStart()   # Test Start sound
time.sleep(1)        # Wait 1 second
BuzzerRobotEnd()     # Test End sound
time.sleep(1)        # Wait 1 second
BuzzerRobotError()   # Test Error sound
time.sleep(1)        # Wait 1 second
BuzzerRobotSpecial() # Test Special melody
