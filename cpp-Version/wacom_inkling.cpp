/**
 * 使用说明/Instructions:
 * 
 * 1. 安装依赖/Install dependencies:
 *    - Raspberry Pi/Linux:
 *      sudo apt-get install libusb-1.0-0-dev
 *    - Mac:
 *      brew install libusb
 *      brew install pkg-config (可选/optional)
 * 
 * 2. 编译命令/Compile command: 
 *    - Raspberry Pi/Linux:
 *      g++ -std=c++11 -o wacom_inkling wacom_inkling.cpp -lusb-1.0
 *    - Mac使用pkg-config:
 *      g++ -std=c++11 -o wacom_inkling wacom_inkling.cpp $(pkg-config --cflags --libs libusb-1.0)
 *    - Mac不使用pkg-config:
 *      g++ -std=c++11 -o wacom_inkling wacom_inkling.cpp -I/opt/homebrew/include/libusb-1.0 -L/opt/homebrew/lib -lusb-1.0
 * 
 * 3. 运行命令/Run command: 
 *    sudo ./wacom_inkling
 *    (需要root权限访问USB设备/Root privileges needed for USB access)
 * 
 * 4. 使用/Usage:
 *    连接Wacom Inkling设备到Raspberry Pi 4B或Mac的USB端口
 *    Connect Wacom Inkling device to Raspberry Pi 4B or Mac USB port
 * 
 * 5. 注意事项/Notes:
 *    - 在Mac上可能需要允许安全扩展和USB设备访问权限
 *    - On Mac systems, you may need to allow security extensions and USB device access permissions
 */

#include <iostream>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <chrono>

// USB常量/USB constants
#define WACOM_VENDOR_ID 0x056A
#define WACOM_PRODUCT_ID 0x0221
#define USBRQ_HID_GET_REPORT 0x01
#define USBRQ_HID_SET_REPORT 0x09

int main() {
    libusb_device_handle *dev_handle = nullptr;
    libusb_context *ctx = nullptr;
    int ret = 0;
    unsigned char endpoint_address = 0;
    int interface = 0;
    int max_packet_size = 0;

    // 初始化libusb/Initialize libusb
    if (libusb_init(&ctx) < 0) {
        std::cerr << "Error initializing libusb" << std::endl;
        return 1;
    }

    // 打开Wacom Inkling设备/Open Wacom Inkling device
    dev_handle = libusb_open_device_with_vid_pid(ctx, WACOM_VENDOR_ID, WACOM_PRODUCT_ID);
    if (!dev_handle) {
        std::cerr << "Error: Wacom Inkling device not found!" << std::endl;
        libusb_exit(ctx);
        return 1;
    }

    // 获取端点信息/Get endpoint information
    libusb_device *device = libusb_get_device(dev_handle);
    struct libusb_config_descriptor *config;
    libusb_get_config_descriptor(device, 0, &config);
    endpoint_address = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
    max_packet_size = config->interface[0].altsetting[0].endpoint[0].wMaxPacketSize;
    
    std::cout << "ENDPOINT: " << (int)endpoint_address << std::endl;

    // 如果内核驱动程序处于活动状态则分离/Detach kernel driver if active
    if (libusb_kernel_driver_active(dev_handle, interface) == 1) {
        std::cout << "Kernel driver active, detaching..." << std::endl;
        if (libusb_detach_kernel_driver(dev_handle, interface) != 0) {
            std::cerr << "Error detaching kernel driver" << std::endl;
            libusb_close(dev_handle);
            libusb_exit(ctx);
            return 1;
        }
    }

    // 声明接口/Claim interface
    ret = libusb_claim_interface(dev_handle, interface);
    if (ret < 0) {
        std::cerr << "Error claiming interface: " << libusb_error_name(ret) << std::endl;
        libusb_close(dev_handle);
        libusb_exit(ctx);
        return 1;
    }

    // 设备配置 - 发送控制传输/Device configuration - sending control transfers
    // 第一次控制传输/First control transfer - 激活数字化仪/Activate digitizer
    std::vector<unsigned char> buf = {0x80, 0x01, 0x03, 0x01, 0x01};
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error in control transfer 1: " << libusb_error_name(ret) << std::endl;
    }

    // 第二次控制传输/Second control transfer - 设置数据模式/Set data mode
    buf = {0x80, 0x01, 0x0A, 0x01, 0x01, 0x0B, 0x01};
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error in control transfer 2: " << libusb_error_name(ret) << std::endl;
    }

    // 获取报告/Get report - 读取设备状态/Read device status
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error getting report 1: " << libusb_error_name(ret) << std::endl;
    } else {
        std::cout << "Get Report 1 result: ";
        for (int i = 0; i < ret; i++) {
            std::cout << (int)buf[i] << " ";
        }
        std::cout << std::endl;
    }

    // 第三次控制传输/Third control transfer - 设置响应模式/Set response mode
    buf = {0x80, 0x01, 0x0B, 0x01};
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error in control transfer 3: " << libusb_error_name(ret) << std::endl;
    }

    // 第四次控制传输/Fourth control transfer - 启用报告/Enable reporting
    buf = {0x80, 0x01, 0x02, 0x01, 0x01};
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error in control transfer 4: " << libusb_error_name(ret) << std::endl;
    }

    // 第五次控制传输/Fifth control transfer - 配置数据格式/Configure data format
    buf = {0x80, 0x01, 0x0A, 0x01, 0x01, 0x02, 0x01};
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error in control transfer 5: " << libusb_error_name(ret) << std::endl;
    }

    // 再次获取报告/Get report again - 确认设备状态/Confirm device status
    buf.resize(33, 0);
    ret = libusb_control_transfer(dev_handle, 0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    if (ret < 0) {
        std::cerr << "Error getting report 2: " << libusb_error_name(ret) << std::endl;
    } else {
        std::cout << "Get Report 2 result: ";
        for (int i = 0; i < ret; i++) {
            std::cout << (int)buf[i] << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "\n\ndata" << std::endl;

    // 读取数据循环/Read data loop - 持续获取设备坐标数据/Continuously get device coordinate data
    const int attempts = 50;
    int collected = 0;
    unsigned char data[max_packet_size];
    int transferred = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (collected < attempts || true) {
        // 使用中断传输从设备读取数据/Use interrupt transfer to read data from device
        ret = libusb_interrupt_transfer(dev_handle, endpoint_address, data, max_packet_size, &transferred, 1000);
        
        if (ret == 0 && transferred > 0) {
            collected++;
            // 解析X坐标、Y坐标和按压状态/Parse X coordinate, Y coordinate and pressed state
            int x = data[1] + (data[2] * 256);
            int y = data[3] + (data[4] * 256);
            int pressed = data[5];
            
            std::cout << "x:" << x << "\ty:" << y << "\tpressed:" << pressed << std::endl;
            std::cout << "Collected: " << collected << std::endl;
            
            // 1000秒后终止循环/Break after 1000 seconds to match Python's timeout
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            if (elapsed > 1000) {
                break;
            }
        } else if (ret == LIBUSB_ERROR_TIMEOUT) {
            // 超时继续尝试/Continue trying after timeout
            continue;
        } else {
            // 遇到错误则退出循环/Exit loop on error
            std::cerr << "Error reading data: " << libusb_error_name(ret) << std::endl;
            break;
        }
    }

    // 释放接口/Release interface
    libusb_release_interface(dev_handle, interface);
    
    // 清理/Clean up
    libusb_close(dev_handle);
    libusb_exit(ctx);
    
    return 0;
} 