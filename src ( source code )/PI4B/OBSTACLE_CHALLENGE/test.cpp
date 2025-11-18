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

using namespace cv;
using namespace std;
using namespace std::chrono;

// ------------------- GLOBAL STRUCTS (Unchanged) -------------------
struct LidarPoint {
    float angle;
    float distance;
    int quality;
};

struct IMUData {
    float roll, pitch, yaw, delta;
    string rotation_dir;
    int total_rot;
    float lin_x, lin_y;
};

struct Pillar {
    Point center;
    float distance; // bigger = nearer (lower in image)
    Scalar color;   // BGR
};

// ------------------- GLOBALS (Unchanged) -------------------
volatile sig_atomic_t stop_flag = 0;
std::atomic<bool> wallFollowStop(false);
std::vector<LidarPoint> latest_points;
std::atomic<bool> ctrl_c_pressed(false);
std::atomic<bool> arcInProgress(false);
std::atomic<bool> softStop(false);


const char* I2C_BUS = "/dev/i2c-1";
const int ESP32_ADDR = 0x08;
float Kp = 2.0, Ki = 0.0, Kd = 0.1; // General straight line PID for Yaw stabilization

// ------------------- FUNCTIONS (Unchanged) -------------------
void signalHandler(int signum) {
    cout << "\nInterrupt signal (" << signum << ") received.\n";
    stop_flag = 1;
}

void ctrlc_handler(int) { ctrl_c_pressed = true; }

bool sendCommand(const string& cmd) {
    int file = open(I2C_BUS, O_RDWR);
    if(file < 0){ cerr << "❌ Failed to open I2C bus\n"; return false; }
    if(ioctl(file, I2C_SLAVE, ESP32_ADDR) < 0){ cerr << "❌ Failed to connect to I2C\n"; close(file); return false; }
    string fullCmd = cmd + "\n";
    if(write(file, fullCmd.c_str(), fullCmd.length()) != (ssize_t)fullCmd.length()){ cerr << "❌ Failed to write to I2C\n"; close(file); return false; }
    close(file);
    return true; 
}

long readEncoder() {
    int file = open(I2C_BUS, O_RDWR);
    if(file < 0) return -1;
    if(ioctl(file, I2C_SLAVE, ESP32_ADDR) < 0){ close(file); return -1; }
    char buf[32] = {0};
    if(read(file, buf, sizeof(buf)-1) < 0){ close(file); return -1; }
    close(file);
    string s(buf);
    size_t pos = s.find("ENC:");
    if(pos != string::npos) return stol(s.substr(pos+4));
    return -1;
}

IMUData readIMUData(const string& path="/tmp/bno_imu.txt") {
    IMUData imu{};
    ifstream f(path);
    if(f.is_open()){
        string line;
        if(getline(f, line)){
            stringstream ss(line);
            string token;
            if(getline(ss, token, ',')) imu.roll = stof(token);
            if(getline(ss, token, ',')) imu.pitch = stof(token);
            if(getline(ss, token, ',')) imu.yaw = stof(token);
            if(getline(ss, token, ',')) imu.delta = stof(token);
            if(getline(ss, token, ',')) imu.rotation_dir = token;
            if(getline(ss, token, ',')) imu.total_rot = stoi(token);
            if(getline(ss, token, ',')) imu.lin_x = stof(token);
            if(getline(ss, token, ',')) imu.lin_y = stof(token);
        }
    }
    return imu;
}

float getYaw() {
    IMUData imu = readIMUData();
    return imu.yaw;
}

void moveStraight(float targetYaw, float speed=-50.0f) {
    float currentYaw = getYaw();
    float error = targetYaw - currentYaw;
    while(error > 180.0f) error -= 360.0f;
    while(error < -180.0f) error += 360.0f;

    float correction = Kp * error;
    float servoAngle = 90 + correction;
    servoAngle = std::clamp(servoAngle, 60.0f, 120.0f);

    sendCommand("SERVO_ANG:" + to_string(int(servoAngle)));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    sendCommand("M_SPEED:" + to_string(int(speed)));
}

