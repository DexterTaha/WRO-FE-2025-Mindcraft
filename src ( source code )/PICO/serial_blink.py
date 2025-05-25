from machine import Pin
import sys

led = Pin("LED", Pin.OUT)

print("Pico ready. Send '1' or '0' to turn LED ON or OFF.")

while True:
    cmd = sys.stdin.readline().strip()
    if not cmd:
        continue
    print("Received:", cmd)
    if cmd == '1':
        led.value(1)
        print("LED ON")
    elif cmd == '0':
        led.value(0)
        print("LED OFF")
    else:
        print("Unknown command")
