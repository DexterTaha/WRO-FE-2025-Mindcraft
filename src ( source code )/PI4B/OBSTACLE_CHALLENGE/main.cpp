// ===================== INCLUDES =====================
#include "camera_module.h"
#include <opencv2/opencv.hpp>

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
using namespace cv;

// ===================== GLOBAL FLAGS =====================
std::atomic<bool> wallFollowStop(false);  // new flag to control wall-following per lap
std::atomic<bool> arcInProgress(false);   // atomic for thread safety
std::atomic<bool> ctrl_c_pressed(false);

// ------------------- SIGNAL HANDLER -------------------
void ctrlc_handler(int) { ctrl_c_pressed = true; }

// ------------------- GLOBALS -------------------
struct LidarPoint {
    float angle;
    float distance;
    int quality;
};

// LIDAR data updated in background
std::vector<LidarPoint> latest_points;

// I2C configuration
const char* I2C_BUS = "/dev/i2c-1";  
const int ESP32_ADDR = 0x08;         
constexpr const char* GREEN = "\033[32m";
constexpr const char* RED   = "\033[31m";
constexpr const char* RESET = "\033[0m";

// PID constants for arc
float Kp = 2.0, Ki = 0.0, Kd = 0.1;

// ===================== CAMERA HSV CONFIG & DETECTION (YOUR ORIGINAL LOGIC) =====================

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



// HSV CONFIG STRUCT
struct HSVConfig {
    int hue_red = 5, sat_red = 200, val_red = 150;
    int hue_pink = 166, sat_pink = 201, val_pink = 63;
    int rangeH = 10, rangeS = 50, rangeV = 50;
};

// LOAD HSV CONFIG FROM FILE IF EXISTS
HSVConfig loadHSVConfig() {
    HSVConfig cfg;
    ifstream file("HSVConfig.txt");
    if(file.is_open()){
        string line;
        getline(file, line);
        int rH,rS,rV,pH,pS,pV;
        if(sscanf(line.c_str(), "HSVConfig cfg = {%d,%d,%d,%d,%d,%d};",
                  &rH,&rS,&rV,&pH,&pS,&pV) == 6)
        {
            cfg.hue_red = rH; cfg.sat_red = rS; cfg.val_red = rV;
            cfg.hue_pink = pH; cfg.sat_pink = pS; cfg.val_pink = pV;
        }
    }
    return cfg;
}




// DETECT RED / GREEN / PINK ORDERED LEFT → RIGHT (NO MARGIN FILTER)
vector<string> detectColorsOrdered(const Mat &frame, const HSVConfig &cfg) {

    Mat hsv; 
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    Mat mask_red, mask_pink, mask_green;

    auto makeMask = [&](int H, int S, int V) {
        int minH = H - cfg.rangeH; if(minH < 0) minH += 180;
        int maxH = H + cfg.rangeH; if(maxH > 180) maxH -= 180;
        int minS = max(0, S - cfg.rangeS);
        int maxS = min(255, S + cfg.rangeS);
        int minV = max(0, V - cfg.rangeV);
        int maxV = min(255, V + cfg.rangeV);

        Mat result, m1, m2;
        if(minH < maxH)
            inRange(hsv, Scalar(minH,minS,minV), Scalar(maxH,maxS,maxV), result);
        else {
            inRange(hsv, Scalar(0,minS,minV), Scalar(maxH,maxS,maxV), m1);
            inRange(hsv, Scalar(minH,minS,minV), Scalar(180,maxS,maxV), m2);
            bitwise_or(m1, m2, result);
        }
        return result;
    };

    mask_red = makeMask(cfg.hue_red, cfg.sat_red, cfg.val_red);
    mask_pink = makeMask(cfg.hue_pink, cfg.sat_pink, cfg.val_pink);

    // green simple range
    inRange(hsv, Scalar(35,150,50), Scalar(85,255,255), mask_green);

    // morphological cleanup
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5,5));
    morphologyEx(mask_red, mask_red, MORPH_CLOSE, kernel);
    morphologyEx(mask_pink, mask_pink, MORPH_CLOSE, kernel);
    morphologyEx(mask_green, mask_green, MORPH_CLOSE, kernel);

    struct Obj { string color; int x; };
    vector<Obj> detections;

    auto collect = [&](Mat &mask, string name) {
        vector<vector<Point>> cnts;
        findContours(mask, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        for(auto &c : cnts){
            if(contourArea(c) < 120) continue;
            Rect box = boundingRect(c);
            detections.push_back({name, box.x});
        }
    };

    collect(mask_red, "red");
    collect(mask_green, "green");
    collect(mask_pink, "pink");

    // sort left → right
    sort(detections.begin(), detections.end(),
         [](auto &a, auto &b){ return a.x < b.x; });

    vector<string> result;
    for(auto &d : detections) result.push_back(d.color);
    return result;
}

