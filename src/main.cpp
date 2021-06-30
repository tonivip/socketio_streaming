#include <iostream>
#include <string>
#include <chrono>
#include <NvInfer.h>
#include <NvInferPlugin.h>
#include <opencv2/opencv.hpp>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <opencv2/highgui.hpp>
#include "videoStreamer.h"
#include <unistd.h>
#include <sio_client.h>
#include <sio_message.h>
#include "base64.h"

using namespace sio;

class ScreenShot
{
    Display* display;
    Window root;
    int x,y,width,height;
    XImage* img{nullptr};
public:
    ScreenShot(int x, int y, int width, int height):
        x(x),
        y(y),
        width(width),
        height(height)
    {
        display = XOpenDisplay(nullptr);
        root = DefaultRootWindow(display);
    }

    void operator() (cv::Mat& cvImg)
    {
        if(img != nullptr)
            XDestroyImage(img);
        img = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
        cvImg = cv::Mat(height, width, CV_8UC4, img->data);
    }

    ~ScreenShot()
    {
        if(img != nullptr)
            XDestroyImage(img);
        XCloseDisplay(display);
    }
};


int main(){
    bool isCSICam = false;
    int videoFrameWidth = 640;
    int videoFrameHeight = 480;
    std::string msg;
    std::string msg2;
    std::vector<unsigned char> buff;
    std::vector<unsigned char> buff2;
    std::string uuid = "1";
    client h;
    h.connect("http://192.168.1.117:5002");
    h.socket()->emit("join-message", uuid);
    VideoStreamer videoStreamer = VideoStreamer(0, videoFrameWidth, videoFrameHeight, 60, isCSICam);
    ScreenShot screen(0,0,1024,600);
    // ScreenShot screen(0,0,640,480);
    cv::Mat frame;
    cv::Mat img;
    while(true){
        videoStreamer.getFrame(frame);
        if (frame.empty()) {
            std::cout << "Empty frame! Exiting...\n Try restarting nvargus-daemon by "
                            "doing: sudo systemctl restart nvargus-daemon" << std::endl;
            break;
        }
        cv::imencode(".jpg", frame, buff);
        auto *enc_msg = reinterpret_cast<unsigned char*>(buff.data());
        msg = base64_encode(enc_msg, buff.size());
        h.socket()->emit("screen-data", msg);
        screen(img);
        cv::imencode(".jpg", img, buff2);
        auto *enc_msg2 = reinterpret_cast<unsigned char*>(buff2.data());
        msg2 = base64_encode(enc_msg2, buff2.size());
        h.socket()->emit("camera-data", msg2);
        frame.release();
    }
    return 0;
}