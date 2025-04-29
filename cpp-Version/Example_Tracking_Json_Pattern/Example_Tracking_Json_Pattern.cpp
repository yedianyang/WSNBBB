// Required include files
#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h> // Header file for sleep().
#include <chrono>
#include "pubSysCls.h"
#include <thread>
#include <mutex>
#include <atomic>
#include "./WacomInkling.hpp"  // Add WacomInkling header
#include "./json2Route.hpp"
#include <cstdlib>  // Add for system() call
#include "filter.cpp"


using namespace sFnd;

// Global variables for motor control
std::atomic<bool> isMoving(false);
std::mutex motorMutex;
std::atomic<bool> inklingRunning(true);

// Atomic struct for thread-safe access
std::atomic<InklingState> latestInklingState{InklingState{0, 0, false, 0}};
std::atomic<long long> motorXLoopTime(0);
std::atomic<int> currentXPosition(0);
std::atomic<long long> motorYLoopTime(0);
std::atomic<int> currentYPosition(0);
std::atomic<long long> motorDataLoopTime(0);
std::atomic<int> errorXposition(0);
std::atomic<int> errorYposition(0);
std::atomic<int> targetXposition(960);
std::atomic<int> targetYposition(960);

// Send message and wait for newline
void msgUser(const char *msg)
{
	std::cout << msg;
	getchar();
}

// Check if the bus power has been applied.
bool IsBusPowerLow(INode &theNode)
{
	return theNode.Status.Power.Value().fld.InBusLoss;
}

// Function to clear the console screen
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

//*********************************************************************************
// This program will load configuration files onto each node connected to the port, then executes
// sequential repeated moves on each axis.
//*********************************************************************************

#define ACC_LIM_RPM_PER_SEC 5000
#define VEL_LIM_RPM 800
#define TIME_TILL_TIMEOUT 10000 // The timeout used for homing(ms)

void moveMotor(INode& theNode, int position) {
	// 这里的锁没有实际作用，因为moveMotor函数内部没有共享资源需要保护
	
	theNode.AccUnit(INode::RPM_PER_SEC);
	theNode.VelUnit(INode::RPM);
	theNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	theNode.Motion.VelLimit = VEL_LIM_RPM;

	theNode.Motion.MovePosnStart(position, true);
}

