from machine import Pin
from time import sleep

led = Pin("LED", Pin.OUT)

while True:
    led.toggle()   # Toggle LED state
    sleep(0.5)     # Wait 0.5 seconds

