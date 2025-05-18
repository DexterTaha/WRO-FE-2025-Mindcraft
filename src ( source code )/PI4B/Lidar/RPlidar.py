import subprocess
import re
import time
import statistics

# Run the Lidar command
command = [
    "sudo",
    "/home/mindcraft/rplidar_sdk/output/Linux/Release/ultra_simple",
    "--channel", "--serial", "/dev/serial0", "460800"
]
process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Pattern to extract data
pattern = re.compile(r"theta:\s*([\d.]+)\s+Dist:\s*([\d.]+)\s+Q:\s*(\d+)")

# Classify angles into zones
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
        return None  # Optional: ignore other angles


# Main 4 zones to report
main_zones = {
    'FRONT': ['FRONT', 'FRONT-LEFT', 'FRONT-RIGHT'],
    'RIGHT': ['RIGHT', 'BACK-RIGHT'],
    'BACK': ['BACK', 'BACK-LEFT'],
    'LEFT': ['LEFT', 'FRONT-LEFT'],
}

# Initialize buffers
zone_buffers = {zone: [] for zone in main_zones}
raw_angles = {subzone: [] for group in main_zones.values() for subzone in group}
display_interval = 0.1
last_display = time.time()

def get_smooth_value(distances):
    """Return average of the 3 closest valid distances"""
    valid = [d for d in distances if 50 < d < 5000]
    valid.sort()
    return round(statistics.mean(valid[:3]), 1) if len(valid) >= 1 else None

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

        # Time to display?
        if time.time() - last_display > display_interval:
            for main_zone, subzones in main_zones.items():
                all_distances = []
                for sz in subzones:
                    all_distances.extend(raw_angles.get(sz, []))
                zone_buffers[main_zone] = get_smooth_value(all_distances)

            # Print all zones
            print(" | ".join(
                f"{zone}: {zone_buffers[zone]} mm" if zone_buffers[zone] else f"{zone}: ---"
                for zone in ['FRONT', 'RIGHT', 'BACK', 'LEFT']
            ))

            # Reset buffers
            raw_angles = {k: [] for k in raw_angles}
            last_display = time.time()

except KeyboardInterrupt:
    print("\nStopped by user.")

# Handle errors
stderr = process.stderr.read()
if stderr:
    print("Error:", stderr.decode())
