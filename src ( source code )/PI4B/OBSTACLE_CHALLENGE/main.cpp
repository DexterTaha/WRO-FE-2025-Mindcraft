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



void arc90Back(int direction, int steerPercent = 120, float stopThresholdDeg = 25.0f) {
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



// ------------------- WAIT UNTIL GYRO TARGET -------------------
/**
 * Wait until the IMU yaw reaches a target angle within a tolerance.
 * @param targetYaw Target yaw in degrees [0-360)
 * @param tolerance Allowed error in degrees (default 1.0°)
 * @param maxTime Maximum wait time in seconds (optional safety)
 * @param useRearSteering Whether to invert yaw correction for rear steering
 */
void waitUntilYaw(float targetYaw, float tolerance = 1.0f, float maxTime = 5.0f, bool useRearSteering = false) {
    auto startTime = high_resolution_clock::now();

    while (!ctrl_c_pressed) {
        IMUData imu = readIMUData();
        float currentYaw = imu.yaw;

        // shortest angular distance
        float diff = targetYaw - currentYaw;
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        // apply rear steering inversion if needed
        if (useRearSteering) diff = -diff;

        if (fabs(diff) <= tolerance) {
            break;  // target reached
        }

        // Safety: break if taking too long
        auto elapsed = duration<float>(high_resolution_clock::now() - startTime).count();
        if (elapsed >= maxTime) {
            std::cout << "[⚠️ WAIT YAW] Timeout reached! Current yaw: " << currentYaw << ", target: " << targetYaw << "\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
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




void followLeftWallRearStableYaw(float targetDistanceCm = 30.0f,
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

    float leftDist = readLIDAR_median_cm(latest_points, 100.0f, 170.0f);
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





void testMotorsAndServo(int servoStep = 30, int motorStep = 10, int delayMs = 500) {
  cout << "=== Testing Servo Angles ===\n";
  for (int angle = 0; angle <= 180 && !ctrl_c_pressed; angle += servoStep) {
    sendCommand("SERVO_ANG:" + to_string(angle));
    this_thread::sleep_for(milliseconds(delayMs));
  }

  // Center servo
  sendCommand("SERVO_ANG:90");
  this_thread::sleep_for(milliseconds(delayMs));

  cout << "=== Testing Motor Speeds ===\n";
  for (int speed = 0; speed <= 100 && !ctrl_c_pressed; speed += motorStep) {
    sendCommand("M_SPEED:" + to_string(speed));
    this_thread::sleep_for(milliseconds(delayMs));
  }

  // Stop motor
  sendCommand("M_SPEED:0");

  cout << "Test completed.\n";
}


// int main() {
//   signal(SIGINT, ctrlc_handler);
//   ctrl_c_pressed = false;
//   sendCommand("SERVO_ANG:90");
//   sendCommand("M_STOP");
//   return 0;
// }



void straightTurnStraight(float straight1_cm, float straight2_cm,
                          float startAngle, float endAngle,
                          float speed, int steps = 50)
{
    // --- Phase 1: initial straight ---
    sendCommand("SERVO_ANG:" + std::to_string(startAngle));
    sendCommand("M_SPEED:" + std::to_string(speed));
    std::this_thread::sleep_for(std::chrono::milliseconds(int(straight1_cm * 10))); // adjust factor to match your robot
    sendCommand("M_STOP");
    sleep(1);

    // --- Phase 2: smooth cubic turn ---
    for (int i = 0; i <= steps; i++) {
        float t = float(i) / steps;        // 0 → 1
        float steering = startAngle + (endAngle - startAngle) * t * t * t;  // cubic
        sendCommand("SERVO_ANG:" + std::to_string(int(steering)));
        sendCommand("M_SPEED:" + std::to_string(speed));
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // small delay between steps
    }
    sendCommand("M_STOP");
    sleep(1);

    // --- Phase 3: final straight ---
    sendCommand("SERVO_ANG:" + std::to_string(endAngle));
    sendCommand("M_SPEED:" + std::to_string(speed));
    std::this_thread::sleep_for(std::chrono::milliseconds(int(straight2_cm * 10))); // adjust factor
    sendCommand("M_STOP");
}




void passGreenRed() {
    sendCommand("SERVO_ANG:60");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:130");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:65");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:-60");
    std::this_thread::sleep_for(std::chrono::milliseconds(2645));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:90");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:60");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));


}



void alignToEqualDistancesPID(float targetLeft, float targetBack) {
    const float tolerance = 3.0f;          // allowed difference in cm
    const int requiredConsecutive = 3;     // stable readings to stop
    int consecutive = 0;

    float leftDist = 0.0f;
    float backDist = 0.0f;

    // PID constants
    const float Kp = 1.0f;
    const float Ki = 0.0f;   // can be tuned later
    const float Kd = 0.3f;

    float integral = 0.0f;
    float lastError = 0.0f;

    auto millisNow = [](){ return std::chrono::steady_clock::now(); };
    auto start = millisNow();
    const int TIMEOUT_MS = 10000;  // 10s safety timeout

    // Start moving backward
    sendCommand("M_SPEED:35");   // negative for backward

    while (!ctrl_c_pressed) {
        // Read LIDAR distances
        leftDist = readLIDAR_median_cm(latest_points, 100.0f, 170.0f);   // LEFT
        backDist = readLIDAR_median_cm(latest_points, 260.0f, 280.0f);  // BACK

        // Ignore invalid/outlier readings
        if (leftDist <= 0 || leftDist > 150 || backDist <= 0 || backDist > 150) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // PID error calculation
        float error = (leftDist - targetLeft) - (backDist - targetBack);
        integral += error * 0.05f;  // loop dt = 50ms
        float derivative = (error - lastError) / 0.05f;
        lastError = error;

        float correction = Kp * error + Ki * integral + Kd * derivative;

        // Map correction to servo angle
        int servoAngle = 90 + static_cast<int>(correction);
        if (servoAngle > 120) servoAngle = 120;
        if (servoAngle < 60)  servoAngle = 60;
        sendCommand("SERVO_ANG:" + std::to_string(servoAngle));

        // Debug print
        cout << "Left=" << leftDist << " Back=" << backDist 
             << " Error=" << error << " Servo=" << servoAngle << endl;

        // Stop condition
        if (fabs(error) <= tolerance) {
            consecutive++;
            if (consecutive >= requiredConsecutive) break;
        } else {
            consecutive = 0;
        }

        // Safety timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            millisNow() - start).count();
        if (elapsed > TIMEOUT_MS) {
            cout << "alignToEqualDistancesPID() TIMEOUT!\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Ensure robot stops
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("SERVO_ANG:90");

    cout << "Alignment complete: Left=" << leftDist 
         << " Back=" << backDist << " cm\n";
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
    std::thread wallFollowThread;

    alignToEqualDistancesPID(60.0f, 60.0f);

    // bool distanceReached = false;
    // int consecutive = 0;
    // const int requiredConsecutive = 3;

    // sendCommand("SERVO_ANG:70");
    // sleep(0.5);
    // sendCommand("M_SPEED:-40");
    // std::this_thread::sleep_for(std::chrono::milliseconds(1600));
    // sendCommand("M_STOP");
    // sleep(0.5);
    // sendCommand("SERVO_ANG:140");
    // sleep(0.5);
    // sendCommand("M_SPEED:-40");
    // std::this_thread::sleep_for(std::chrono::milliseconds(1300));
    // sendCommand("M_STOP");
    // sleep(0.5);
    // sendCommand("SERVO_ANG:97");
    // sleep(0.5);

    // wallFollowStop = false;
    // wallFollowThread = std::thread([&]() {
    //     followRightWallRearStableYaw(15.0f, -50, 1.5f, 0.0f, 0.5f, 15.0f);
    // });

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // --- Wait for front distance to reach threshold during wall-follow ---

    // Global safety flag (place this at the top of your file)
    



    // // --- Clean up wall-follow thread ---
    // if (wallFollowThread.joinable()) wallFollowThread.join();
    // sleep(1);
    // sendCommand("SERVO_ANG:90");  // reset steering
    // std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // float distance = readLIDAR_median_cm(latest_points, 10, 50);
    // cout << "Initial rear distance: " << distance << " cm\n";


    // float currentBack = 0.0f;

    // // // Wait until the first valid distance
    // // while (currentBack <= 0.0f && !ctrl_c_pressed) {
    // //     currentBack = readLIDAR_median_cm(latest_points, 260.0f, 280.0f);
    // //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // // }

    // // cout << "Initial distances: Back=" << currentBack << " cm\n";

    // // Start moving backward
    // sendCommand("M_SPEED:40");

    // // Keep updating distance until <= 65 cm
    // while (!ctrl_c_pressed) {
    //     currentBack = readLIDAR_median_cm(latest_points, 260.0f, 280.0f);

    //     if (currentBack <= 58.0f && currentBack > 0.0f) {
    //         break;  // stop condition reached
    //     }

    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // }

    // sendCommand("M_STOP");


    

    //passGreenRed();






    // wallFollowStop = false;
    // wallFollowThread = std::thread([&]() {
    //     followLeftWallRearStableYaw(20.0f, -40, 1.5f, 0.0f, 0.5f, 15.0f);
    // });

    // // --- Wait for front distance to reach threshold during wall-follow ---
    // bool distanceReached = false;
    // int consecutive = 0;
    // const int requiredConsecutive = 3;

    // while (!distanceReached && !ctrl_c_pressed) {
    //     float dist = readLIDAR_median_cm(latest_points, 80.0f, 100.0f);

    //     if (dist > 70.0f && dist <= 90.0f) {
    //         consecutive++;
    //         if (consecutive >= requiredConsecutive) {
    //             distanceReached = true;
    //             wallFollowStop = true;
    //             sendCommand("M_STOP");
    //             std::cout << "[⚠️ WAIT LIDAR] Object detected at "
    //                     << dist << " cm → STOP after arc\n";
    //         }
    //     } else {
    //         consecutive = 0;
    //     }

    //     std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // }

    // // --- Clean up wall-follow thread ---
    // if (wallFollowThread.joinable()) wallFollowThread.join();
    // sendCommand("SERVO_ANG:90");  // reset steering
    // std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // sleep(1);



    // arc90Back(1, 80);  // blocking, completes fully
    // std::this_thread::sleep_for(std::chrono::milliseconds(200)); // cooldown



    

    //passGreenRed();

    

    





    
    // wallFollowStop = false;
    // wallFollowThread = std::thread([&]() {
    //     followRightWallRearStableYaw(30.0f, -40, 1.5f, 0.0f, 0.5f, 15.0f);
    // });

//
//
//     // --- Wait for LIDAR ready ---
//     while (latest_points.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//     sendCommand("SERVO_ANG:90");
//
//
//
    // --- Wait for valid LIDAR readings ---
        



//         // --- Decide which wall to follow based on live readings ---
//         int direction = (currentRight > currentLeft) ? -1 : 1;
//         std::string wallSide = (direction == -1) ? "left" : "right";

//         std::cout << "Following " << wallSide << " wall (Right=" 
//                 << currentRight << ", Left=" << currentLeft << ")\n";
// //
// //
//
//
//     float Fi = readLIDAR_median_cm(latest_points, 80, 100);
//     float Ri = readLIDAR_median_cm(latest_points, 10, 50);
//     float Li = readLIDAR_median_cm(latest_points, 130, 170);
//
//     std::cout << "Initial distances: Fi="<<Fi<<", Ri="<<Ri<<", Li="<<Li<<std::endl;
//
//
//
//
//
//
//
//     std::cout << "Done.\n";
//
//     // Ensure LIDAR thread exits
//     ctrl_c_pressed = true;
//     lidarThread.join();
//
//     sendCommand("SERVO_ANG:140");
//     sendCommand("M_SPEED:-50");  // start moving backward
//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     sendCommand("M_STOP");
//
//
//
//     // while (readLIDAR_median_cm(latest_points, 80, 100) > 8) std::this_thread::sleep_for(std::chrono::milliseconds(50));
//
//
//     sendCommand("SERVO_ANG:30");
//     sendCommand("M_SPEED:80");
//     std::this_thread::sleep_for(std::chrono::milliseconds(1800));
//     sendCommand("M_STOP");
//
//     sendCommand("SERVO_ANG:130");
//     sendCommand("M_SPEED:100");
//     std::this_thread::sleep_for(std::chrono::milliseconds(2000));
//     sendCommand("M_STOP");
//
//

    close(sock);

    return 0;
}
