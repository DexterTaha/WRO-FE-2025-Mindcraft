import subprocess
import re
import time
import statistics
import serial

# === Serial connection to Pico ===
pico = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
time.sleep(2)  # Let Pico reset

# === Run the Lidar command ===
command = [
    "sudo",
    "/home/mindcraft/rplidar_sdk/output/Linux/Release/ultra_simple",
    "--channel", "--serial", "/dev/serial0", "460800"
]
process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# === Regex pattern for Lidar output ===
pattern = re.compile(r"theta:\s*([\d.]+)\s+Dist:\s*([\d.]+)\s+Q:\s*(\d+)")

# === Angle-to-zone classification ===
def angle_to_zone(angle):
    if 255 <= angle < 285:
        return 'BACK'
    elif (345 <= angle <= 360 or 0 <= angle < 15):
        return 'LEFT'
    elif 75 <= angle < 105:
        return 'FRONT'
    elif 165 <= angle < 195:
        return 'RIGHT'
    else:
        return None

# === Zones ===
main_zones = {
    'FRONT': ['FRONT'],
    'RIGHT': ['RIGHT'],
    'BACK': ['BACK'],
    'LEFT': ['LEFT'],
}
zone_buffers = {zone: [] for zone in main_zones}
raw_angles = {subzone: [] for group in main_zones.values() for subzone in group}

display_interval = 0.1
last_display = time.time()

def get_smooth_value(distances):
    valid = [d for d in distances if 50 < d < 5000]
    valid.sort()
    return round(statistics.mean(valid[:3]), 1) if valid else None

# === Wall Following Control Parameters ===
TARGET_DISTANCE = 300  # mm
Kp = 0.001
MAX_STEERING = 1.0
BASE_SPEED = 0.5

def compute_wall_following_control(current_distance):
    if current_distance is None:
        return 0.3, 0.0
    error = current_distance - TARGET_DISTANCE
    if error < -50:  # too close threshold (50 mm inside target)
        steering = -MAX_STEERING  # hard left turn
    else:
        steering = -Kp * error
        steering = max(min(steering, MAX_STEERING), -MAX_STEERING)
    return round(BASE_SPEED, 2), round(steering, 2)


# === Main Loop ===
try:
    for line in process.stdout:
        decoded = line.decode().strip()
        match = pattern.search(decoded)
        if match:
            theta = float(match.group(1))
            distance = float(match.group(2))
            quality = int(match.group(3))

            zone = angle_to_zone(theta)
            if zone and quality > 0:
                raw_angles[zone].append(distance)

        # Every 0.1 seconds
        if time.time() - last_display > display_interval:
            for main_zone, subzones in main_zones.items():
                all_distances = []
                for sz in subzones:
                    all_distances.extend(raw_angles.get(sz, []))
                zone_buffers[main_zone] = get_smooth_value(all_distances)

            # Get distances for all 4 zones (send 0 if None)
            right_distance = zone_buffers.get('RIGHT') or 0
            left_distance = zone_buffers.get('LEFT') or 0
            front_distance = zone_buffers.get('FRONT') or 0
            back_distance = zone_buffers.get('BACK') or 0

            # Format message as "right:left:front:back\n"
            message = f"{right_distance}:{left_distance}:{front_distance}:{back_distance}\n"

            pico.write(message.encode())     # Send distances to Pico
            print(message.strip())  # Print sent message

            raw_angles = {k: [] for k in raw_angles}
            last_display = time.time()


except KeyboardInterrupt:
    print("\nStopped by user.")

# Check for Lidar errors
stderr = process.stderr.read()
if stderr:
    print("Error:", stderr.decode())