// ------------------- LIDAR UTILS (Unchanged) -------------------
static float normalizeAngle(float a){
    while(a < 0.0f) a += 360.0f;
    while(a >= 360.0f) a -= 360.0f;
    return a;
}

static float medianOf(vector<float>& v){
    sort(v.begin(), v.end());
    size_t n = v.size();
    if(n % 2) return v[n/2];
    return (v[n/2-1] + v[n/2]) * 0.5f;
}

float readLIDAR_median_cm(const vector<LidarPoint>& points, float angle_min, float angle_max, int quality_threshold=10, float max_valid_mm=60000.0f){
    vector<float> distances_mm;
    angle_min = normalizeAngle(angle_min);
    angle_max = normalizeAngle(angle_max);

    auto in_interval = [&](float a) -> bool{
        a = normalizeAngle(a);
        if(angle_min <= angle_max) return (a >= angle_min && a <= angle_max);
        return (a >= angle_min || a <= angle_max);
    };

    for(const auto& p : points){
        if(p.distance <= 0.0f) continue;
        if(p.quality < quality_threshold) continue;
        if(p.distance > max_valid_mm) continue;
        if(in_interval(p.angle)) distances_mm.push_back(p.distance);
    }

    if(distances_mm.empty()) return -1.0f;
    return medianOf(distances_mm)/10.0f; // mm -> cm
}


float readLIDAR_nearest_cm(const vector<LidarPoint>& points, float angle_min, float angle_max, int quality_threshold=10, float max_valid_mm=60000.0f){
    float nearest_mm = max_valid_mm;
    angle_min = normalizeAngle(angle_min);
    angle_max = normalizeAngle(angle_max);

    auto in_interval = [&](float a) -> bool{
        a = normalizeAngle(a);
        if(angle_min <= angle_max) return (a >= angle_min && a <= angle_max);
        return (a >= angle_min || a <= angle_max);
    };

    for(const auto& p : points){
        if(p.distance <= 0.0f) continue;
        if(p.quality < quality_threshold) continue;
        if(p.distance > max_valid_mm) continue;
        if(in_interval(p.angle)){
            if(p.distance < nearest_mm) nearest_mm = p.distance;
        }
    }

    if(nearest_mm == max_valid_mm) return -1.0f; // no valid points
    return nearest_mm / 10.0f; // mm -> cm
}


void arc90Back(int direction, int steerPercent = 120, float stopThresholdDeg = 25.0f) {
    arcInProgress = true;  
    // ... (arc90Back logic remains unchanged) ...
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

    arcInProgress = false; 

    std::cout << "✅ Backward 90° arc completed. Final yaw ≈ "
              << imu.yaw << "° (target " << targetYaw << "°)\n";
}


// ------------------- LIDAR RECEIVER THREAD (Unchanged) -------------------
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


