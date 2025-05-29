from machine import Pin, PWM
import time
import re
import sys
import select

# === Serial Setup (Read from USB serial /dev/ttyACM0) ===
import sys
uart = sys.stdin  # Reading from USB serial input (stdin)

# === RGB LED Setup ===
red = Pin(13, Pin.OUT)
green = Pin(14, Pin.OUT)
blue = Pin(15, Pin.OUT)

# === PID Parameters for wall following ===
TARGET_DISTANCE = 300  # mm
Kp = 0.005
Ki = 0.000
Kd = 0.001

pid_integral = 0
last_error = 0


direction='NONE'

# === Button and Buzzer Setup ===
button = Pin(20, Pin.IN, Pin.PULL_UP)
buzzer = PWM(Pin(21))

# === Motor Setup ===
PWMA = PWM(Pin(28))      # PWM for speed
AIN1 = Pin(26, Pin.OUT)  # Direction
AIN2 = Pin(22, Pin.OUT)  # Direction
STBY = Pin(27, Pin.OUT)  # Standby
PWMA.freq(1000)

# === Servo Setup ===
servo = PWM(Pin(0))
servo.freq(50)
servoCentre = 85
TurnRange = 60
servoRMx = servoCentre + (TurnRange / 2)
servoLMn = servoCentre - (TurnRange / 2)

def set_servo_angle(angle):
    angle = max(0, min(180, angle))
    min_duty = 1000
    max_duty = 9000
    duty = int(min_duty + (angle / 180) * (max_duty - min_duty))
    servo.duty_u16(duty)

# === RGB Color Control ===
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

def set_color(color_name, duration=None, mode="solid"):
    color = color_map.get(color_name.lower(), (0, 0, 0))

    if mode == "solid":
        red.value(color[0])
        green.value(color[1])
        blue.value(color[2])
        if duration is not None:
            time.sleep(duration)
            red.value(0)
            green.value(0)
            blue.value(0)

    elif mode == "blink":
        if duration is None:
            while True:
                red.value(color[0])
                green.value(color[1])
                blue.value(color[2])
                time.sleep(0.25)
                red.value(0)
                green.value(0)
                blue.value(0)
                time.sleep(0.25)
        else:
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

# === Buzzer Tones ===
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
    melody = [262, 294, 330, 349]
    for note in melody:
        play_tone(note, 0.3)
        time.sleep(0.1)

# === Motor Control ===
def Run(speed):
    STBY.value(1)
    speed = max(-100, min(100, speed))
    if speed > 0:
        AIN1.value(1)
        AIN2.value(0)
    elif speed < 0:
        AIN1.value(0)
        AIN2.value(1)
    else:
        AIN1.value(0)
        AIN2.value(0)

    duty = int(abs(speed) * 65535 * 0.9 / 100)
    PWMA.duty_u16(duty)
    print(f"Run motor: speed={speed}, duty={duty}")

def Stop():
    AIN1.value(0)
    AIN2.value(0)
    PWMA.duty_u16(0)
    STBY.value(0)
    print("Motor stopped")

# === Button Check ===
def is_button_pressed():
    if button.value() == 0:
        time.sleep(0.05)
        return button.value() == 0
    return False

# === PID Control Function ===
def compute_pid(current_distance):
    global pid_integral, last_error
    if current_distance is None:
        return 0  # No valid reading

    error = current_distance - TARGET_DISTANCE
    pid_integral += error
    derivative = error - last_error
    last_error = error

    output = Kp * error + Ki * pid_integral + Kd * derivative
    return max(min(output, 1), -1)  # Clamp between -1 and 1


# === Move Robot ===
def move(speed, steer):
    steer = max(-100, min(100, steer))
    angle = servoCentre + (steer / 100) * (TurnRange / 2)
    set_servo_angle(angle)
    print(f"Steering: steer={steer}, angle={angle}")
    Run(speed)

# === Startup & End Actions ===
def robotstart():
    print("robot start")
    BuzzerRobotStart()
    set_color("green", duration=2, mode="blink")


def robotend():
    Stop()
    set_color("red", 1, "solid")
    BuzzerRobotEnd()
    print("robot finished")
    

def lidaread():
    print("Waiting for Lidar distances...")
    dir = 0

    pattern = re.compile(r"(\d+(?:\.\d+)?):(\d+(?:\.\d+)?):(\d+(?:\.\d+)?):(\d+(?:\.\d+)?)")

    front = right = left = None

    while not front or not right or not left:
        if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            line_str = sys.stdin.readline().strip()
            match = pattern.match(line_str)
            if match:
                right = float(match.group(1))
                left = float(match.group(2))
                front = float(match.group(3))
                back = float(match.group(4))
                print(f"Initial distances - RIGHT: {right}, LEFT: {left}, FRONT: {front}")
                
def direction():
    global direction  # Use the global direction variable

    print("Starting direction scan...")

    pattern = re.compile(r"(\d+(?:\.\d+)?):(\d+(?:\.\d+)?):(\d+(?:\.\d+)?):(\d+(?:\.\d+)?)")

    front = right = left = None

    while True:
        if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            line_str = sys.stdin.readline().strip()
            match = pattern.match(line_str)
            if match:
                right = float(match.group(1))
                left = float(match.group(2))
                front = float(match.group(3))
                back = float(match.group(4))

                print(f"RIGHT: {right}, LEFT: {left}, FRONT: {front}, BACK: {back}")

                if front > 400:
                    move(80, 0)  # Move forward at 80% speed
                else:
                    Stop()
                    print("Obstacle detected in front within 40 cm. Stopping...")
                    if right > left:
                        direction = "RIGHT"
                        set_color("cyan", duration=2, mode="blink")
                    else:
                        direction = "LEFT"
                        print(f"Direction set to: {direction}")
                        set_color("magenta", duration=2, mode="blink")
                    break

        time.sleep(0.01)

    
        
                
# === Main Function ===
def main():
    
    
    direction()
    
    
    
    """
    set_color("blue", 1, "blink")
   
    lidaread()

    # Inline the previous direction() logic here:
    while front >= 200:
        print("Front obstacle detected at 200mm or closer, stopping robot...")
        move(80, 0)
        if right > left:
            dir = 1
            
        elif left > right:
            dir = 2
        
    print(dir)
            


    # PID loop commented out for now
    
    while True:
        try:
            if is_button_pressed():
                print("Button pressed during run, stopping robot...")
                Stop()
                return

            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                line_str = sys.stdin.readline().strip()
                print(f"Receive... : \"{line_str}\"")
                match = pattern.match(line_str)

                if match:
                    right = float(match.group(1))
                    left = float(match.group(2))
                    front = float(match.group(3))
                    back = float(match.group(4))

                    print(f"RIGHT: {right} mm, LEFT: {left} mm, FRONT: {front} mm, BACK: {back} mm")

                    pid_steer = compute_pid(right)
                    steer_value = int(pid_steer * 100)

                    move(100, steer_value)
                else:
                    print("Line did not match expected format.")

            time.sleep(0.01)

        except KeyboardInterrupt:
            Stop()
            return
    """


# === Program Entry Point ===
try:
    print("Waiting for button press...")
    set_servo_angle(servoCentre)
    while True:
        if is_button_pressed():
            robotstart()
            main()
            robotend()

except KeyboardInterrupt:
    Stop()
    buzzer.duty_u16(0)
    set_color("off")
    set_servo_angle(servoCentre)
    print("Program interrupted by user.")


