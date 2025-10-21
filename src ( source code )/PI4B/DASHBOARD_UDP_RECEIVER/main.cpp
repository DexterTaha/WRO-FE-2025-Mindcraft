#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <opencv2/opencv.hpp>
#include <sys/time.h>
#include <cmath>


std::atomic<bool> ctrl_c_pressed(false);
void ctrlc_handler(int) { ctrl_c_pressed = true; }

// --- Data structures ---
struct LidarPoint { float angle; float dist; uint8_t quality; };
struct ImuSample {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float lin_x = 0.0f;
    float lin_y = 0.0f;
    float delta = 0.0f;
    std::string rotation_dir = "";
    int total_rot = 0;
};

// Thread-safe storage
std::mutex mtx;
std::vector<LidarPoint> latestLidar;
ImuSample latestImu;

// --- Highlight angles (dynamic) ---
std::mutex angle_mtx;
std::vector<float> highlightAngles{360, 203, 123.4f};

// Helper to parse angles from a comma separated string
std::vector<float> parseAnglesFromString(const std::string &s) {
    std::vector<float> out;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // trim
        size_t a = token.find_first_not_of(" \t\r\n");
        size_t b = token.find_last_not_of(" \t\r\n");
        if (a == std::string::npos) continue;
        std::string t = token.substr(a, b - a + 1);
        try {
            out.push_back(std::stof(t));
        } catch (...) { /* ignore bad tokens */ }
    }
    return out;
}


// --- UDP parsing functions ---
std::vector<LidarPoint> parseLidarData(const std::string &data) {
    std::vector<LidarPoint> points;
    std::istringstream ss(data);
    std::string token;
    while (ss >> token) {
        size_t pos1 = token.find(',');
        size_t pos2 = token.find_last_of(',');
        if (pos1 == std::string::npos || pos2 == std::string::npos || pos1 == pos2) continue;
        try {
            float angle = std::stof(token.substr(0, pos1));
            float dist  = std::stof(token.substr(pos1 + 1, pos2 - pos1 - 1));
            int qual    = std::stoi(token.substr(pos2 + 1));
            points.push_back({angle, dist, static_cast<uint8_t>(qual)});
        } catch (...) {
            continue;
        }
    }
    return points;
}

ImuSample parseImuData(const std::string &data) {
    ImuSample imu;
    std::istringstream ss(data);
    std::string token;
    std::vector<std::string> toks;
    while (std::getline(ss, token, ',')) {
        size_t a = token.find_first_not_of(" \t\r\n");
        size_t b = token.find_last_not_of(" \t\r\n");
        if (a == std::string::npos) toks.push_back("");
        else toks.push_back(token.substr(a, b - a + 1));
    }

    try {
        if (toks.size() > 0) imu.roll = std::stof(toks[0]);
        if (toks.size() > 1) imu.pitch = std::stof(toks[1]);
        if (toks.size() > 2) imu.yaw = std::stof(toks[2]);
    } catch (...) {}

    if (toks.size() >= 6) {
        try { imu.delta = std::stof(toks[3]); } catch(...) { imu.delta = 0.0f; }
        imu.rotation_dir = toks[4];
        if (imu.rotation_dir == "None" || imu.rotation_dir == "none" || imu.rotation_dir == "—") imu.rotation_dir.clear();
        try { imu.total_rot = std::stoi(toks[5]); } catch(...) { imu.total_rot = 0; }
        imu.lin_x = 0.0f;
        imu.lin_y = 0.0f;
    } else if (toks.size() >= 5) {
        try { imu.lin_x = std::stof(toks[3]); } catch(...) { imu.lin_x = 0.0f; }
        try { imu.lin_y = std::stof(toks[4]); } catch(...) { imu.lin_y = 0.0f; }
        imu.delta = 0.0f;
        imu.rotation_dir.clear();
        imu.total_rot = 0;
    }
    return imu;
}

// --- UDP receiver threads ---
void udpReceiverThread(int port, bool isLidar) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) { perror("socket"); return; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); close(sock); return; }

    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 200000; // 200 ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buffer[65536];
    while (!ctrl_c_pressed) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer)-1, 0, nullptr, nullptr);
        if(len > 0) {
            buffer[len] = '\0';
            std::lock_guard<std::mutex> lk(mtx);
            if(isLidar) latestLidar = parseLidarData(buffer);
            else latestImu = parseImuData(buffer);
        }
    }
    close(sock);
}

static inline float normalize360(double a) {
    double r = std::fmod(a, 360.0);    // remainder can be negative
    if (r < 0.0) r += 360.0;
    return static_cast<float>(r);
}

static inline float addAndWrapYaw(double yaw, double offset_deg) {
    return normalize360(yaw + offset_deg);
}

