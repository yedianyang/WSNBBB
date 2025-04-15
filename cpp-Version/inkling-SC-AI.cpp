// InklingClearPathSCController.cpp - 基于Wacom Inkling控制ClearPath-SC电机

#include <iostream>
#include <libusb.h>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <cmath>
#include <string>
#include <queue>
#include <condition_variable>
#include "sFoundation.h"  // ClearPath SC SDK头文件

// 错误代码定义
enum ErrorCode {
    SUCCESS = 0,
    INKLING_INIT_ERROR = 100,
    INKLING_READ_ERROR = 101,
    CLEARPATH_INIT_ERROR = 200,
    CLEARPATH_PORT_ERROR = 201,
    CLEARPATH_NODE_ERROR = 202,
    CLEARPATH_MOVE_ERROR = 203,
    THREAD_ERROR = 300
};

// 坐标和状态数据结构
struct PenData {
    int x;
    int y;
    bool pressed;
    std::chrono::steady_clock::time_point timestamp;
};

class InklingClearPathSCController {
private:
    // Inkling设备常量
    const uint16_t WACOM_VENDOR_ID = 0x056A;
    const uint16_t WACOM_PRODUCT_ID = 0x0221;
    const uint8_t USBRQ_HID_GET_REPORT = 0x01;
    const uint8_t USBRQ_HID_SET_REPORT = 0x09;
    
    // ClearPath-SC参数
    const int X_MAX_POS = 240;  // X轴最大位置
    const int Y_MAX_POS = 240;  // Y轴最大位置
    const double FILTER_COEFFICIENT = 0.7;  // 滤波系数
    
    // 映射参数
    const double X_SCALE = 193.0 / 1920.0;  // X轴缩放系数
    const double Y_SCALE = -145.0 / 1920.0; // Y轴缩放系数（负值表示反向）
    const int X_TARGET = 960;  // 目标X坐标
    const int Y_TARGET = 960;  // 目标Y坐标
    
    // libusb相关变量
    libusb_context* usbContext = nullptr;
    libusb_device_handle* deviceHandle = nullptr;
    int interface = 0;
    unsigned char endpointAddress = 0;
    int maxPacketSize = 0;
    
    // ClearPath-SC相关变量
    SysManager* myMgr = nullptr;
    std::vector<INode*> xAxisNode;  // X轴节点
    std::vector<INode*> yAxisNode;  // Y轴节点
    double xCurrentPos = 0.0;
    double yCurrentPos = 0.0;
    
    // 线程控制
    std::atomic<bool> running{false};
    std::mutex dataMutex;
    std::mutex controlMutex;
    std::thread inklingThread;
    std::thread motorControlThread;
    
    // 坐标数据
    PenData currentPenData{0, 0, false};
    std::queue<PenData> dataQueue;
    std::condition_variable dataCondition;
    
    // 滤波后的坐标
    double filteredX = 0.0;
    double filteredY = 0.0;
    
    // 错误处理
    std::atomic<ErrorCode> lastError{SUCCESS};
    std::string errorMessage;

public:
    InklingClearPathSCController() {
        // 构造函数，初始化默认值
    }
    
    ~InklingClearPathSCController() {
        stop();
        cleanup();
    }
    
    ErrorCode initialize() {
        // 初始化Inkling设备
        ErrorCode result = initializeInkling();
        if (result != SUCCESS) {
            return result;
        }
        
        // 初始化ClearPath-SC
        result = initializeClearPathSC();
        if (result != SUCCESS) {
            cleanupInkling();
            return result;
        }
        
        return SUCCESS;
    }
    
    ErrorCode start() {
        if (running) {
            return SUCCESS;  // 已经在运行中
        }
        
        running = true;
        
        try {
            // 启动Inkling数据获取线程
            inklingThread = std::thread(&InklingClearPathSCController::inklingDataLoop, this);
            
            // 启动电机控制线程
            motorControlThread = std::thread(&InklingClearPathSCController::motorControlLoop, this);
        } catch (const std::exception& e) {
            running = false;
            lastError = THREAD_ERROR;
            errorMessage = "线程创建失败: " + std::string(e.what());
            return THREAD_ERROR;
        }
        
        return SUCCESS;
    }
    
    void stop() {
        if (!running) {
            return;
        }
        
        running = false;
        dataCondition.notify_all();
        
        if (inklingThread.joinable()) {
            inklingThread.join();
        }
        
        if (motorControlThread.joinable()) {
            motorControlThread.join();
        }
    }
    
    ErrorCode getLastError(std::string& message) {
        message = errorMessage;
        return lastError;
    }
    
    void setTarget(int x, int y) {
        std::lock_guard<std::mutex> lock(controlMutex);
        X_TARGET = x;
        Y_TARGET = y;
    }
    
