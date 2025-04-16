// main.cpp
#include "./WacomInkling.hpp"
#include <iostream>
#include <thread>
#include <atomic>

int main() {
    std::cout << "Wacom Inkling & ClearPath-SC 测试..." << std::endl;
    
    // === 线程控制变量 ===
    std::atomic<bool> motorRunning{false};
    std::thread motorThread;
    
    // === 第一部分：初始化 ===
    // 1. Inkling初始化（内部已有线程管理）
    WacomInkling inkling;
    if (!inkling.initialize()) {
        std::cerr << "Inkling初始化失败: " << inkling.getLastError() << std::endl;
        return 1;
    }
    
    // 2. 电机初始化
    /*
    PSEUDO_CODE: 电机初始化
    {
        ...电机初始化代码...
    }
    */
    
    // === 第二部分：启动线程 ===
    // 1. 启动Inkling线程（内部管理）
    inkling.start();
    
    // 2. 启动电机控制线程
    motorRunning = true;
    motorThread = std::thread([&]() {
        InklingData data;
        while (motorRunning) {
            if (inkling.getLatestData(data)) {
                /*
                PSEUDO_CODE: 电机控制循环
                {
                    - 获取最新的Inkling数据
                    - 计算电机目标位置
                    - 执行运动控制
                    - 错误处理
                }
                */
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    
    // === 第三部分：主线程（用于监控和用户交互）===
    std::cout << "按回车键退出..." << std::endl;
    std::cin.get();
    
    // === 第四部分：清理退出 ===
    // 1. 停止电机控制线程
    motorRunning = false;
    if (motorThread.joinable()) {
        motorThread.join();
    }
    
    // 2. 停止Inkling线程（内部管理）
    inkling.stop();
    
    /*
    PSEUDO_CODE: 电机清理
    {
        - 停止电机运动
        - 禁用节点
        - 关闭端口
    }
    */
    
    std::cout << "程序已退出" << std::endl;
    return 0;
}