static inline float invertYawAround(double yaw, int invert_value) {
    // yaw expected normalized in [0,360)
    return normalize360(invert_value - yaw);
}


// --- Thread to read dynamic angles from stdin ---
void angleInputThread() {
    std::string line;
    while (!ctrl_c_pressed) {
        std::cout << "Enter angles separated by ',' (e.g., 0,90,180): ";
        if(!std::getline(std::cin, line)) continue;

        std::vector<float> newAngles = parseAnglesFromString(line);
        if(!newAngles.empty()) {
            std::lock_guard<std::mutex> lk(angle_mtx);
            highlightAngles = newAngles;
        }
    }
}

// --- Helper to draw hollow triangle ---
void drawRobotTriangle(cv::Mat &img, cv::Point center, float yaw_deg, float size=40.0f) {
    float yaw_rad = yaw_deg * CV_PI / 180.0f;

    cv::Point tip(center.x + int(size * cos(yaw_rad)),
            center.y - int(size * sin(yaw_rad)));
    cv::Point left(center.x + int(size/2 * cos(yaw_rad + 2.5f)),
            center.y - int(size/2 * sin(yaw_rad + 2.5f)));
    cv::Point right(center.x + int(size/2 * cos(yaw_rad - 2.5f)),
            center.y - int(size/2 * sin(yaw_rad - 2.5f)));

    std::vector<cv::Point> pts = { tip, left, right };
    cv::polylines(img, pts, true, cv::Scalar(0,255,255), 2);
}