    void getCurrentPosition(double& x, double& y) {
        std::lock_guard<std::mutex> lock(controlMutex);
        x = xCurrentPos;
        y = yCurrentPos;
    }

private:
    ErrorCode initializeInkling() {
        // 初始化libusb
        if (libusb_init(&usbContext) < 0) {
            errorMessage = "无法初始化libusb";
            return INKLING_INIT_ERROR;
        }
        
        // 打开Wacom Inkling设备
        deviceHandle = libusb_open_device_with_vid_pid(usbContext, WACOM_VENDOR_ID, WACOM_PRODUCT_ID);
        if (!deviceHandle) {
            errorMessage = "找不到Wacom Inkling设备";
            libusb_exit(usbContext);
            return INKLING_INIT_ERROR;
        }
        
        // 获取端点信息
        libusb_device* device = libusb_get_device(deviceHandle);
        struct libusb_config_descriptor* config;
        libusb_get_config_descriptor(device, 0, &config);
        endpointAddress = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
        maxPacketSize = config->interface[0].altsetting[0].endpoint[0].wMaxPacketSize;
        
        // 如果内核驱动程序处于活动状态则分离
        if (libusb_kernel_driver_active(deviceHandle, interface) == 1) {
            if (libusb_detach_kernel_driver(deviceHandle, interface) != 0) {
                errorMessage = "无法分离内核驱动程序";
                libusb_close(deviceHandle);
                libusb_exit(usbContext);
                return INKLING_INIT_ERROR;
            }
        }
        
        // 声明接口
        int ret = libusb_claim_interface(deviceHandle, interface);
        if (ret < 0) {
            errorMessage = "无法声明接口: " + std::string(libusb_error_name(ret));
            libusb_close(deviceHandle);
            libusb_exit(usbContext);
            return INKLING_INIT_ERROR;
        }
        
        // 设备配置 - 发送控制传输
        configureInkling();
        
        return SUCCESS;
    }
    
    void configureInkling() {
        // 各种控制传输配置，遵循wacom_inkling.cpp中的配置步骤
        std::vector<unsigned char> buf;
        
        // 第一次控制传输 - 激活数字化仪
        buf = {0x80, 0x01, 0x03, 0x01, 0x01};
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 第二次控制传输 - 设置数据模式
        buf = {0x80, 0x01, 0x0A, 0x01, 0x01, 0x0B, 0x01};
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 获取报告 - 读取设备状态
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 第三次控制传输 - 设置响应模式
        buf = {0x80, 0x01, 0x0B, 0x01};
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 第四次控制传输 - 启用报告
        buf = {0x80, 0x01, 0x02, 0x01, 0x01};
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 第五次控制传输 - 配置数据格式
        buf = {0x80, 0x01, 0x0A, 0x01, 0x01, 0x02, 0x01};
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
        
        // 再次获取报告 - 确认设备状态
        buf.resize(33, 0);
        libusb_control_transfer(deviceHandle, 0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, buf.data(), buf.size(), 0);
    }
    
