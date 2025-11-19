#include "camera_module.h"
#include <opencv2/opencv.hpp>
#include <csignal>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace cv;
using namespace std;

volatile std::sig_atomic_t stop_flag = 0;
void signalHandler(int){ stop_flag = 1; }

// ----------------------------------------------------
// HSV CONFIG STRUCT
// ----------------------------------------------------
struct HSVConfig {
    int hue_red = 5, sat_red = 200, val_red = 150;
    int hue_pink = 166, sat_pink = 201, val_pink = 63;
    int rangeH = 10, rangeS = 50, rangeV = 50;
};

// ----------------------------------------------------
// LOAD HSV CONFIG FROM FILE IF EXISTS
// ----------------------------------------------------
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

// ----------------------------------------------------
// DETECT RED / GREEN / PINK ORDERED LEFT → RIGHT
// RETURNS VECTOR<string> with order
// ----------------------------------------------------
vector<string> detectColorsOrdered(const Mat &frame, const HSVConfig &cfg) {

    Mat hsv; 
    cvtColor(frame, hsv, COLOR_BGR2HSV);

    Mat mask_red, mask_pink, mask_green;

    // -------------------------
    // compute HSV ranges
    // -------------------------
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

// ----------------------------------------------------
// DETECT RED / GREEN / PINK AND DRAW CONTOURS + LABELS
// ALSO RETURNS ORDERED COLORS
// ----------------------------------------------------
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
    int marginBottom = 80;   // <- tweak this if needed

    auto collect = [&](Mat &mask, string name) {
        vector<vector<Point>> cnts;
        findContours(mask, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (auto &c : cnts) {
            if (contourArea(c) < 120) continue;

            Rect box = boundingRect(c);

            // --- POSITION FILTER: ignore stuff glued to the very bottom ---
            if (box.y + box.height > bottomH - marginBottom)
                continue;   // floor line will be rejected here

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


// ----------------------------------------------------
// MAIN PROGRAM
// ----------------------------------------------------
int main(){
    signal(SIGINT, signalHandler);

    CameraModule camera([](lccv::PiCamera &){});
    if(!camera.start()){ cerr<<"Failed to start camera\n"; return -1; }

    HSVConfig cfg = loadHSVConfig();

    Mat frame;
    namedWindow("Camera Feed", WINDOW_AUTOSIZE);

    while(!stop_flag){
        TimedFrame timed;
        if(!camera.waitForFrame(timed)) break;
        frame = timed.frame;

        int H = frame.rows;
        Rect roi(0, H/2, frame.cols, H/2);
        Mat bottom = frame(roi).clone();

        Mat display = Mat::zeros(frame.size(), frame.type());
        bottom.copyTo(display(roi));

        // DETECT + DRAW + GET ORDER
        vector<string> colors =
            detectColorsAndDraw(bottom, display, cfg, H/2);

        // Build display text
        string txt = "Detected: ";
        for(auto &c : colors) txt += c + " ";

        putText(display, txt, Point(10,40),
                FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,255), 2);

        imshow("Camera Feed", display);
        if(waitKey(1) == 27) break;
    }

    camera.stop();
    destroyAllWindows();
    return 0;
}