int main() {
    signal(SIGINT, signalHandler);
    ctrl_c_pressed = false;
    wallFollowStop = false;

    
    // --- UDP LIDAR setup ---
    const char* ip = "127.0.0.1";
    const int port = 5005;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return -1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return -1;
    }

    std::thread lidarThread(lidarReceiver, sock);

    // --- Camera setup ---
    CameraModule camera([](lccv::PiCamera &){});
    if(!camera.start()){ cerr << "Failed to start camera\n"; return -1; }

    Mat frame;
    namedWindow("Camera Feed", WINDOW_AUTOSIZE);

    // --- PID / state variables ---
    enum State {SEARCH, FOLLOW_OBJECT, AVOID_OBJECT, AFTER_AVOID};
    State currentState = SEARCH;
    long avoidanceStartEncoder = 0;
    bool isAvoidingLocked = false;
    Scalar lastPillarColor = Scalar(0,0,0);

    static float integral_horiz = 0.0f;
    static float lastError_horiz = 0.0f;
    auto lastTime = high_resolution_clock::now();

    const float TARGET_FOLLOW_DIST_CM = 50.0f;
    const float WALL_TARGET_DIST_CM = 30.0f;
    const float WALL_MIN_DIST_CM = 20.0f;
    const float COLOR_STEER_OFFSET_MAX = 25.0f;
    const long AVOIDANCE_MOVE_DISTANCE_TICKS = 1500;
    const float Kp_dist = 3.0f;
    const float Kp_horiz_max = 0.35f;
    const float Kp_horiz_min = 0.05f;
    const float Ki_horiz = 0.001f;
    const float Kd_horiz = 0.05f;
    const float WALL_Kp = 3.0f;
    const float WALL_MAX_CORRECTION = 15.0f;
    const float MAX_FORWARD_SPEED = -50.0f;
    const float MIN_FORWARD_SPEED = -10.0f;

    int frameWidth = 640; // default, will update per frame
    int frameCenterX = frameWidth/2;

    auto shutdownRobot = [](){
        for(int i=0;i<3;i++){
            sendCommand("M_STOP");
            sendCommand("SERVO_ANG:90");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    while(!stop_flag){
        TimedFrame timed;
        if(!camera.waitForFrame(timed)){ cerr << "Failed to get frame\n"; break; }
        frame = timed.frame;
        frameWidth = frame.cols;
        frameCenterX = frameWidth/2;

        // --- LIDAR ---
        float lidarFront = readLIDAR_nearest_cm(latest_points, 80, 100);
        float lidarRight = readLIDAR_median_cm(latest_points, 10, 50);

        // --- WALL ---
        float wallCorrection = 0.0f;
        bool wallDanger = false;
        if(lidarRight > 0){
            if(lidarRight < WALL_MIN_DIST_CM){ wallCorrection=-WALL_MAX_CORRECTION*2.5f; wallDanger=true; }
            else if(lidarRight < WALL_TARGET_DIST_CM*2.0f){ wallCorrection=-WALL_Kp*(WALL_TARGET_DIST_CM - lidarRight); }
        }
        wallCorrection = std::clamp(wallCorrection, -WALL_MAX_CORRECTION, WALL_MAX_CORRECTION);

        // --- Camera ---
        Mat hsv; cvtColor(frame, hsv, COLOR_BGR2HSV);
        Mat maskRed1, maskRed2, maskRed, maskGreen;
        inRange(hsv, Scalar(0,150,50), Scalar(10,255,255), maskRed1);
        inRange(hsv, Scalar(160,150,50), Scalar(180,255,255), maskRed2);
        bitwise_or(maskRed1, maskRed2, maskRed);
        inRange(hsv, Scalar(35,150,50), Scalar(85,255,255), maskGreen);

        vector<Pillar> pillars;
        auto processMask = [&](Mat &mask, Scalar color){
            Mat labels, stats, centroids;
            int n = connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);
            for(int i=1;i<n;i++){
                int area=stats.at<int>(i,CC_STAT_AREA);
                int w=stats.at<int>(i,CC_STAT_WIDTH);
                int h=stats.at<int>(i,CC_STAT_HEIGHT);
                float ar=float(w)/h;
                if(area<100 || ar<0.3f || ar>0.8f) continue;
                Point center(stats.at<int>(i,CC_STAT_LEFT)+w/2, stats.at<int>(i,CC_STAT_TOP)+h/2);
                float dist=frame.rows - center.y;
                pillars.push_back({center, dist, color});
            }
        };
        processMask(maskRed, Scalar(0,0,255));
        processMask(maskGreen, Scalar(0,255,0));

        Pillar nearestPillar{};
        bool hasPillar = !pillars.empty();
        if(hasPillar){
            auto it = max_element(pillars.begin(), pillars.end(), [](const Pillar &a, const Pillar &b){ return a.distance < b.distance;});
            nearestPillar = *it;
            lastPillarColor = nearestPillar.color;
        }

        auto now = high_resolution_clock::now();
        float dt = duration<float>(now - lastTime).count(); lastTime=now; if(dt==0) dt=0.001f;

        switch(currentState){
            case SEARCH:{
                if(hasPillar && lidarFront>TARGET_FOLLOW_DIST_CM) currentState=FOLLOW_OBJECT;
                else moveStraight(getYaw(), -20.0f);
                break;
            }
            case FOLLOW_OBJECT:{
                if(!hasPillar){ currentState=SEARCH; break; }
                if(lidarFront>0 && lidarFront<=TARGET_FOLLOW_DIST_CM){ 
                    isAvoidingLocked=true; avoidanceStartEncoder=readEncoder(); 
                    currentState=AVOID_OBJECT; break;
                }
                // --- PID follow ---
                float horizontalError=nearestPillar.center.x-frameCenterX;
                float factor=pow(abs(horizontalError)/(frameWidth/2.0f),2.0f);
                float Kp_cur=Kp_horiz_min+(Kp_horiz_max-Kp_horiz_min)*factor;
                integral_horiz+=horizontalError*dt;
                float derivative=(horizontalError-lastError_horiz)/dt;
                lastError_horiz=horizontalError;
                float objSteer=Kp_cur*horizontalError+Ki_horiz*integral_horiz+Kd_horiz*derivative;
                float colorBias=(nearestPillar.color==Scalar(0,0,255)?COLOR_STEER_OFFSET_MAX:-COLOR_STEER_OFFSET_MAX);
                float fade=std::min(1.0f,std::max(0.0f,(TARGET_FOLLOW_DIST_CM*2.0f-lidarFront)/(TARGET_FOLLOW_DIST_CM*2.0f)));
                colorBias*=fade;
                float finalSteer=std::clamp(objSteer+colorBias+wallCorrection,-40.0f,40.0f);
                int servoAng=std::clamp(int(90-finalSteer),60,120);
                sendCommand("SERVO_ANG:"+to_string(servoAng));

                float linearSpeed = MIN_FORWARD_SPEED;
                if(lidarFront>0) linearSpeed=std::clamp(-Kp_dist*(lidarFront-TARGET_FOLLOW_DIST_CM),MAX_FORWARD_SPEED,MIN_FORWARD_SPEED);
                if(wallDanger) linearSpeed=MIN_FORWARD_SPEED;
                sendCommand("M_SPEED:"+to_string(int(linearSpeed)));
                break;
            }
            case AVOID_OBJECT:{
                float steerBias=(nearestPillar.color==Scalar(0,0,255)?2.0f*COLOR_STEER_OFFSET_MAX:-2.0f*COLOR_STEER_OFFSET_MAX);
                float finalSteer=std::clamp(steerBias+wallCorrection,-40.0f,40.0f);
                int servoAng=std::clamp(int(90-finalSteer),60,120);
                sendCommand("SERVO_ANG:"+to_string(servoAng));
                sendCommand("M_SPEED:"+to_string(int(MIN_FORWARD_SPEED)));

                if(abs(readEncoder()-avoidanceStartEncoder)>=AVOIDANCE_MOVE_DISTANCE_TICKS){
                    isAvoidingLocked=false;
                    avoidanceStartEncoder=0;
                    currentState=AFTER_AVOID;
                }
                break;
            }
            case AFTER_AVOID:{
                moveStraight(getYaw(), -30.0f);
                currentState=SEARCH;
                break;
            }
        }

        imshow("Camera Feed", frame);
        if(waitKey(1)==27) break;
    }

    // --- Cleanup ---
    stop_flag = 1; ctrl_c_pressed = true; wallFollowStop=true;
    if(lidarThread.joinable()) lidarThread.join();
    sendCommand("M_STOP");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    sendCommand("SERVO_ANG:90");
    camera.stop();
    destroyAllWindows();
    return 0;


}