// DETECT RED / GREEN / PINK AND DRAW CONTOURS + LABELS (WITH marginBottom & BOTTOM HALF)
vector<string> detectColorsAndDraw(const Mat &bottom, Mat &display, const HSVConfig &cfg, int offsetY) {

    Mat hsv;
    cvtColor(bottom, hsv, COLOR_BGR2HSV);

    Mat mask_red, mask_pink, mask_green;

    auto makeMask = [&](int H, int S, int V) {
        int minH = H - cfg.rangeH; if(minH < 0) minH += 180;
        int maxH = H + cfg.rangeH; if(maxH > 180) maxH -= 180;
        int minS = max(0, S - cfg.rangeS);
        int maxS = min(255, S + cfg.rangeS);
        int minV = max(0, V - cfg.rangeV);
        int maxV = min(255, V + cfg.rangeV);

        Mat result, m1, m2;
        if (minH < maxH)
            inRange(hsv, Scalar(minH,minS,minV), Scalar(maxH,maxS,maxV), result);
        else {
            inRange(hsv, Scalar(0,minS,minV), Scalar(maxH,maxS,maxV), m1);
            inRange(hsv, Scalar(minH,minS,minV), Scalar(180,maxS,maxV), m2);
            bitwise_or(m1, m2, result);
        }
        return result;
    };

    mask_red  = makeMask(cfg.hue_red,  cfg.sat_red,  cfg.val_red);
    mask_pink = makeMask(cfg.hue_pink, cfg.sat_pink, cfg.val_pink);
    inRange(hsv, Scalar(35,150,50), Scalar(85,255,255), mask_green); // green

    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5,5));
    morphologyEx(mask_red,   mask_red,   MORPH_CLOSE, kernel);
    morphologyEx(mask_pink,  mask_pink,  MORPH_CLOSE, kernel);
    morphologyEx(mask_green, mask_green, MORPH_CLOSE, kernel);

    struct Obj { string color; int x; Rect box; };
    vector<Obj> objects;

    int bottomH = bottom.rows;
    int marginBottom = 80;   // <- your original margin to ignore floor

    auto collect = [&](Mat &mask, string name) {
        vector<vector<Point>> cnts;
        findContours(mask, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (auto &c : cnts) {
            if (contourArea(c) < 120) continue;

            Rect box = boundingRect(c);

            // POSITION FILTER: ignore stuff glued to the very bottom (floor)
            if (box.y + box.height > bottomH - marginBottom)
                continue;

            objects.push_back({name, box.x, box});
        }
    };

    collect(mask_red,  "red");
    collect(mask_green,"green");
    collect(mask_pink, "pink");

    // sort by left→right
    sort(objects.begin(), objects.end(),
         [](const Obj &a, const Obj &b){ return a.x < b.x; });

    // draw and build output list
    vector<string> result;
    for (auto &obj : objects) {
        Scalar col;
        if (obj.color == "red")   col = Scalar(0,0,255);
        if (obj.color == "green") col = Scalar(0,255,0);
        if (obj.color == "pink")  col = Scalar(203,192,255);

        Rect box = obj.box;
        box.y += offsetY;   // shift from ROI coords to full-image coords

        rectangle(display, box, col, 2);
        putText(display, obj.color,
                Point(box.x, box.y - 5),
                FONT_HERSHEY_SIMPLEX, 0.7, col, 2);

        result.push_back(obj.color);
    }

    return result;
}

vector<string> removeConsecutiveDuplicates(const vector<string> &in) {
    vector<string> out;
    if (in.empty()) return out;

    out.push_back(in[0]);
    for (size_t i = 1; i < in.size(); i++) {
        if (in[i] != in[i-1]) {
            out.push_back(in[i]);
        }
    }
    return out;
}


// ==================== ENCODER & IMU ====================
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






// ------------------- ARCS -------------------
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

