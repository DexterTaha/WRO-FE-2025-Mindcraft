#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <thread>
#include <numeric>
#include <chrono>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>
#include <algorithm>
#include <limits>
#include <atomic>

using namespace std;
using namespace std::chrono;

std::atomic<bool> wallFollowStop(false);  // new flag to control wall-following per lap


// ------------------- GLOBALS -------------------
struct LidarPoint {
    float angle;
    float distance;
    int quality;
};

// LIDAR data updated in background
std::vector<LidarPoint> latest_points;
std::atomic<bool> ctrl_c_pressed(false);

// I2C configuration
const char* I2C_BUS = "/dev/i2c-1";  
const int ESP32_ADDR = 0x08;         

// PID constants for arc
float Kp = 2.0, Ki = 0.0, Kd = 0.1;

// ------------------- SIGNAL HANDLER -------------------
void ctrlc_handler(int) { ctrl_c_pressed = true; }

// ------------------- I2C COMMANDS -------------------
bool sendCommand(const string& cmd) {
    int file = open(I2C_BUS, O_RDWR);
    if (file < 0) { cerr << "❌ Failed to open I2C bus\n"; return false; }
    if (ioctl(file, I2C_SLAVE, ESP32_ADDR) < 0) { cerr << "❌ Failed to connect to I2C\n"; close(file); return false; }
    string fullCmd = cmd + "\n";
    if (write(file, fullCmd.c_str(), fullCmd.length()) != (ssize_t)fullCmd.length()) {
        cerr << "❌ Failed to write to I2C device\n"; close(file); return false;
    }
    close(file);
    cout << "✅ Sent: " << fullCmd;
    return true;
}

// ------------------- ENCODER & IMU -------------------
long readEncoder() {
    int file = open(I2C_BUS, O_RDWR);
    if (file < 0) return -1;
    if (ioctl(file, I2C_SLAVE, ESP32_ADDR) < 0) { close(file); return -1; }
    char buf[32] = {0};
    if (read(file, buf, sizeof(buf)-1) < 0) { close(file); return -1; }
    close(file);
    string s(buf);
    size_t pos = s.find("ENC:");
    if (pos != string::npos) return stol(s.substr(pos+4));
    return -1;
}

struct IMUData {
    float roll, pitch, yaw, delta;
    string rotation_dir;
    int total_rot;
    float lin_x, lin_y;
};

IMUData readIMUData(const string& path="/tmp/bno_imu.txt") {
    IMUData imu{};
    ifstream f(path);
    if (f.is_open()) {
        string line;
        if (getline(f, line)) {
            stringstream ss(line);
            string token;
            if (getline(ss, token, ',')) imu.roll = stof(token);
            if (getline(ss, token, ',')) imu.pitch = stof(token);
            if (getline(ss, token, ',')) imu.yaw = stof(token);
            if (getline(ss, token, ',')) imu.delta = stof(token);
            if (getline(ss, token, ',')) imu.rotation_dir = token;
            if (getline(ss, token, ',')) imu.total_rot = stoi(token);
            if (getline(ss, token, ',')) imu.lin_x = stof(token);
            if (getline(ss, token, ',')) imu.lin_y = stof(token);
        }
    }
    return imu;
}

// ------------------- LIDAR UTILS -------------------
static float normalizeAngle(float a) {
    while (a < 0.0f) a += 360.0f;
    while (a >= 360.0f) a -= 360.0f;
    return a;
}

static float medianOf(std::vector<float>& v) {
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2) return v[n / 2];
    return (v[n / 2 - 1] + v[n / 2]) * 0.5f;
}

float readLIDAR_median_cm(const std::vector<LidarPoint>& points,
                          float angle_min, float angle_max,
                          int quality_threshold = 10,
                          float max_valid_mm = 60000.0f) {
    std::vector<float> distances_mm;

    angle_min = normalizeAngle(angle_min);
    angle_max = normalizeAngle(angle_max);

    auto in_interval = [&](float a) -> bool {
        a = normalizeAngle(a);
        if (angle_min <= angle_max) return (a >= angle_min && a <= angle_max);
        return (a >= angle_min || a <= angle_max);
    };

    for (const auto& p : points) {
        if (p.distance <= 0.0f) continue;             // ignore invalid
        if (p.quality < quality_threshold) continue;  // ignore low-quality
        if (p.distance > max_valid_mm) continue;      // ignore absurdly large
        if (in_interval(p.angle)) distances_mm.push_back(p.distance);
    }

    if (distances_mm.empty()) return -1.0f;

    float median_mm = medianOf(distances_mm);
    return median_mm / 10.0f; // convert mm → cm
}


