from machine import Pin, PWM
import time

# Button and Buzzer Setup
button = Pin(20, Pin.IN, Pin.PULL_UP)  # Button connected to GND
buzzer = PWM(Pin(21))

# RGB LED Setup
red = Pin(13, Pin.OUT)
green = Pin(14, Pin.OUT)
blue = Pin(15, Pin.OUT)

# Common Cathode RGB LED – 1 = ON, 0 = OFF
colors = [
    (1, 0, 0),  # Red
    (0, 1, 0),  # Green
    (0, 0, 1),  # Blue
    (1, 1, 0),  # Yellow
    (0, 1, 1),  # Cyan
    (1, 0, 1),  # Magenta
    (1, 1, 1),  # White
    (0, 0, 0)   # Off
]

color_index = 0

def show_color(r, g, b):
    red.value(r)
    green.value(g)
    blue.value(b)

while True:
    if button.value() == 0:  # Button pressed
        # Beep
        buzzer.freq(1000)
        buzzer.duty_u16(30000)
        
        # Show next color
        show_color(*colors[color_index])
        color_index = (color_index + 1) % len(colors)

        # Wait and stop buzzer
        time.sleep(0.3)
        buzzer.duty_u16(0)
        
        # Wait for button release
        while button.value() == 0:
            time.sleep(0.01)

    # Keep buzzer off if idle
    buzzer.duty_u16(0)
    time.sleep(0.01)