    ErrorCode initializeClearPathSC() {
        try {
            // 创建SysManager实例
            myMgr = SysManager::Instance();
            
            // 查找SC Hub端口
            std::vector<std::string> comHubPorts;
            SysManager::FindComHubPorts(comHubPorts);
            
            if (comHubPorts.empty()) {
                errorMessage = "未找到SC Hub端口";
                return CLEARPATH_PORT_ERROR;
            }
            
            // 配置SC Hub端口
            for (size_t i = 0; i < comHubPorts.size() && i < NET_CONTROLLER_MAX; i++) {
                myMgr->ComHubPort(i, comHubPorts[i].c_str());
            }
            
            // 打开端口
            myMgr->PortsOpen(comHubPorts.size());
            
            // 检查端口状态，并收集节点信息
            for (size_t i = 0; i < comHubPorts.size(); i++) {
                IPort& port = myMgr->Ports(i);
                
                // 检查端口状态
                if (port.OpenState() != PORT_OPENED) {
                    errorMessage = "端口 " + std::to_string(i) + " 打开失败";
                    return CLEARPATH_PORT_ERROR;
                }
                
                // 检查节点
                if (port.NodeCount() < 2) {  // 至少需要2个节点（X轴和Y轴）
                    errorMessage = "端口 " + std::to_string(i) + " 上没有足够的节点";
                    return CLEARPATH_NODE_ERROR;
                }
                
                // 启用注意事件
                port.Adv.Attn.Enable(true);
                
                // 检查节点类型并配置节点
                for (size_t j = 0; j < port.NodeCount(); j++) {
                    INode& node = port.Nodes(j);
                    
                    // 确保是ClearPath-SC高级电机
                    if (node.Info.NodeType() != IInfo::CLEARPATH_SC_ADV) {
                        errorMessage = "节点 " + std::to_string(j) + " 不是ClearPath-SC高级电机";
                        return CLEARPATH_NODE_ERROR;
                    }
                    
                    // 检查访问级别
                    if (!node.Setup.AccessLevelIsFull()) {
                        errorMessage = "节点 " + std::to_string(j) + " 没有完全访问权限";
                        return CLEARPATH_NODE_ERROR;
                    }
                    
                    // 设置加速度限制
                    node.Motion.AccLimit = 100000;
                    
                    // 根据节点索引分配X轴和Y轴
                    if (j == 0) {
                        xAxisNode.push_back(&node);
                    } else if (j == 1) {
                        yAxisNode.push_back(&node);
                    }
                    
                    // 启用中断移动功能
                    node.Info.Ex.Parameter(98, 1);
                }
            }
            
            // 初始位置归零 - Home操作
            homeAxes();
            
            // 在initializeClearPathSC()中添加性能优化设置
            for (auto& node : xAxisNode) {
                node->Info.Ex.Parameter(98, 1);  // 启用中断移动
                node->Motion.AccLimit = 100000;   // 提高加速度限制
                node->Motion.VelLimit = 50000;    // 设置适当的速度限制
                // 可以根据实际需求调整这些参数
            }
            
            // 同样设置Y轴节点
            for (auto& node : yAxisNode) {
                node->Info.Ex.Parameter(98, 1);
                node->Motion.AccLimit = 100000;
                node->Motion.VelLimit = 50000;
            }
            
            return SUCCESS;
        } catch (mnErr& theErr) {
            errorMessage = "ClearPath-SC错误: 地址=" + std::to_string(theErr.TheAddr) + 
                           ", 代码=0x" + std::to_string(theErr.ErrorCode) + 
                           ", 消息=" + std::string(theErr.ErrorMsg);
            return CLEARPATH_INIT_ERROR;
        } catch (std::exception& e) {
            errorMessage = "初始化ClearPath-SC时发生异常: " + std::string(e.what());
            return CLEARPATH_INIT_ERROR;
        }
    }
    