void arc90Forward(int direction, int steerPercent = 120, float stopThresholdDeg = 35.0f) {
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


// Move the robot so that the BACK distance to an obstacle is ~= targetDistanceCm.
// If current back distance > target → go BACKWARD  (M_SPEED:-X)
// If current back distance < target → go FORWARD  (M_SPEED:+X)
//
// angle_min / angle_max: rear LIDAR sector (adjust for your setup)
void backToDistance(float targetDistanceCm,
                    float toleranceCm   = 2.0f,
                    int   baseSpeedPct  = 40,
                    float angle_min     = 260.0f,
                    float angle_max     = 280.0f)
{
    std::cout << "\n[📏 backToDistance] Target back distance = "
              << targetDistanceCm << " cm (±" << toleranceCm << " cm)\n";

    // Optional: keep wheels straight while doing this
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int lastDirection = 0; // -1 = backward, +1 = forward, 0 = stopped

    while (!ctrl_c_pressed) {
        float backDist = readLIDAR_median_cm(latest_points, angle_min, angle_max);

        if (backDist <= 0.0f) {
            // No valid reading → stop for safety and retry
            sendCommand("M_STOP");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        float diff = backDist - targetDistanceCm; 
        // diff > 0 → too far from wall (need to go backward)
        // diff < 0 → too close (need to go forward)

        std::cout << "[📡 BACK LIDAR] dist=" << backDist 
                  << " cm, diff=" << diff << " cm\n";

        if (std::fabs(diff) <= toleranceCm) {
            // We're close enough → stop and exit
            sendCommand("M_STOP");
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            std::cout << "✅ Reached target back distance ≈ "
                      << backDist << " cm\n";
            break;
        }

        int desiredDirection = (diff > 0.0f) ? -1 : +1;
        // diff > 0 → too far → go BACKWARD  (direction = -1 → M_SPEED:-base)
        // diff < 0 → too close → go FORWARD (direction = +1 → M_SPEED:+base)

        if (desiredDirection != lastDirection) {
            int speedCmd = desiredDirection * baseSpeedPct;
            // Keep your original behavior (note the minus here):
            sendCommand("M_SPEED:" + std::to_string(-speedCmd));
            lastDirection = desiredDirection;

            std::cout << "➡ Moving "
                      << (desiredDirection < 0 ? "BACKWARD" : "FORWARD")
                      << " at " << speedCmd << "%\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Safety stop if loop exits
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}






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
    arcVariableBack(-1, 60, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcVariableBack(1, 60, 40);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;

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
    arcInProgress = true;

    // Final arc
    arc90Back(-1, 60);
    arcInProgress = false;

    backToDistance(50);


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
    arcInProgress = true;
    arc90Forward(-1, 50);
    arcInProgress = false;

    backToDistance(50);


}


void passRed() {

    sendCommand("SERVO_ANG:60");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:120");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));





    std::thread wallFollowThread;  // reusable thread variable

    if (wallFollowThread.joinable()) wallFollowThread.join(); // join any previous thread
            wallFollowThread = std::thread([&]() {
                while (!wallFollowStop && !ctrl_c_pressed) {
                    followRightWallRearStableYaw(30.0f, -60, 1.031f, 0.0f, 0.5f, 16.0f);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
    });

    wallFollowStop = false;
    arcInProgress = false;

    // Reset consecutive counter and front distance tracking
    bool distanceReached = false;
    int consecutive = 0;
    const int requiredConsecutive = 3;

    while (!distanceReached && !ctrl_c_pressed) {
                float dist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);
                if (dist > 70.0f && dist <= 80.0f) {
                    consecutive++;
                    if (consecutive >= requiredConsecutive) {
                        distanceReached = true;
                        wallFollowStop = true;
                        if (wallFollowThread.joinable()) wallFollowThread.join();
                        sendCommand("M_STOP");
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                        sendCommand("SERVO_ANG:90");
                        std::cout << "[⚠️ WAIT LIDAR] Object detected at "
                                << dist << " cm → STOP after arc\n";
                    }
                } else consecutive = 0;

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    arcInProgress = true;

    arc90Back(-1, 60);

    arcInProgress = false;

    backToDistance(50);


}



void passGreen() {



    arcInProgress = true;
    arcVariableBack(-1, 60, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcVariableBack(1, 60, 40);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;

    // Backward movement until LIDAR sees < 170 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    while (readLIDAR_median_cm(latest_points, 80, 100) > 25) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    arcInProgress = true;


    arc90Forward(-1, 50);

    arcInProgress = false;
    backToDistance(50);


}




void passRedGreen() {

    sendCommand("SERVO_ANG:60");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:120");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));


    // Backward movement until LIDAR sees < 170 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Variable arcs
    // Block other threads from interfering

    arcInProgress = true;
    arc90Back(-1, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arc90Back(1, 40);
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
    arcInProgress = true;

    arc90Forward(-1, 50);
    arcInProgress = false;

    backToDistance(50);


}




void passGreenRed() {
    // Block other threads from interfering
    arcInProgress = true;
    arcVariableBack(-1, 60, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcVariableBack(1, 60, 40);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;


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
    arc90Back(1, 30);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    arc90Back(-1, 60);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    arcInProgress = false;

    // Backward until LIDAR sees < 70 cm
    sendCommand("SERVO_ANG:98");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    while (readLIDAR_median_cm(latest_points, 80, 100) > 80) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Final arc
    arcInProgress = true;

    arc90Back(-1, 60);
    arcInProgress = false;

    backToDistance(50);


}



// ===================== COLOR PATTERN LOGIC =====================

enum PatternType {
    PATTERN_NONE = 0,
    PATTERN_RED_GREEN,
    PATTERN_RED_GREEN_PARKING
};

// Use EXACT same logic as your camera main: bottom half ROI + detectColorsAndDraw + marginBottom
PatternType detectPatternFromFrame(const Mat &frame, const HSVConfig &cfg) {
    if (frame.empty()) return PATTERN_NONE;

    int H = frame.rows;
    Rect roi(0, H/2, frame.cols, H/2);
    Mat bottom = frame(roi).clone();

    // We still create display Mat, but we don't show it (no imshow)
    Mat display = Mat::zeros(frame.size(), frame.type());
    bottom.copyTo(display(roi));

    vector<string> colors = detectColorsAndDraw(bottom, display, cfg, H/2);

    // remove repeated same-color detections
    colors = removeConsecutiveDuplicates(colors);


    std::cout << "Detected colors (L->R): ";
    for (auto &c : colors) std::cout << c << " ";
    std::cout << std::endl;

    // EXACT requirement:
    // if red then green (passRedGreen)
    // if red green and parking (== pink) (passRedGreenParking)

    if (colors.size() >= 2 && colors[0] == "red" && colors[1] == "green") {
        if (colors.size() >= 3 && colors[2] == "pink") {
            return PATTERN_RED_GREEN_PARKING;
        }
        return PATTERN_RED_GREEN;
    }

    return PATTERN_NONE;
}

// Capture one frame from CameraModule and decide pattern
PatternType detectPatternOnce(CameraModule &camera, const HSVConfig &cfg) {
    TimedFrame timed;
    if (!camera.waitForFrame(timed)) {
        std::cerr << "❌ Failed to grab frame from camera\n";
        return PATTERN_NONE;
    }
    return detectPatternFromFrame(timed.frame, cfg);
}



// ===================== MAIN =====================
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

    // ---- CAMERA INIT (same module as your camera code) ----
    CameraModule camera([](lccv::PiCamera &){});
    if(!camera.start()){
        cerr << "Failed to start camera\n";
        ctrl_c_pressed = true;
        if (lidarThread.joinable()) lidarThread.join();
        close(sock);
        return -1;
    }

    HSVConfig hsvCfg = loadHSVConfig();

    // --- Wait for LIDAR ready ---
    while (latest_points.empty() && !ctrl_c_pressed)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    float Fi = readLIDAR_median_cm(latest_points, 80, 100);
    float Ri = readLIDAR_median_cm(latest_points, 10, 50);
    float Li = readLIDAR_median_cm(latest_points, 130, 170);

    std::cout << "Initial distances: Fi="<<Fi<<", Ri="<<Ri<<", Li="<<Li<<std::endl;
    sendCommand("SERVO_ANG:98");  // center steering
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // --- Optional: decide wall side as before ---
    float currentRight = 0.0f;
    float currentLeft  = 0.0f;

    while ( currentLeft <= 0.0f || currentRight <= 0.0f ) {
        currentLeft  = readLIDAR_median_cm(latest_points, 140.0f, 160.0f);
        currentRight = readLIDAR_median_cm(latest_points, 10.0f, 50.0f); 
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    int direction = (currentRight > currentLeft) ? -1 : 1;
    std::string wallSide = (direction == -1) ? "left" : "right";

    std::cout << "Following " << wallSide << " wall (Right=" 
            << currentRight << ", Left=" << currentLeft << ")\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // -------- SNAPSHOT COLOR DECISION --------
    PatternType pattern = detectPatternOnce(camera, hsvCfg);

    switch (pattern) {
        case PATTERN_RED_GREEN_PARKING:
            std::cout << "Pattern RED-GREEN-PINK(PARKING) detected → passRedGreenParking()\n";
            passRedGreenParking();
            break;

        case PATTERN_RED_GREEN:
            std::cout << "Pattern RED-GREEN detected → passRedGreen()\n";
            passRedGreen();
            break;

        default:
            std::cout << "No matching pattern → default = passGreen()\n";
            passGreen();
            break;
    }

    std::cout << "Maneuver completed.\n";

    // ---- CLEANUP ----
    ctrl_c_pressed = true;

    camera.stop();
    if (lidarThread.joinable()) lidarThread.join();

    sendCommand("M_STOP");
    sendCommand("SERVO_ANG:90");
    close(sock);

    return 0;
}
