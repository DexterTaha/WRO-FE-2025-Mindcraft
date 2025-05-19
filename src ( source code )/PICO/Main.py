from machine import Pin, PWM
import time

# RGB LED Setup
red = Pin(13, Pin.OUT)
green = Pin(14, Pin.OUT)
blue = Pin(15, Pin.OUT)

# Button and Buzzer Setup
button = Pin(20, Pin.IN, Pin.PULL_UP)  # Button connected to GND
buzzer = PWM(Pin(21))

# Motor Setup
PWMA = PWM(Pin(28))      # PWM for speed
AIN1 = Pin(26, Pin.OUT)  # Direction
AIN2 = Pin(22, Pin.OUT)  # Direction
STBY = Pin(27, Pin.OUT)  # Standby

# Start with buzzer off
buzzer.duty_u16(0)
PWMA.freq(1000)

# RGB color map
color_map = {
    "red":     (1, 0, 0),
    "green":   (0, 1, 0),
    "blue":    (0, 0, 1),
    "yellow":  (1, 1, 0),
    "cyan":    (0, 1, 1),
    "magenta": (1, 0, 1),
    "white":   (1, 1, 1),
    "off":     (0, 0, 0)
}

def set_color(color_name, duration=1.0, mode="solid"):
    color = color_map.get(color_name.lower(), (0, 0, 0))
    if mode == "solid":
        red.value(color[0])
        green.value(color[1])
        blue.value(color[2])
        time.sleep(duration)
        red.value(0)
        green.value(0)
        blue.value(0)
    elif mode == "blink":
        blink_times = int(duration / 0.5)
        for _ in range(blink_times):
            red.value(color[0])
            green.value(color[1])
            blue.value(color[2])
            time.sleep(0.25)
            red.value(0)
            green.value(0)
            blue.value(0)
            time.sleep(0.25)

def play_tone(frequency, duration):
    buzzer.freq(frequency)
    buzzer.duty_u16(30000)
    time.sleep(duration)
    buzzer.duty_u16(0)

def BuzzerRobotStart():
    for _ in range(3):
        play_tone(1000, 0.2)
        time.sleep(0.1)

def BuzzerRobotEnd():
    play_tone(500, 1)

def BuzzerRobotError():
    for _ in range(5):
        play_tone(1500, 0.1)
        time.sleep(0.1)

def BuzzerRobotSpecial():
    melody = [262, 294, 330, 349]  # C, D, E, F
    for note in melody:
        play_tone(note, 0.3)
        time.sleep(0.1)

def Run(speed):
    """Run the motor with given speed [-100 to 100]"""
    STBY.value(1)
    speed = max(-100, min(100, speed))

    if speed < 0:
        AIN1.value(1)
        AIN2.value(0)
    elif speed > 0:
        AIN1.value(0)
        AIN2.value(1)
    else:
        AIN1.value(0)
        AIN2.value(0)

    duty = int(abs(speed) * 65535 * 0.9 / 100)  # limit to 90% PWM
    PWMA.duty_u16(duty)
    print(f"Run motor: speed={speed}, duty={duty}")

def Stop():
    AIN1.value(0)
    AIN2.value(0)
    PWMA.duty_u16(0)
    STBY.value(0)
    print("Motor stopped")

def is_button_pressed():
    if button.value() == 0:
        time.sleep(0.05)  # debounce
        return button.value() == 0
    return False

# Main Program Loop
try:
    print("Waiting for button press...")
    while True:
        if is_button_pressed():
            print("Button pressed!")
            BuzzerRobotStart()
            set_color("green", 1, "blink")

            Run(80)
            time.sleep(2)

            Run(-80)
            time.sleep(2)

            Stop()
            set_color("red", 1, "solid")
            BuzzerRobotEnd()

            print("Cycle complete. Waiting for next press...")

except KeyboardInterrupt:
    Stop()
    buzzer.duty_u16(0)
    set_color("off")
    print("Program interrupted by user.")