void waitLIDAR(const std::vector<LidarPoint>& points,
               float angle_min, float angle_max,
               float target_distance_cm) {
    bool detected = false;
    while (!detected && !ctrl_c_pressed) {
        float dist = readLIDAR_median_cm(points, angle_min, angle_max);
        if (dist > 0 && dist <= target_distance_cm) {
            sendCommand("M_STOP");
            cout << "[⚠️ WAIT LIDAR] Object detected at " << dist << " cm → STOP\n";
            detected = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ------------------- ARC -------------------
void arc(int direction, float baseSpeed, float degrees, float steerPercent=100.0) {
    cout << "\nStarting arc " << (direction>0 ? "LEFT" : "RIGHT")
         << " for " << degrees << "° with " << steerPercent << "% steering\n";

    IMUData imu = readIMUData();
    float startYaw = imu.yaw;
    float targetYaw = fmod(startYaw + direction * degrees, 360.0f);
    if (targetYaw < 0) targetYaw += 360.0f;

    float integral = 0.0, prevError = 0.0;
    auto lastTime = high_resolution_clock::now();

    int steerAngle = 90 + int(direction * steerPercent / 2.0);
    steerAngle = max(0, min(180, steerAngle));
    sendCommand("SERVO_ANG:" + to_string(steerAngle));

    while (true) {
        imu = readIMUData();
        float currentYaw = imu.yaw;
        float error = targetYaw - currentYaw;
        if (error > 180.0) error -= 360.0;
        if (error < -180.0) error += 360.0;
        if (fabs(error) < 0.5) break;

        auto now = high_resolution_clock::now();
        float dt = duration<float>(now - lastTime).count();
        lastTime = now;

        integral += error * dt;
        float derivative = (error - prevError) / dt;
        prevError = error;

        float correction = Kp*error + Ki*integral + Kd*derivative;
        float speed = baseSpeed - correction/50.0;
        speed = max(0.0f, min(speed, 1.0f));
        int cmdSpeed = int(speed * 100);
        if (cmdSpeed < 15) cmdSpeed = 15;
        sendCommand("M_SPEED:" + to_string(cmdSpeed));

        cout << "Yaw=" << currentYaw << " | Target=" << targetYaw
             << " | Speed=" << speed << " | Steer=" << steerAngle << endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    sendCommand("M_SPEED:0");
    sendCommand("SERVO_ANG:90");
    cout << "Arc completed\n";
}



void arc90Back(int direction, int steerPercent = 120) {
    cout << "\nStarting ultra-fast 90° backward arc "
         << (direction > 0 ? "LEFT" : "RIGHT") << " (rear steering)\n";

    // --- Read starting yaw ---
    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    // --- Compute target yaw for exact 90° rotation ---
    float targetYaw = fmod(startYaw - direction * 90.0f, 360.0f);
    if (targetYaw < 0) targetYaw += 360.0f;

    // --- Rear steering: partial turn, fixed outside loop ---
    // Adjust for rear steering physically reversed
    int steerAngle = 90 + int(direction * steerPercent);
    steerAngle = max(0, min(180, steerAngle)); // clamp
    sendCommand("SERVO_ANG:" + to_string(steerAngle));
    cout << "Steering angle set to " << steerAngle << endl;

    // --- Start full backward speed ---
    sendCommand("M_SPEED:-200");  // backward speed 100%

    // --- Loop until robot reaches target yaw ---
    while (true) {
        imu = readIMUData();
        float currentYaw = imu.yaw;

        // Compute shortest angle difference
        float diff = targetYaw - currentYaw;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        if (fabs(diff) < 1.0f) break;  // stop at 1° tolerance
        this_thread::sleep_for(chrono::milliseconds(10));  // fast update
    }

    // --- Full stop and straighten rear steering ---
    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
    this_thread::sleep_for(chrono::milliseconds(200)); // stabilize

    cout << "Ultra-fast backward 90° arc completed. Rear steering centered.\n";
}



// ------------------- GO -------------------
void go(int direction = 1, int speedPercent = 200, float Kp_steer = 0.3, float maxSteerCorrection = 15.0, bool invertRear = true)
{
    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    while (!ctrl_c_pressed) {
        imu = readIMUData();
        float currentYaw = imu.yaw;

        float relativeYaw = currentYaw - startYaw;
        if (relativeYaw > 180.0f) relativeYaw -= 360.0f;
        if (relativeYaw < -180.0f) relativeYaw += 360.0f;

        if (fabs(relativeYaw) < 0.3f) relativeYaw = 0;

        float correction = Kp_steer * relativeYaw;
        if (invertRear) correction = -correction; // flip for rear steering

        if (correction > maxSteerCorrection) correction = maxSteerCorrection;
        if (correction < -maxSteerCorrection) correction = -maxSteerCorrection;

        int steerAngle = int(90 + correction);
        steerAngle = std::max(0, std::min(180, steerAngle));

        sendCommand("SERVO_ANG:" + std::to_string(steerAngle));
        sendCommand("M_SPEED:" + std::to_string(direction * speedPercent));

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
}



void followRightWallRearStableYaw(float targetDistanceCm = 30.0f,
                                  float baseSpeedPercent = 80,
                                  float Kp = 2.0f,
                                  float Ki = 0.0f,
                                  float Kd = 0.5f,
                                  float maxSteerCorrection = 20.0f)
{
    float integral = 0.0f;
    float prevError = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    while (!ctrl_c_pressed && !wallFollowStop) {  // check the new stop flag
        imu = readIMUData();
        float currentYaw = imu.yaw;

        float relativeYaw = currentYaw - startYaw;
        if (relativeYaw > 180.0f) relativeYaw -= 360.0f;
        if (relativeYaw < -180.0f) relativeYaw += 360.0f;

        if (fabs(relativeYaw) < 0.2f) relativeYaw = 0;

        float rightDist = readLIDAR_median_cm(latest_points, 10.0f, 50.0f);
        if (rightDist <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        float error = targetDistanceCm - rightDist;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        integral += error * dt;
        float derivative = (error - prevError) / dt;
        prevError = error;

        float correction = Kp * error + Ki * integral + Kd * derivative;

        if (correction > maxSteerCorrection) correction = maxSteerCorrection;
        if (correction < -maxSteerCorrection) correction = -maxSteerCorrection;

        int steerAngle = int(90 + correction);
        steerAngle = std::max(0, std::min(180, steerAngle));

        sendCommand("SERVO_ANG:" + std::to_string(steerAngle));
        sendCommand("M_SPEED:" + std::to_string(int(baseSpeedPercent)));

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
}



// ------------------- LIDAR RECEIVER THREAD -------------------
void lidarReceiver(int sock) {
    const int buffer_size = 65536;
    char buffer[buffer_size];

    while (!ctrl_c_pressed) {
        sockaddr_in sender_addr{};
        socklen_t sender_len = sizeof(sender_addr);

        ssize_t len = recvfrom(sock, buffer, buffer_size - 1, 0,
                               (struct sockaddr*)&sender_addr, &sender_len);
        if (len < 0) continue;

        buffer[len] = '\0';
        std::string line(buffer);
        if (line.empty()) continue;

        std::vector<LidarPoint> points;
        std::istringstream ss(line);
        std::string item;

        while (ss >> item) {
            std::istringstream item_ss(item);
            std::string angle_str, dist_str, quality_str;

            if (!std::getline(item_ss, angle_str, ',')) continue;
            if (!std::getline(item_ss, dist_str, ',')) continue;
            if (!std::getline(item_ss, quality_str, ',')) continue;

            LidarPoint p;
            p.angle = std::stof(angle_str);
            p.distance = std::stof(dist_str);
            p.quality = std::stoi(quality_str);
            points.push_back(p);
        }

        latest_points = points;
    }
}

// ------------------- MAIN -------------------
int main() {
    signal(SIGINT, ctrlc_handler);

    // Reset globals for safe multiple runs
    ctrl_c_pressed = false;
    wallFollowStop = false;

    const char* ip = "127.0.0.1";
    const int port = 5005;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    std::cout << "Listening on " << ip << ":" << port << std::endl;

    // Start LIDAR receiver thread
    std::thread lidarThread(lidarReceiver, sock);

    // --- Wait until some LIDAR points are received ---
    while (latest_points.empty() && !ctrl_c_pressed) {
        std::cout << "Waiting for initial LIDAR points...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ------------------- MAIN LAP LOOP -------------------
    for (int lap = 0; lap < 12; ++lap) {
        std::cout << "\n=== Lap " << (lap + 1) << " ===\n";

        wallFollowStop = false;

        // Start wall-following in a thread
        std::thread wallFollowThread([]() {
            followRightWallRearStableYaw(30.0f, -200, 1.0f, 0.0f, 0.5f, 30.0f);
        });

        // Wait for front distance <= threshold
        bool distanceReached = false;
        int consecutive = 0;
        const int requiredConsecutive = 3;

        while (!distanceReached && !ctrl_c_pressed) {
            float dist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);

            // Only consider reasonable distances
            if (dist > 10.0f && dist <= 60.0f) {
                consecutive++;
                if (consecutive >= requiredConsecutive) {
                    distanceReached = true;
                    wallFollowStop = true;
                    sendCommand("M_STOP");
                    std::cout << "[⚠️ WAIT LIDAR] Object detected at " << dist << " cm → STOP\n";
                }
            } else {
                consecutive = 0; // reset if invalid or out of range
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Stop wall-following thread cleanly
        wallFollowThread.join();
        sendCommand("SERVO_ANG:90");

        // Perform lap turn
        arc90Back(1, 60);

        // Optional short delay before next lap
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "All 4 laps completed.\n";

    // Ensure LIDAR thread exits
    ctrl_c_pressed = true;
    lidarThread.join();

    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
    close(sock);

    return 0;
}
