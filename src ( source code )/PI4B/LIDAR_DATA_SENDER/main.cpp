#include <iostream>
#include <vector>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <cmath>

#include "sl_lidar.h"
#include "sl_lidar_driver.h"

#define INVERT_ANGLES 1

using namespace sl;

bool ctrl_c_pressed = false;
void ctrlc_handler(int) { ctrl_c_pressed = true; }

int main(int argc, char** argv) {
    signal(SIGINT, ctrlc_handler);

    // Check for --debug argument
    bool debug = false;
    for(int i = 1; i < argc; ++i) {
        if(std::strcmp(argv[i], "--debug") == 0) debug = true;
    }

    const char* port = "/dev/serial0";
    const sl_u32 baudrate = 460800;

    ILidarDriver* drv = *createLidarDriver();
    if (!drv) {
        std::cerr << "createLidarDriver failed\n";
        return -1;
    }

    IChannel* channel = *createSerialPortChannel(port, baudrate);
    if (!channel || !SL_IS_OK(drv->connect(channel))) {
        std::cerr << "connect failed\n";
        delete drv;
        return -1;
    }

    drv->setMotorSpeed(1023);
    drv->startScan(false, true);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5005);
    inet_aton("127.0.0.1", &addr.sin_addr);

    sl_lidar_response_measurement_node_hq_t nodes[8192];

    while (!ctrl_c_pressed) {
        size_t count = sizeof(nodes)/sizeof(nodes[0]);
        sl_result res = drv->grabScanDataHq(nodes, count);
        if (!SL_IS_OK(res)) continue;
        drv->ascendScanData(nodes, count);

        std::ostringstream ss;
        std::ostringstream debug_ss; // For debug printing

        for (size_t i = 0; i < count; ++i) {
            float dist = nodes[i].dist_mm_q2 / 4.0f;
            if (dist <= 0.0f) continue;

            float angle = nodes[i].angle_z_q14 * 90.f / 16384.f;
#if INVERT_ANGLES
	    angle = fmodf(360.0f - angle, 360.0f);  // invert angles at compile time
#endif

            uint8_t quality = (uint8_t)(nodes[i].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);

            ss << angle << "," << dist << "," << (int)quality << " ";

            if(debug) {
                debug_ss << "(" << angle << "°, " << dist << "mm, Q" << (int)quality << ") ";
            }
        }

        std::string data = ss.str();
        if (!data.empty()) {
            if (data.back() == ' ') data.pop_back();
            data += "\n";
            sendto(sock, data.c_str(), data.size(), 0, (sockaddr*)&addr, sizeof(addr));

            if(debug) {
                std::cout << "Sent: " << debug_ss.str() << std::endl;
            }
        }

        usleep(10000); // 10 ms
    }

    std::cout << "\nCtrl+C detected, stopping motor and exiting...\n";
    drv->stop();
    drv->setMotorSpeed(0);

    close(sock);
    delete drv;
    return 0;
}