void inklingDataThread(WacomInkling& inkling) {
	while (inklingRunning) {
		auto loop_start = std::chrono::high_resolution_clock::now();
		
		InklingData data;
		if (inkling.getData(data)) {
			// Store latest data
			latestInklingState.store(InklingState{data.x, data.y, data.pressed,
				std::chrono::duration_cast<std::chrono::microseconds>(
					std::chrono::high_resolution_clock::now() - loop_start).count()
			});
		}	
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void displayDataThread() {
	while (inklingRunning) {
		// Clear screen and print watch-like display
		clearScreen();
		printf("┌─────────────────────────────────────────────┐\n");
		printf("│              System Status                  │\n");
		printf("├─────────────────────────────────────────────┤\n");
		printf("│ Inkling Position:                           │\n");
		printf("│   X: %-4d  Y: %-4d  Pressed: %d              │\n", 
			   latestInklingState.load().x, 
			   latestInklingState.load().y, 
			   latestInklingState.load().pressed);
		printf("├─────────────────────────────────────────────┤\n");
		printf("│ Motor Status:                               │\n");
		printf("│   Current X Position: %-2d                  │\n", currentXPosition.load());
		printf("│   Current Y Position: %-2d                  │\n", currentYPosition.load());
		printf("│   Error X Position: %-2d                  │\n", errorXposition.load());
		printf("│   Error Y Position: %-2d                  │\n", errorYposition.load());
		printf("│   Target X Position: %-2d                  │\n", 00000);
		printf("│   Target Y Position: %-2d                  │\n", 00000); //(latestInklingState.load().y - targetYposition.load() ) * 58/9 + currentXPosition.load());
		printf("├─────────────────────────────────────────────┤\n");
		printf("│ Performance Metrics:                        │\n");
		printf("│   Inkling Loop Time: %-6lld us             │\n", latestInklingState.load().loopTime);
		printf("│   Motor X Loop Time: %-6lld us             │\n", motorXLoopTime.load());
		printf("│   Motor Y Loop Time: %-6lld us             │\n", motorYLoopTime.load());
		printf("│   Motor Data Loop Time: %-6lld us          │\n", motorDataLoopTime.load());
		printf("└─────────────────────────────────────────────┘\n");
		
		std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~20 FPS
	}
}

void motorControlThreadX(IPort& myPort) {
	/**
	 * X轴电机控制线程函数，基于Inkling输入控制电机运动
	 * 
	 * 该函数在inklingRunning为true时循环运行，主要功能包括：
	 * 1. 获取最新的Inkling状态并计算目标X轴位置
	 * 2. 读取当前X轴电机位置并存储
	 * 3. 如果目标位置在有效范围内（0-45000），则移动电机到目标位置
	 * 4. 跟踪执行时间以监控性能
	 * 
	 * @param myPort 电机控制端口接口的引用
	 */

	while (inklingRunning) {
		auto start_time = std::chrono::high_resolution_clock::now();
		//InklingState currentState = latestInklingState.load();
		
		int currentInklingX = latestInklingState.load().x;
		int curMotorXPosition = currentXPosition.load();
		int tarXPosition = targetXposition.load();

		int motorGoToPositionX = (currentInklingX - tarXPosition) * 386/45 + curMotorXPosition;

		motorGoToPositionX = adaptiveLowPassFilter(curMotorXPosition, motorGoToPositionX, 0.5f);

		if (curMotorXPosition >= 0 && curMotorXPosition <= 46000) {
			//if(motorPositionX >= 0 && motorPositionX <= 45000) {
				moveMotor(myPort.Nodes(0), motorGoToPositionX);
			//}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
		
		
		motorXLoopTime.store(duration.count());
	}
}



void motorControlThreadY(IPort& myPort) {
	/**
	 * Y轴电机控制线程函数，基于Inkling输入控制电机运动
	 * 
	 * 该函数在inklingRunning为true时循环运行，主要功能包括：
	 * 1. 获取最新的Inkling状态并计算目标Y轴位置
	 * 2. 读取当前Y轴电机位置并存储
	 * 3. 如果目标位置在有效范围内（0-45000），则移动电机到目标位置
	 * 4. 跟踪执行时间以监控性能
	 * 
	 * @param myPort 电机控制端口接口的引用
	 */
	while (inklingRunning) {
		auto start_time = std::chrono::high_resolution_clock::now();

		int currentInklingY = latestInklingState.load().y;
		int curMotorYPosition = currentYPosition.load();
		int tarYPosition = targetYposition.load();

		int motorGoToPositionY = (currentInklingY - tarYPosition) * 58/9 + curMotorYPosition;

		motorGoToPositionY = adaptiveLowPassFilter(curMotorYPosition, motorGoToPositionY, 0.5f);

		//printf("motorGoToPositionY: %d\n", motorGoToPositionY);

		if (curMotorYPosition >= 0 && curMotorYPosition <= 45000) {
			//if(motorPositionY >= 0 && motorPositionY <= 45000) {
				moveMotor(myPort.Nodes(1), motorGoToPositionY);
			//}
		}

		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
		
		motorYLoopTime.store(duration.count());
	}
}

void motorPositionDataThread(IPort& myPort) {
    while (inklingRunning) {
		auto start_time = std::chrono::high_resolution_clock::now();

        int curMotorXPosition = int(myPort.Nodes(0).Motion.PosnMeasured.Value());
        int curMotorYPosition = int(myPort.Nodes(1).Motion.PosnMeasured.Value());

		int errMotorXPosition = int(myPort.Nodes(0).Motion.PosnTracking.Value());
		int errMotorYPosition = int(myPort.Nodes(1).Motion.PosnTracking.Value());


		currentXPosition.store(curMotorXPosition);
		currentYPosition.store(curMotorYPosition);
		errorXposition.store(errMotorXPosition);
		errorYposition.store(errMotorYPosition);

        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Adjust the sleep time as needed

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

		motorDataLoopTime.store(duration.count());
    }
}


void inklingTargetPositionDataThread() {
	/**
	 * 更新目标位置
	 * 
	 * 该函数用于更新目标位置，并计算目标位置与当前位置的误差
	 * 
	 */


	int tarXPosition = 960;
	int tarYPosition = 960; 

	targetXposition.store(tarXPosition);
	targetYposition.store(tarYPosition);
}

int main(int argc, char *argv[]) {
    msgUser("Motion Example starting. Press Enter to continue.");

    size_t portCount = 0;
    std::vector<std::string> comHubPorts;
    SysManager *myMgr = SysManager::Instance();

    try {
        // 端口初始化和配置
        SysManager::FindComHubPorts(comHubPorts);
        printf("Found %d SC Hubs\n", comHubPorts.size());

        for (portCount = 0; portCount < comHubPorts.size() && portCount < NET_CONTROLLER_MAX; portCount++)
        {

            myMgr->ComHubPort(portCount, comHubPorts[portCount].c_str()); // define the first SC Hub port (port 0) to be associated
                                                                          //  with COM portnum (as seen in device manager)
        }

        if (portCount < 0)
        {

            printf("Unable to locate SC hub port\n");

            msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key

            return -1; // This terminates the main program
        }
        // printf("\n I will now open port \t%i \n \n", portnum);
        myMgr->PortsOpen(portCount); // Open the port

        // 获取系统支持的线程数
        unsigned int numThreads = std::thread::hardware_concurrency();
        std::cout << "Number of CPU cores: " << numThreads << std::endl;

        // 获取端口引用
        IPort &myPort = myMgr->Ports(0);  // 假设使用第一个端口

        try {
            // Wacom Inkling 初始化
            WacomInkling inkling;
            if (!inkling.initialize()) {
                printf("Failed to reset Wacom Inkling: %s\n", inkling.getLastError().c_str());
                printf("Please check:\n");
                printf("1. Device is properly connected via USB\n");
                printf("2. USB permissions are correctly set\n");
                printf("3. No other application is using the device\n");
                printf("4. Try unplugging and replugging the device\n");
                return -1;
            }
            
            // 设备初始化成功后，创建并启动所有线程
            std::thread inklingThread(inklingDataThread, std::ref(inkling));
            std::thread inklingTargetPositionThread(inklingTargetPositionDataThread);
            std::thread motorPositionThread(motorPositionDataThread, std::ref(myPort));
            std::thread motorThreadX(motorControlThreadX, std::ref(myPort));
            std::thread motorThreadY(motorControlThreadY, std::ref(myPort));
            std::thread displayThread(displayDataThread);

            // 主循环
            while (true) {
                printf("\nPress 'q' to quit: ");
                std::string input;
                std::getline(std::cin, input);

                if (input == "q" || input == "Q") {
                    inklingRunning = false;
                    break;
                }
            }

            // 清理线程
            printf("\nCleaning up...\n");
            inklingThread.join();
            inklingTargetPositionThread.join();
            motorPositionThread.join();
            motorThreadX.join();
            motorThreadY.join();
            displayThread.join();
            
            // 清理设备
            inkling.stop();
            inkling.release();
            
            // 禁用节点
            for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
                myPort.Nodes(iNode).EnableReq(false);
            }

        } catch (const std::exception& e) {
            printf("Error during Inkling operation: %s\n", e.what());
            // 确保在异常情况下也能正确清理资源
            for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
                myPort.Nodes(iNode).EnableReq(false);
            }
        }

    } catch (mnErr &theErr) {
        printf("Failed to disable Nodes\n");
        printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", 
               theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
        msgUser("Press any key to continue.");
        return 0;
    }

    // 关闭端口
    myMgr->PortsClose();
    msgUser("Press any key to continue.");
    return 0;
}