int main(int argc, char** argv) {
    signal(SIGINT, ctrlc_handler);

    // --- CLI options ---
    // default: input thread disabled (user asked)
    bool enableInputThread = false;

    // parse CLI args: --input enables stdin thread, --angles sets initial highlight angles
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            printf(
                    "Robot Dashboard Usage:\n"
                    "  --input              Enable stdin angle input thread (disabled by default)\n"
                    "  --angles \"0,90,180\"  Set initial highlight angles via CLI\n"
                    "  --angles=0,90,180    Alternative syntax for highlight angles\n"
                    "  -h, --help           Show this help message\n"
                  );
            return 0; // exit after printing help
        }
        if (a == "--input") {
            enableInputThread = true;
        } else if (a == "--angles" && i + 1 < argc) {
            std::vector<float> parsed = parseAnglesFromString(argv[i+1]);
            if (!parsed.empty()) {
                std::lock_guard<std::mutex> lk(angle_mtx);
                highlightAngles = parsed;
            }
            ++i; // skip value
        } else if (a.rfind("--angles=", 0) == 0) {
            std::string val = a.substr(strlen("--angles="));
            std::vector<float> parsed = parseAnglesFromString(val);
            if (!parsed.empty()) {
                std::lock_guard<std::mutex> lk(angle_mtx);
                highlightAngles = parsed;
            }
        }
    }

    // Start UDP receiver threads
    std::thread lidarThread(udpReceiverThread, 5005, true);
    std::thread imuThread(udpReceiverThread, 5006, false);

    // Start angle input thread optionally
    std::thread inputThread;
    if (enableInputThread) inputThread = std::thread(angleInputThread);

    // --- Display setup ---
    const int win_size = 800;
    cv::Mat display(win_size, win_size, CV_8UC3, cv::Scalar(0,0,0));
    const cv::Point center(win_size/2, win_size/2);
    cv::namedWindow("Robot Dashboard", cv::WINDOW_AUTOSIZE);

    float prev_yaw = 0.0f;
    const float alpha = 0.2f;
    const float green_len = 150.0f;

    while (!ctrl_c_pressed) {
        display.setTo(cv::Scalar(0,0,0));

        // Draw reference circle & axes
        cv::circle(display, center, 300, cv::Scalar(100,100,100), 1);
        cv::line(display, center, cv::Point(center.x + 300, center.y), cv::Scalar(0,255,255), 1);
        cv::line(display, center, cv::Point(center.x, center.y - 300), cv::Scalar(0,255,255), 1);
        cv::line(display, center, cv::Point(center.x - 300, center.y), cv::Scalar(0,255,255), 1);
        cv::line(display, center, cv::Point(center.x, center.y + 300), cv::Scalar(0,255,255), 1);

        // Angle labels
        cv::putText(display, "0",   cv::Point(center.x + 305, center.y + 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);
        cv::putText(display, "90",  cv::Point(center.x - 20, center.y - 305), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);
        cv::putText(display, "180", cv::Point(center.x - 330, center.y + 5), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);
        cv::putText(display, "270", cv::Point(center.x - 20, center.y + 320), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255,255,255), 2);

        // Copy latest data safely
        std::vector<LidarPoint> lidarCopy;
        ImuSample imuCopy;
        {
            std::lock_guard<std::mutex> lk(mtx);
            lidarCopy = latestLidar;
            imuCopy = latestImu;
        }

        // Draw LIDAR points (orange)
        for(auto &p : lidarCopy) {
            float rad = p.angle * CV_PI / 180.0f;
            int x = static_cast<int>(center.x + p.dist * 0.2f * cos(rad));
            int y = static_cast<int>(center.y - p.dist * 0.2f * sin(rad));
            cv::circle(display, cv::Point(x,y), 2, cv::Scalar(0,140,255), -1);
        }

        // Draw highlighted angles with distance & label
        std::vector<float> currentAngles;
        {
            std::lock_guard<std::mutex> lk(angle_mtx);
            currentAngles = highlightAngles;
        }

        for(float angle : currentAngles) {
            float minDiff = 360.0f;
            float dist = 200.0f;
            for(auto &p : lidarCopy) {
                float diff = fabs(p.angle - angle);
                if(diff < minDiff) { minDiff = diff; dist = p.dist; }
            }
            float rad = angle * CV_PI / 180.0f;
            int x = static_cast<int>(center.x + dist * 0.2f * cos(rad));
            int y = static_cast<int>(center.y - dist * 0.2f * sin(rad));
            cv::circle(display, cv::Point(x, y), 6, cv::Scalar(255,0,255), -1); // purple
            cv::putText(display, cv::format("%.1f", angle), cv::Point(x+8, y-8),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255,0,255), 2);
        }

        // Smooth yaw (for UI smoothing only)
        float smooth_yaw = alpha*imuCopy.yaw + (1-alpha)*prev_yaw;
        prev_yaw = smooth_yaw;

        float yaw_rad = invertYawAround(addAndWrapYaw(imuCopy.yaw, 90.0f), 180) * CV_PI / 180.0f;
        //float yaw_rad = imuCopy.yaw * CV_PI / 180.0f;

        // Draw heading arrow (green)
        cv::Point heading(
                center.x + int(green_len * cos(yaw_rad)),
                center.y - int(green_len * sin(yaw_rad))
                );
        cv::arrowedLine(display, center, heading, cv::Scalar(0,255,0), 2);

        // Draw linear acceleration arrow (red)
        float lin_scale = 50.0f;
        float lin_world_x = imuCopy.lin_x * cos(yaw_rad) - imuCopy.lin_y * sin(yaw_rad);
        float lin_world_y = imuCopy.lin_x * sin(yaw_rad) + imuCopy.lin_y * cos(yaw_rad);
        cv::Point lin_end(center.x + int(lin_world_x * lin_scale),
                center.y - int(lin_world_y * lin_scale));
        cv::arrowedLine(display, center, lin_end, cv::Scalar(0,0,255), 2);

        // Robot triangle
        drawRobotTriangle(display, center, 90.0f, 40.0f);

        // Top-left info text
        int line = 20;
        cv::putText(display, cv::format("Yaw: %.2f", imuCopy.yaw), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2); line+=25;
        cv::putText(display, cv::format("Pitch: %.2f", imuCopy.pitch), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,255), 2); line+=25;
        cv::putText(display, cv::format("Roll: %.2f", imuCopy.roll), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,255), 2); line+=25;

        if (imuCopy.delta != 0.0f) {
            cv::putText(display, cv::format("delta: %.3f", imuCopy.delta), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200,200,0), 2);
        } else {
            cv::putText(display, "delta: 0.000", cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200,200,0), 2);
        }
        line += 25;

        std::string dir_display = imuCopy.rotation_dir.empty() ? "—" : imuCopy.rotation_dir;
        cv::putText(display, cv::format("Dir: %s", dir_display.c_str()), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,200,0), 2); line+=25;
        cv::putText(display, cv::format("Rotations: %+d", imuCopy.total_rot), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,200,0), 2); line+=25;

        cv::putText(display, cv::format("Lin X: %.2f", imuCopy.lin_x), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,100,0), 2); line+=25;
        cv::putText(display, cv::format("Lin Y: %.2f", imuCopy.lin_y), cv::Point(10,line), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,0,255), 2);

        // Legend
        int rx = win_size - 260;
        int ry = 20;
        cv::putText(display, "GV: Yaw vector", cv::Point(rx, ry), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,0), 2); ry += 25;
        cv::putText(display, "RV: Acceleration vector", cv::Point(rx, ry), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,0,255), 2); ry += 25;

        cv::imshow("Robot Dashboard", display);
        int key = cv::waitKey(1);
        if(key == 27) ctrl_c_pressed = true;
    }

    ctrl_c_pressed = true;
    lidarThread.join();
    imuThread.join();
    if (enableInputThread && inputThread.joinable()) inputThread.join();
    return 0;
}

