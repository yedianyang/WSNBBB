#pragma once

#include <libusb-1.0/libusb.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

// Struct to hold all Inkling data
// struct InklingState {
//     int x;
//     int y;
//     bool pressed;
//     long long loopTime;
// };

struct InklingData {
    int x;
    int y;
    bool pressed;
    std::chrono::steady_clock::time_point timestamp;
};

class WacomInkling {
private:
    // Inkling设备常量
    const uint16_t WACOM_VENDOR_ID = 0x056A;
    const uint16_t WACOM_PRODUCT_ID = 0x0221;
    const uint8_t USBRQ_HID_GET_REPORT = 0x01;
    const uint8_t USBRQ_HID_SET_REPORT = 0x09;
    
    // USB相关变量
    libusb_context* usbContext = nullptr;
    libusb_device_handle* deviceHandle = nullptr;
    int interface = 0;
    unsigned char endpointAddress = 0;
    int maxPacketSize = 0;
    
    // 线程控制
    std::atomic<bool> running{false};
    std::mutex dataMutex;
    std::thread dataThread;
    
    // 数据队列
    std::queue<InklingData> dataQueue;
    std::condition_variable dataCondition;
    
    // 当前数据
    InklingData currentData{0, 0, false};
    
    // 错误处理
    std::string errorMessage;
    
    // 私有方法
    void configureDevice();
    void dataLoop();
    void releaseDevice();
    bool openDevice();

public:
    WacomInkling();
    ~WacomInkling();
    
    // 初始化和控制
    bool initialize();
    void start();
    void stop();
    void release();
    bool reset();
    
    // 数据访问
    bool getData(InklingData& data);
    bool getLatestData(InklingData& data);
    
    // 错误处理
    std::string getLastError() const;
};
