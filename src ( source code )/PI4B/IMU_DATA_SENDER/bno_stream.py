#! /usr/bin/env python3
import board
import adafruit_bno055
import sys
import socket
import time
import argparse
import math

parser = argparse.ArgumentParser(description="BNO055 rotation detector")
parser.add_argument("--debug", action="store_true", help="Enable debug printing")
parser.add_argument("--invert", action="store_true", help="Invert yaw around 180°")
parser.add_argument("--target-rotations", type=int, default=0, help="Stop after this many full rotations (positive=CW, negative=CCW, 0=disabled)")
parser.add_argument("--require-direction", choices=["any", "CW", "CCW"], default="any", help="Require rotations to be in this direction")
parser.add_argument("--udp-ip", default="127.0.0.1", help="UDP target IP")
parser.add_argument("--udp-port", type=int, default=5006, help="UDP target port")
parser.add_argument("--delay", type=float, default=0.015, help="Main loop delay (s)")
parser.add_argument("--threshold", type=float, default=0.1, help="Yaw delta threshold (degrees) to register motion")
args = parser.parse_args()

DEBUG = args.debug
INVERT = args.invert
TARGET = args.target_rotations
REQUIRE_DIR = args.require_direction
UDP_IP = args.udp_ip
UDP_PORT = args.udp_port
DELAY = args.delay
THRESH = args.threshold

COLOR_LABEL = "\033[97m"
COLOR_EULER = "\033[92m"
COLOR_SEQ = "\033[94m"
COLOR_ROT = "\033[95m"
COLOR_ALERT = "\033[93m"
COLOR_RESET = "\033[0m"

i2c = board.I2C()
sensor = adafruit_bno055.BNO055_I2C(i2c)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def safe(val):
    return 0.0 if val is None else float(val)

def invert_yaw_around_180(yaw):
    return (540.0 - yaw) % 360.0

def apply_invert(yaw):
    return invert_yaw_around_180(yaw) if INVERT else yaw

def unwrap(prev, cur):
    delta = cur - prev
    if delta > 180.0:
        delta -= 360.0
    elif delta <= -180.0:
        delta += 360.0
    return prev + delta, delta

seq = 0
prev_unwrapped = None
accum_yaw = 0.0
total_rot = 0

try:
    while True:
        e = sensor.euler or (0.0, 0.0, 0.0)
        raw_heading, roll, pitch = safe(e[0]), safe(e[1]), safe(e[2])

        # --- linear acceleration ---
        la = sensor.linear_acceleration or (0.0, 0.0, 0.0)
        lin_x, lin_y = safe(la[0]), safe(la[1])

        # sanitize yaw
        if not math.isfinite(raw_heading) or abs(raw_heading) > 1000.0:
            if DEBUG:
                print(f"{COLOR_LABEL}⚠ Skipping bad heading: {raw_heading}{COLOR_RESET}")
            time.sleep(DELAY)
            continue

        yaw_normalized = raw_heading % 360.0
        yaw = apply_invert(yaw_normalized)

        if prev_unwrapped is None:
            prev_unwrapped = yaw
            time.sleep(DELAY)
            continue

        unwrapped, delta = unwrap(prev_unwrapped, yaw)
        prev_unwrapped = unwrapped

        if abs(delta) < THRESH:
            delta = 0.0
            rotation_dir = None
        else:
            accum_yaw += delta
            rotation_dir = "CW" if delta > 0 else "CCW"

            while accum_yaw >= 360.0:
                total_rot += 1
                accum_yaw -= 360.0
            while accum_yaw <= -360.0:
                total_rot -= 1
                accum_yaw += 360.0

        # include lin_x and lin_y in UDP message
        msg = f"{roll:.2f},{pitch:.2f},{yaw:.2f},{delta:.3f},{rotation_dir},{total_rot},{lin_x:.3f},{lin_y:.3f}\n"
        try:
            sock.sendto(msg.encode("utf-8"), (UDP_IP, UDP_PORT))
        except Exception:
            pass

        if DEBUG:
            seq += 1
            print(
                f"{COLOR_LABEL}Euler:{COLOR_RESET} {COLOR_EULER}Roll={roll:6.2f} Pitch={pitch:6.2f} Yaw={yaw:6.2f}°{COLOR_RESET} | "
                f"{COLOR_ROT}Dir={rotation_dir or '-':4s} Δ={delta:6.2f}°{COLOR_RESET} | "
                f"{COLOR_ALERT}Cum={accum_yaw:8.2f}° ({total_rot:+d} rot){COLOR_RESET} | "
                f"{COLOR_LABEL}LinX={lin_x:6.2f} LinY={lin_y:6.2f}{COLOR_RESET} | {COLOR_SEQ}Seq: {seq:5d}{COLOR_RESET}"
            )

        # stop condition
        if TARGET != 0:
            reached = False
            if TARGET > 0 and total_rot >= TARGET:
                reached = True
            elif TARGET < 0 and total_rot <= TARGET:
                reached = True

            if reached:
                if REQUIRE_DIR == "any":
                    print(f"{COLOR_ALERT}Target reached: {total_rot:+d} rotations{COLOR_RESET}")
                    break
                elif REQUIRE_DIR == "CW" and total_rot > 0:
                    print(f"{COLOR_ALERT}Target reached: {total_rot:+d} rotations (CW){COLOR_RESET}")
                    break
                elif REQUIRE_DIR == "CCW" and total_rot < 0:
                    print(f"{COLOR_ALERT}Target reached: {total_rot:+d} rotations (CCW){COLOR_RESET}")
                    break

        time.sleep(DELAY)

except KeyboardInterrupt:
    if DEBUG:
        print(f"\n{COLOR_LABEL}Exiting...{COLOR_RESET}")
    sys.exit(0)

