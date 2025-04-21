// Required include files
#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include "pubSysCls.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <ncurses.h>  // Add ncurses header
#include "./WacomInkling.hpp"

using namespace sFnd;

// Global variables for motor control
std::atomic<bool> isMoving(false);
std::mutex motorMutex;
std::atomic<bool> inklingRunning(true);
std::atomic<bool> motorThreadRunning(true);
std::thread motorThread;
std::queue<std::pair<int, int>> motorPositionQueue;
std::mutex queueMutex;
std::condition_variable queueCondition;

// ... existing code ...

void motorControlThread(IPort& myPort) {
    while (motorThreadRunning) {
        std::pair<int, int> positions;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, []{ 
                return !motorPositionQueue.empty() || !motorThreadRunning; 
            });
            
            if (!motorThreadRunning) break;
            
            positions = motorPositionQueue.front();
            motorPositionQueue.pop();
        }
        
        // 设置运动参数
        for(int i = 0; i < 2; i++) {
            INode& node = myPort.Nodes(i);
            node.AccUnit(INode::RPM_PER_SEC);
            node.VelUnit(INode::RPM);
            node.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
            node.Motion.VelLimit = VEL_LIM_RPM;
        }
        
        // 执行运动
        if(positions.first <= 45000 && positions.first >= 0) {
            myPort.Nodes(0).Motion.MovePosnStart(positions.first, true);
        }
        if(positions.second <= 45000 && positions.second >= 0) {
            myPort.Nodes(1).Motion.MovePosnStart(positions.second, true);
        }
    }
}

void inklingDataThread(WacomInkling& inkling) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    while (inklingRunning) {
        auto loop_start = std::chrono::high_resolution_clock::now();
        
        InklingData data;
        if (inkling.getData(data)) {
            // 计算目标位置
            int motorPositionX = static_cast<int>((TARGET_POSITION_X - data.x) * (386.0/45.0));
            int motorPositionY = static_cast<int>((TARGET_POSITION_Y - data.y) * (58.0/9.0));
            
            // 将位置数据放入队列
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                motorPositionQueue.push({motorPositionX, motorPositionY});
            }
            queueCondition.notify_one();
            
            // Clear the screen
            clear();
            
            // Print header
            mvprintw(0, 0, "Wacom Inkling Position Monitor (Press 'q' to quit)");
            mvprintw(1, 0, "------------------------------------------------");
            
            // Print data
            auto current_time = std::chrono::system_clock::now();
            auto time_since_epoch = current_time.time_since_epoch();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
            mvprintw(3, 0, "Time: %lld ms", milliseconds);
            mvprintw(4, 0, "Inkling Position: X=%d, Y=%d, Pressed=%d", data.x, data.y, data.pressed);
            mvprintw(5, 0, "Target Motor Position: X=%d, Y=%d", motorPositionX, motorPositionY);
            
            // Print execution time
            auto loop_end = std::chrono::high_resolution_clock::now();
            auto loop_duration = std::chrono::duration_cast<std::chrono::microseconds>(loop_end - loop_start);
            mvprintw(7, 0, "Loop execution time: %lld us", loop_duration.count());
            
            // Refresh the screen
            refresh();
            
            // Check for 'q' key press
            int ch = getch();
            if (ch == 'q' || ch == 'Q') {
                inklingRunning = false;
                motorThreadRunning = false;
                queueCondition.notify_one();
                break;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Clean up ncurses
    endwin();
}

int main(int argc, char *argv[])
{
    // ... existing initialization code ...

    // Initialize Wacom Inkling
    WacomInkling inkling;
    
    printf("Attempting to release any existing Inkling connection...\n");
    inkling.release(); 
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Check if device is connected and initialize
    printf("Checking device connection...\n");
    if (!inkling.initialize()) {
        printf("Failed to initialize Wacom Inkling: %s\n", inkling.getLastError().c_str());
        printf("Please check:\n");
        printf("1. Device is properly connected via USB\n");
        printf("2. USB permissions are correctly set\n");
        printf("3. No other application is using the device\n");
        return -1;
    }
    
    printf("Wacom Inkling initialized successfully\n");
    printf("Starting data acquisition...\n");
    inkling.start();
    printf("Data acquisition started\n");
    printf("Device is ready to track position\n");
    printf("Press 'q' to quit\n\n");

    // Create motor control thread
    motorThread = std::thread(motorControlThread, std::ref(myPort));

    // Start inkling data thread
    std::thread inklingThread(inklingDataThread, std::ref(inkling));

    // Wait for threads to finish
    inklingThread.join();
    if (motorThread.joinable()) {
        motorThread.join();
    }

    // Cleanup
    printf("\nCleaning up...\n");
    inkling.stop();
    inkling.release();
    printf("Cleanup completed\n");

    // ... rest of the existing code ...
} 