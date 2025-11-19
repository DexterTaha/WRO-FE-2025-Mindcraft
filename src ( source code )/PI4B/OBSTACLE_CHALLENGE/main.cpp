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
std::atomic<bool> arcInProgress(false);  // atomic for thread safety



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
constexpr const char* GREEN = "\033[32m";
constexpr const char* RED   = "\033[31m";
constexpr const char* RESET = "\033[0m";

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



void arc90Back(int direction, int steerPercent = 120, float stopThresholdDeg = 20.0f) {
    arcInProgress = true;  // prevent other threads from stopping motors

    std::cout << "\nStarting 90° backward arc "
              << (direction > 0 ? "LEFT" : "RIGHT") << " (rear steering)\n";

    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    float targetYaw = fmod(startYaw + direction * 90.0f, 360.0f);
    if (targetYaw < 0) targetYaw += 360.0f;

    int steerAngle = 90 - int(direction * steerPercent);
    steerAngle = std::max(0, std::min(180, steerAngle));
    sendCommand("SERVO_ANG:" + std::to_string(steerAngle));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (!ctrl_c_pressed) {
        imu = readIMUData();
        float currentYaw = imu.yaw;

        float diff = targetYaw - currentYaw;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        if (fabs(diff) < stopThresholdDeg) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sendCommand("SERVO_ANG:90");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    arcInProgress = false;  // allow other threads to send commands

    std::cout << "✅ Backward 90° arc completed. Final yaw ≈ "
              << imu.yaw << "° (target " << targetYaw << "°)\n";
}
void arc90Forward(int direction, int steerPercent = 120, float stopThresholdDeg = 25.0f) {
    arcInProgress = true;  // prevent other threads from stopping motors

    std::cout << "\nStarting 90° forward arc "
              << (direction > 0 ? "LEFT" : "RIGHT") << " (front steering)\n";

    // --- INITIAL ORIENTATION ---
    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    // Target after turning 90 degrees
    float targetYaw = fmod(startYaw + direction * 90.0f, 360.0f);
    if (targetYaw < 0) targetYaw += 360.0f;

    // --- STEERING ANGLE ---
    // MIRROR of backward:
    // Backward used:   steer = 90 - direction * steerPercent
    // Forward becomes: steer = 90 + direction * steerPercent
    int steerAngle = 90 + int(direction * steerPercent);
    steerAngle = std::max(0, std::min(180, steerAngle));

    sendCommand("SERVO_ANG:" + std::to_string(steerAngle));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // --- MOTOR FORWARD ---
    sendCommand("M_SPEED:60");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // --- CONTROL LOOP ---
    while (!ctrl_c_pressed) {
        imu = readIMUData();
        float currentYaw = imu.yaw;

        float diff = targetYaw - currentYaw;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        if (fabs(diff) < stopThresholdDeg) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // --- STOP + RESET ---
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    sendCommand("SERVO_ANG:90");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    arcInProgress = false;

    std::cout << "✅ Forward 90° arc completed. Final yaw ≈ "
              << imu.yaw << "° (target " << targetYaw << "°)\n";
}




// ------------------- LIDAR RECEIVER THREAD -------------------
void lidarReceiver(int sock) {
    const int buffer_size = 65536;
    char buffer[buffer_size];

    while (!ctrl_c_pressed) {
if (ctrl_c_pressed) break;  
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










void followRightWallRearStableYaw(float targetDistanceCm = 30.0f,
                                  float baseSpeedPercent = 30,
                                  float Kp = 2.0f,
                                  float Ki = 0.0f,
                                  float Kd = 0.5f,
                                  float maxSteerCorrection = 10.0f)
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




void followLeftWallRearStableYaw(float targetDistanceCm = 30.0f,
                                  float baseSpeedPercent = 30,
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

        float leftDist = readLIDAR_median_cm(latest_points, 130.0f, 170.0f);
        if (leftDist <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        float error = targetDistanceCm - leftDist;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        integral += error * dt;
        float derivative = (error - prevError) / dt;
        prevError = error;

        float correction = -(Kp * error + Ki * integral + Kd * derivative);

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


void bootMenuCheck() {
    cout << "==== ROBOT BOOT MENU ====\n";
    this_thread::sleep_for(chrono::milliseconds(500));

    // --- IMU check ---
    cout << "Checking IMU... ";
    IMUData imu = readIMUData();
    this_thread::sleep_for(chrono::milliseconds(500));

    if (imu.total_rot != 0 || imu.yaw != 0.0f || imu.roll != 0.0f) {
        cout << GREEN << "IMU available" << RESET << "\n";
    } else {
        cout << RED << "IMU NOT detected!" << RESET << "\n";
    }

    // --- LIDAR check ---
    cout << "Checking LIDAR... ";
    this_thread::sleep_for(chrono::milliseconds(500));

    // Try to read median distance in front 30° sector (angle 345° → 15°)
    float medianFrontCm = readLIDAR_median_cm(latest_points, 345.0f, 15.0f);
    if (medianFrontCm > 0) {
        cout << GREEN << "LIDAR available" << RESET 
             << " | Front distance ≈ " << medianFrontCm << " cm\n";
    } else {
        cout << RED << "LIDAR NOT detected!" << RESET << "\n";
    }

    cout << "\nBoot check complete. Starting robot...\n";
    this_thread::sleep_for(chrono::milliseconds(500));
}


void arcVariableBack(int direction, float targetDegree, int steerPercent = 120, float toleranceDeg = 2.5f) {
    arcInProgress = true; // Prevent other threads from stopping motors

    std::cout << "\nStarting backward arc: Target " << targetDegree << "° "
              << (direction > 0 ? "LEFT" : "RIGHT") << " (rear steering)\n";

    IMUData imu = readIMUData();
    float startYaw = imu.yaw;

    // Calculate the target yaw, ensuring it wraps correctly between 0 and 360 degrees
    float targetYaw = fmod(startYaw + direction * targetDegree, 360.0f);
    if (targetYaw < 0) targetYaw += 360.0f;

    // Calculate and clamp the servo angle for steering
    int steerAngle = 90 - int(direction * steerPercent);
    steerAngle = std::max(0, std::min(180, steerAngle));
    sendCommand("SERVO_ANG:" + std::to_string(steerAngle));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Start moving backward
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (!ctrl_c_pressed) {
        imu = readIMUData();
        float currentYaw = imu.yaw;

        // Calculate the difference between target and current yaw, handling 360/0 wrap-around
        float diff = targetYaw - currentYaw;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        // Stop if the error is within the tolerance
        if (fabs(diff) < toleranceDeg) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Stop and straighten wheels
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sendCommand("SERVO_ANG:90");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    arcInProgress = false; // Allow other threads to send commands

    std::cout << "✅ Backward " << targetDegree << "° arc completed. Final yaw ≈ "
              << imu.yaw << "° (target " << targetYaw << "°)\n";
}


// int main() {
//     signal(SIGINT, ctrlc_handler);   // optional, in case user wants Ctrl+C during boot

//     // Reset flags
//     ctrl_c_pressed = false;
//     wallFollowStop = false;



//     // Run boot menu check
//     bootMenuCheck();

//     sendCommand("SERVO_ANG:104");

//     // At this stage, you can continue with full robot initialization
//     // Or exit if you only want boot menu for now
//     return 0;
// }



void arcRight() {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:145");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-70");
    std::this_thread::sleep_for(std::chrono::milliseconds(950));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:90");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}


void passGreenRedParking() {
    // Block other threads from interfering
    arcInProgress = true;

    
    // First pair of arcs
    arc90Back(-1, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arc90Back(1, 40);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    arcInProgress = false;  // allow other threads to send commands

    // Backward movement until LIDAR sees < 170 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    while (readLIDAR_median_cm(latest_points, 80, 100) > 170) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Variable arcs
    arcInProgress = true;
    arcVariableBack(1, 60, 50);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcVariableBack(-1, 70, 75);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;

    // Backward until LIDAR sees < 70 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    while (readLIDAR_median_cm(latest_points, 80, 100) > 70) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Final arc
    arc90Back(-1, 60);


}

void passRedGreenParking() {


    // Backward movement until LIDAR sees < 170 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Variable arcs
    // Block other threads from interfering

    arcInProgress = true;
    arcVariableBack(-1, 60, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcVariableBack(1, 60, 40);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;

    // Backward until LIDAR sees < 70 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    while (readLIDAR_median_cm(latest_points, 80, 100) > 25) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Final arc
    arc90Forward(-1, 50);


}





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


    // --- Wait for LIDAR ready ---
    while (latest_points.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(100));

    float Fi = readLIDAR_median_cm(latest_points, 80, 100);
    float Ri = readLIDAR_median_cm(latest_points, 10, 50);
    float Li = readLIDAR_median_cm(latest_points, 130, 170);

    std::cout << "Initial distances: Fi="<<Fi<<", Ri="<<Ri<<", Li="<<Li<<std::endl;
    sendCommand("SERVO_ANG:98");  // center steering
    std::this_thread::sleep_for(std::chrono::milliseconds(100));






    // --- Wait for valid LIDAR readings ---
        float currentRight = 0.0f;
        float currentLeft  = 0.0f;

        while ( currentLeft <= 0.0f || currentRight <= 0.0f ) {
            currentLeft  = readLIDAR_median_cm(latest_points, 140.0f, 160.0f);
            currentRight = readLIDAR_median_cm(latest_points, 10.0f, 50.0f); 


            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // --- Decide which wall to follow based on live readings ---
        int direction = (currentRight > currentLeft) ? -1 : 1;
        std::string wallSide = (direction == -1) ? "left" : "right";

        std::cout << "Following " << wallSide << " wall (Right=" 
                << currentRight << ", Left=" << currentLeft << ")\n";



    std::this_thread::sleep_for(std::chrono::milliseconds(200));


    for (int lap = 0; lap < 4; ++lap) {
        std::cout << "\n=== Lap " << (lap + 1) << " ===\n";

        passRedGreenParking();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }






                



    // ---- CONDITION -----





    std::cout << "All 4 laps completed.\n";

    ctrl_c_pressed = true;
    lidarThread.join();

    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
    close(sock);

    return 0;
}






// int main() {

//     signal(SIGINT, ctrlc_handler);

//     // Reset globals for safe multiple runs
//     ctrl_c_pressed = false;
//     wallFollowStop = false;

//     const char* ip = "127.0.0.1";
//     const int port = 5005;

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) { perror("socket"); return -1; }

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     addr.sin_addr.s_addr = inet_addr(ip);

//     if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind");
//         close(sock);
//         return -1;
//     }

//     std::cout << "Listening on " << ip << ":" << port << std::endl;

//     // Start LIDAR receiver thread
//     std::thread lidarThread(lidarReceiver, sock);

//         // --- Wait for LIDAR ready ---
//     while (latest_points.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(100));

//     float Fi = readLIDAR_median_cm(latest_points, 80, 100);
//     float Ri = readLIDAR_median_cm(latest_points, 10, 50);
//     float Li = readLIDAR_median_cm(latest_points, 130, 170);

//     std::cout << "Initial distances: Fi="<<Fi<<", Ri="<<Ri<<", Li="<<Li<<std::endl;
//     sendCommand("SERVO_ANG:90");  // center steering


    
//     float currentRight = 0.0f;
//     float currentLeft  = 0.0f;

//     while (currentRight <= 0.0f || currentLeft <= 0.0f) {
//         currentRight = readLIDAR_median_cm(latest_points, 170.0f, 190.0f);    // right sector
//         currentLeft  = readLIDAR_median_cm(latest_points, 350.0f, 10.0f);  // left sector

//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     }

//     // --- Decide which wall to follow based on live readings ---
//     int direction = (currentRight > currentLeft) ? -1 : 1;
//     std::string wallSide = (direction == -1) ? "left" : "right";

//     std::cout << "Following " << wallSide << " wall (Right=" 
//             << currentRight << ", Left=" << currentLeft << ")\n";



//     sendCommand("M_STOP");

//     // Move until close to wall
//     sendCommand("M_SPEED:-60");


//     // --- Direction decision ---
//     int direction = (Ri > Li) ? -1 : 1;
//     std::string wallSide = (direction == 1) ? "right" : "left";
//     std::cout << "Following " << wallSide << " wall\n";

//     for (int lap = 0; lap < 3; ++lap) {
//         std::cout << "\n=== Lap " << (lap + 1) << " ===\n";

//         sendCommand("SERVO_ANG:90");  // center steering



//         wallFollowStop = false;

//         // Start wall-following in a thread
//         std::thread wallFollowThread([]() {
//             followRightWallRearStableYaw(40.0f, -80, 1.0f, 0.0f, 0.5f, 10.0f);
//         });

//         // Wait for front distance <= threshold
//         bool distanceReached = false;
//         int consecutive = 0;
//         const int requiredConsecutive = 3;

//         while (!distanceReached && !ctrl_c_pressed) {
//             float dist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);

//             // Only consider reasonable distances
//             if (dist > 60.0f && dist <= 80.0f) {
//                 consecutive++;
//                 if (consecutive >= requiredConsecutive) {
//                     distanceReached = true;
//                     wallFollowStop = true;
//                     sendCommand("M_SPEED:0");
//                     sendCommand("M_STOP");
//                     std::cout << "[⚠️ WAIT LIDAR] Object detected at " << dist << " cm → STOP\n";
//                 }
//             } else {
//                 consecutive = 0; // reset if invalid or out of range
//             }

//             std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         }

//         // Stop wall-following thread cleanly
//         wallFollowThread.join();
//         sendCommand("SERVO_ANG:90");



//         // Optional short delay before next lap
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//         //sendCommand("SERVO_ANG:120");  // center steering



        
//         arc90Back(-1, 60);

//     }

//     std::cout << "Returning to starting front distance...\n";

//     arc90Back(-1, 60);

//     // Reset control flag
//     wallFollowStop = false;

//     // Start wall-following thread along the previously chosen wall side
//     std::thread wallFollowThread([direction]() {
//         followRightWallRearStableYaw(30.0f, -80, 1.0f, 0.0f, 0.5f, 10.0f);
//     });


//     const float ARC_RADIUS_CM = 25.0f;


//     const float rightThreshold = Ri + ARC_RADIUS_CM;
//     bool reachedRight = false;

//     while (!reachedRight && !ctrl_c_pressed) {
//         float frontDist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);

//         if (frontDist <= rightThreshold) {
//             reachedRight = true;
//             wallFollowStop = true;
//             sendCommand("M_STOP");
//             std::cout << "[⚠️ FINAL] Front distance reached: " << frontDist << " cm → STOP\n";
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     }

//     // Stop wall-following thread cleanly
//     wallFollowThread.join();

//     // Perform the final arc
//     arc90Back(-1, 60);

//     std::this_thread::sleep_for(std::chrono::milliseconds(1));

//     sendCommand("SERVO_ANG:90");
//     sendCommand("M_SPEED:-80");

//     const float frontThreshold = Fi;
//     bool reachedFront = false;


//     while (!reachedFront && !ctrl_c_pressed) {
//         float frontDist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);

//         if (frontDist <= frontThreshold) {
//             reachedFront = true;
//             wallFollowStop = true;
//             sendCommand("M_STOP");
//             std::cout << "[⚠️ FINAL] Front distance reached: " << frontDist << " cm → STOP\n";
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//     }

//     std::cout << "✅ Returned to starting position.\n";

    

    


// }





// int main() {
//     signal(SIGINT, ctrlc_handler);

//     // Reset globals for safe multiple runs
//     ctrl_c_pressed = false;
//     wallFollowStop = false;

//     const char* ip = "127.0.0.1";
//     const int port = 5005;

//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) { perror("socket"); return -1; }

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     addr.sin_addr.s_addr = inet_addr(ip);

//     if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind");
//         close(sock);
//         return -1;
//     }

//     std::cout << "Listening on " << ip << ":" << port << std::endl;

//     // Start LIDAR receiver thread
//     std::thread lidarThread(lidarReceiver, sock);

//     followRightWallRearStableYaw(30.0f, -60, 1.0f, 0.0f, 0.5f, 15.0f);

// }
