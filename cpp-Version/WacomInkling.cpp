#include "WacomInkling.hpp"
#include <iostream>

WacomInkling::WacomInkling() {}

WacomInkling::~WacomInkling() {
    stop();
    if (deviceHandle) {
        libusb_release_interface(deviceHandle, interface);
        libusb_close(deviceHandle);
    }
    if (usbContext) {
        libusb_exit(usbContext);
    }
}

bool WacomInkling::initialize() {
    try {
        if (libusb_init(&usbContext) < 0) {
            errorMessage = "无法初始化libusb";
            return false;
        }
        
        deviceHandle = libusb_open_device_with_vid_pid(usbContext, WACOM_VENDOR_ID, WACOM_PRODUCT_ID);
        if (!deviceHandle) {
            errorMessage = "找不到Wacom Inkling设备";
            return false;
        }
        
        // 获取端点信息
        libusb_device* device = libusb_get_device(deviceHandle);
        struct libusb_config_descriptor* config;
        libusb_get_config_descriptor(device, 0, &config);
        endpointAddress = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
        maxPacketSize = config->interface[0].altsetting[0].endpoint[0].wMaxPacketSize;
        
        if (libusb_kernel_driver_active(deviceHandle, interface) == 1) {
            if (libusb_detach_kernel_driver(deviceHandle, interface) != 0) {
                errorMessage = "无法分离内核驱动程序";
                return false;
            }
        }
        
        if (libusb_claim_interface(deviceHandle, interface) < 0) {
            errorMessage = "无法声明接口";
            return false;
        }
        
        configureDevice();
        return true;
    }
    catch (const std::exception& e) {
        errorMessage = std::string("初始化错误: ") + e.what();
        return false;
    }
}

void WacomInkling::configureDevice() {
    std::vector<unsigned char> buf;
    
    // 配置序列
    const std::vector<std::vector<unsigned char>> configs = {
        {0x80, 0x01, 0x03, 0x01, 0x01},
        {0x80, 0x01, 0x0A, 0x01, 0x01, 0x0B, 0x01},
        {0x80, 0x01, 0x0B, 0x01},
        {0x80, 0x01, 0x02, 0x01, 0x01},
        {0x80, 0x01, 0x0A, 0x01, 0x01, 0x02, 0x01}
    };
    
    for (const auto& config : configs) {
        buf = config;
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    }
}

void WacomInkling::start() {
    if (running) return;
    
    running = true;
    dataThread = std::thread(&WacomInkling::dataLoop, this);
}

void WacomInkling::stop() {
    running = false;
    if (dataThread.joinable()) {
        dataThread.join();
    }
}

void WacomInkling::dataLoop() {
    unsigned char data[maxPacketSize];
    int transferred = 0;
    
    while (running) {
        try {
            int ret = libusb_interrupt_transfer(
                deviceHandle, endpointAddress, data, maxPacketSize, &transferred, 1000);
            
            if (ret == 0 && transferred > 0) {
                InklingData newData;
                newData.x = data[1] + (data[2] * 256);
                newData.y = data[3] + (data[4] * 256);
                newData.pressed = data[5];
                newData.timestamp = std::chrono::steady_clock::now();
                
                {
                    std::lock_guard<std::mutex> lock(dataMutex);
                    currentData = newData;
                    dataQueue.push(newData);
                }
                dataCondition.notify_one();
            }
        }
        catch (const std::exception& e) {
            errorMessage = std::string("数据读取错误: ") + e.what();
        }
    }
}

bool WacomInkling::getData(InklingData& data) {
    std::unique_lock<std::mutex> lock(dataMutex);
    if (dataQueue.empty()) {
        return false;
    }
    
    data = dataQueue.front();
    dataQueue.pop();
    return true;
}

bool WacomInkling::getLatestData(InklingData& data) {
    std::lock_guard<std::mutex> lock(dataMutex);
    data = currentData;
    return true;
}

std::string WacomInkling::getLastError() const {
    return errorMessage;
}