    void homeAxes() {
        // X轴归零
        for (auto& node : xAxisNode) {
            node->Motion.MoveWentDone();  // 清除之前的移动状态
            node->Motion.MovePosnStart(0);  // 移动到原点位置
            
            // 等待移动完成
            while (!node->Motion.MoveIsDone()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        // Y轴归零
        for (auto& node : yAxisNode) {
            node->Motion.MoveWentDone();  // 清除之前的移动状态
            node->Motion.MovePosnStart(0);  // 移动到原点位置
            
            // 等待移动完成
            while (!node->Motion.MoveIsDone()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    
    void cleanup() {
        cleanupClearPathSC();
        cleanupInkling();
    }
    
    void cleanupInkling() {
        if (deviceHandle) {
            libusb_release_interface(deviceHandle, interface);
            libusb_close(deviceHandle);
            deviceHandle = nullptr;
        }
        
        if (usbContext) {
            libusb_exit(usbContext);
            usbContext = nullptr;
        }
    }
    
    void cleanupClearPathSC() {
        if (myMgr) {
            myMgr->PortsClose();
            myMgr = nullptr;
        }
    }
    
    void inklingDataLoop() {
        unsigned char data[maxPacketSize];
        int transferred = 0;
        auto lastReadTime = std::chrono::steady_clock::now();
        
        while (running) {
            try {
                // 使用中断传输从设备读取数据
                int ret = libusb_interrupt_transfer(
                    deviceHandle, endpointAddress, data, maxPacketSize, &transferred, 1000);
                
                auto currentTime = std::chrono::steady_clock::now();
                
                if (ret == 0 && transferred > 0) {
                    // 解析X坐标、Y坐标和按压状态
                    int newX = data[1] + (data[2] * 256);
                    int newY = data[3] + (data[4] * 256);
                    bool newPressed = data[5];
                    
                    // 创建新的数据点
                    PenData newData = {newX, newY, newPressed, currentTime};
                    
                    // 更新当前数据并添加到队列
                    {
                        std::lock_guard<std::mutex> lock(dataMutex);
                        currentPenData = newData;
                        dataQueue.push(newData);
                    }
                    
                    // 通知数据处理线程
                    dataCondition.notify_one();
                    
                    // 更新上次成功读取时间
                    lastReadTime = currentTime;
                } else if (ret == LIBUSB_ERROR_TIMEOUT) {
                    // 超时，检查是否过长时间未读取到数据
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        currentTime - lastReadTime).count();
                    
                    if (elapsed > 10) {  // 10秒无数据，可能设备已断开
                        lastError = INKLING_READ_ERROR;
                        errorMessage = "10秒内未读取到数据，设备可能已断开连接";
                    }
                } else {
                    // 读取错误
                    lastError = INKLING_READ_ERROR;
                    errorMessage = "读取数据错误: " + std::string(libusb_error_name(ret));
                    
                    // 短暂暂停防止错误循环过快
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } catch (const std::exception& e) {
                lastError = INKLING_READ_ERROR;
                errorMessage = "读取线程异常: " + std::string(e.what());
                
                // 短暂暂停防止错误循环过快
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    
    double applyFilter(double previousValue, double newValue) {
        return previousValue * FILTER_COEFFICIENT + newValue * (1 - FILTER_COEFFICIENT);
    }
    
    void motorControlLoop() {
        try {
            while (running) {
                PenData data;
                bool hasData = false;
                
                // 从队列获取数据
                {
                    std::unique_lock<std::mutex> lock(dataMutex);
                    if (dataQueue.empty()) {
                        // 等待新数据，最多等待100ms
                        dataCondition.wait_for(lock, std::chrono::milliseconds(100));
                    }
                    
                    if (!dataQueue.empty()) {
                        data = dataQueue.front();
                        dataQueue.pop();
                        hasData = true;
                    }
                }
                
                if (hasData) {
                    // 应用滤波
                    filteredX = applyFilter(filteredX, data.x);
                    filteredY = applyFilter(filteredY, data.y);
                    
                    // 计算相对于目标的偏差
                    double deltaX = filteredX - X_TARGET;
                    double deltaY = filteredY - Y_TARGET;
                    
                    // 映射到电机控制参数
                    double xMove = deltaX * X_SCALE;
                    double yMove = deltaY * Y_SCALE;
                    
                    // 计算新的位置
                    double xTargetPos = xCurrentPos + xMove;
                    double yTargetPos = yCurrentPos + yMove;
                    
                    // 防止越界
                    xTargetPos = std::max(0.0, std::min(static_cast<double>(X_MAX_POS), xTargetPos));
                    yTargetPos = std::max(0.0, std::min(static_cast<double>(Y_MAX_POS), yTargetPos));
                    
                    // 检查移动距离是否足够大
                    if (std::abs(xMove) > 2 || std::abs(yMove) > 2) {
                        moveToPosition(xTargetPos, yTargetPos);
                        
                        // 更新当前位置
                        std::lock_guard<std::mutex> lock(controlMutex);
                        xCurrentPos = xTargetPos;
                        yCurrentPos = yTargetPos;
                    }
                }
            }
        } catch (mnErr& theErr) {
            lastError = CLEARPATH_MOVE_ERROR;
            errorMessage = "ClearPath-SC错误: 地址=" + std::to_string(theErr.TheAddr) + 
                           ", 代码=0x" + std::to_string(theErr.ErrorCode) + 
                           ", 消息=" + std::string(theErr.ErrorMsg);
        } catch (const std::exception& e) {
            lastError = THREAD_ERROR;
            errorMessage = "电机控制线程异常: " + std::string(e.what());
        }
    }
    
    void moveToPosition(double x, double y) {
        static auto lastMoveTime = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();
        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastMoveTime).count();
        
        // 限制命令发送频率，例如最小间隔5ms
        if (timeDiff < 5) {
            return;
        }
        
        // 更新移动时间戳
        lastMoveTime = currentTime;
        
        // X轴移动
        for (auto& node : xAxisNode) {
            node->Motion.MoveWentDone();
            node->Motion.MovePosnStart(x);
        }
        
        // Y轴移动
        for (auto& node : yAxisNode) {
            node->Motion.MoveWentDone();
            node->Motion.MovePosnStart(y);
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "Wacom Inkling与ClearPath-SC实时同步控制程序启动..." << std::endl;
    
    InklingClearPathSCController controller;
    
    ErrorCode result = controller.initialize();
    if (result != SUCCESS) {
        std::string errorMsg;
        controller.getLastError(errorMsg);
        std::cerr << "初始化失败: " << errorMsg << std::endl;
        return 1;
    }
    
    std::cout << "初始化成功，开始实时控制..." << std::endl;
    
    result = controller.start();
    if (result != SUCCESS) {
        std::string errorMsg;
        controller.getLastError(errorMsg);
        std::cerr << "启动失败: " << errorMsg << std::endl;
        return 2;
    }
    
    // 主循环，监听键盘输入以退出
    std::cout << "按下回车键退出程序..." << std::endl;
    std::cin.get();
    
    std::cout << "正在停止控制..." << std::endl;
    controller.stop();
    
    std::cout << "程序已退出" << std::endl;
    return 0;
}