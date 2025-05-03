from machine import Pin
import time

# Setup RGB pins
red = Pin(13, Pin.OUT)
green = Pin(14, Pin.OUT)
blue = Pin(15, Pin.OUT)

# Common cathode logic (HIGH = ON)
# If you're using a common anode RGB LED, reverse 0/1 values

colors = [
    (1, 0, 0),  # Red
    (0, 1, 0),  # Green
    (0, 0, 1),  # Blue
    (1, 1, 0),  # Yellow
    (0, 1, 1),  # Cyan
    (1, 0, 1),  # Magenta
    (1, 1, 1),  # White
    (0, 0, 0),  # Off
]

while True:
    for color in colors:
        red.value(color[0])
        green.value(color[1])
        blue.value(color[2])
        time.sleep(0.5)

