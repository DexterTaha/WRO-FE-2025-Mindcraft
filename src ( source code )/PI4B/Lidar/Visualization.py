import subprocess
import re
import time
import matplotlib.pyplot as plt
import numpy as np
import threading
from matplotlib.widgets import CheckButtons

# Start the Lidar process
command = [
    "sudo",
    "/home/mindcraft/rplidar_sdk/output/Linux/Release/ultra_simple",
    "--channel", "--serial", "/dev/serial0", "460800"
]
process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

# Regex to extract angle and distance
pattern = re.compile(r"theta:\s*([\d.]+)\s+Dist:\s*([\d.]+)\s+Q:\s*(\d+)")

# Data buffers
angles = []
distances = []

# Lock for thread safety
data_lock = threading.Lock()

def read_lidar_data():
    for line in process.stdout:
        decoded = line.decode().strip()
        match = pattern.search(decoded)
        if match:
            theta = 360.0 - float(match.group(1))  # Swap left and right
            distance = float(match.group(2))
            quality = int(match.group(3))
            if quality > 0 and 50 < distance < 5000:
                with data_lock:
                    angles.append((np.deg2rad(theta) + np.pi) % (2 * np.pi))  # Rotate 180°
                    distances.append(distance)



# Start data reading in a separate thread
reader_thread = threading.Thread(target=read_lidar_data)
reader_thread.daemon = True
reader_thread.start()

# Plot setup
plt.ion()
fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, polar=True)

# Set background colors to black
fig.patch.set_facecolor('black')
ax.set_facecolor('black')

# Set text color to white
ax.title.set_color('white')
ax.tick_params(colors='white')
ax.grid(color='gray', linestyle='--', linewidth=0.5)

# Initial empty scatter plot (points white)
scatter = ax.scatter([], [], s=5, c='white')
ax.set_ylim(0, 1000)
ax.set_title("Live Lidar Visualization")

# Add switches
switch_ax = plt.axes([0.75, 0.8, 0.15, 0.1])  # [left, bottom, width, height]
switch_ax.set_facecolor('black')
check = CheckButtons(switch_ax, ['Switch Left', 'Switch Right'], [False, False])
for label in switch_ax.get_xticklabels() + switch_ax.get_yticklabels():
    label.set_color('white')

# Example event handler
def switch_callback(label):
    print(f"{label} toggled")

check.on_clicked(switch_callback)

try:
    while True:
        with data_lock:
            theta_data = np.array(angles)
            dist_data = np.array(distances)
            angles.clear()
            distances.clear()

        if len(theta_data) > 0:
            ax.clear()

            # Reset background and colors after clearing
            ax.set_facecolor('black')
            ax.set_ylim(0, 1000)
            ax.set_title("Live Lidar Visualization", color='white')
            ax.tick_params(colors='white')
            ax.grid(color='gray', linestyle='--', linewidth=0.5)

            ax.scatter(theta_data, dist_data, s=5, c='white')
            plt.pause(0.01)

except KeyboardInterrupt:
    print("Visualization stopped.")

stderr = process.stderr.read()
if stderr:
    print("Error:", stderr.decode())
