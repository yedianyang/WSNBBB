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
#include "./constants.hpp"
#include "./json2Route.hpp"
#include "./WacomInkling.hpp" // Add WacomInkling header
#include "./filter.hpp"
#include <cstdlib> // Add for system() call

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

// Global condition variable and mutex for inkling state
std::condition_variable inklingStateCV;
std::mutex inklingStateMutx;

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
void clearScreen()
{
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

void moveMotor(INode &theNode, int position)
{
	// 这里的锁没有实际作用，因为moveMotor函数内部没有共享资源需要保护

	theNode.AccUnit(INode::RPM_PER_SEC);
	theNode.VelUnit(INode::RPM);
	theNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	theNode.Motion.VelLimit = VEL_LIM_RPM;

	theNode.Motion.MovePosnStart(position, true);
}

void inklingDataThread(WacomInkling &inkling)
{
	while (inklingRunning)
	{
		auto loop_start = std::chrono::high_resolution_clock::now();

		InklingData data;
		if (inkling.getData(data))
		{
			// Store latest data
			latestInklingState.store(InklingState{data.x, data.y, data.pressed,
												  std::chrono::duration_cast<std::chrono::microseconds>(
													  std::chrono::high_resolution_clock::now() - loop_start)
													  .count()});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void displayDataThread()
{
	while (inklingRunning)
	{
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
		printf("│   Current X Position: %-2d cnt                │\n", currentXPosition.load());
		printf("│   Current Y Position: %-2d cnt                │\n", currentYPosition.load());
		printf("│   Error X Position: %-2d cnt              │\n", errorXposition.load());
		printf("│   Error Y Position: %-2d cnt              │\n", errorYposition.load());
		printf("│   Target X Position: %-2d mm             │\n", targetXposition.load());
		printf("│   Target Y Position: %-2d mm             │\n", targetYposition.load()); //(latestInklingState.load().y - targetYposition.load() ) * 58/9 + currentXPosition.load());
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

void motorControlThreadX(IPort &myPort)
{
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

	while (inklingRunning)
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		// InklingState currentState = latestInklingState.load();

		int currentInklingX = latestInklingState.load().x;
		int curMotorXPosition = currentXPosition.load();
		int tarXPosition = targetXposition.load() * 1920 / 193;

		int motorGoToPositionX = (currentInklingX - tarXPosition) * 386 / 45 + curMotorXPosition;

		motorGoToPositionX = adaptiveLowPassFilter(curMotorXPosition, motorGoToPositionX, 0.5f);

		if (curMotorXPosition >= 0 && curMotorXPosition <= 46000)
		{
			// if(motorPositionX >= 0 && motorPositionX <= 45000) {
			moveMotor(myPort.Nodes(0), motorGoToPositionX);
			//}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

		motorXLoopTime.store(duration.count());
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	moveMotor(myPort.Nodes(0), 23000);
}

void motorControlThreadY(IPort &myPort)
{
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
	while (inklingRunning)
	{
		auto start_time = std::chrono::high_resolution_clock::now();

		int currentInklingY = latestInklingState.load().y;
		int curMotorYPosition = currentYPosition.load();
		int tarYPosition = targetYposition.load() * 1920 / 145;

		int motorGoToPositionY = (currentInklingY - tarYPosition) * 58 / 9 + curMotorYPosition;

		motorGoToPositionY = adaptiveLowPassFilter(curMotorYPosition, motorGoToPositionY, 0.5f);

		// printf("motorGoToPositionY: %d\n", motorGoToPositionY);

		if (curMotorYPosition >= 0 && curMotorYPosition <= 45000)
		{
			// if(motorPositionY >= 0 && motorPositionY <= 45000) {
			moveMotor(myPort.Nodes(1), motorGoToPositionY);
			//}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

		motorYLoopTime.store(duration.count());
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	moveMotor(myPort.Nodes(1), 23000);
}

void motorPositionDataThread(IPort &myPort)
{
	while (inklingRunning)
	{
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

void inklingTargetPositionDataThread(std::vector<Command> commands) {
	/**
	 * 备用实现：使用索引跟踪命令执行进度
	 * 增加了更详细的状态处理和错误检查
	 */
	
    for (size_t i = 0; i < commands.size() && inklingRunning; i++) {
        const auto &command = commands[i];
        
        // 更新目标位置
        targetXposition.store(command.x);
        targetYposition.store(command.y);
        std::cout << "Target position set to: (" << command.x << ", " << command.y << ")" << std::endl;

        // 检查笔的状态
        if (latestInklingState.load().pressed) {
            // 如果笔被按下，等待1秒后继续下一个命令
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Command " << i + 1 << "/" << commands.size() << " processed" << std::endl;
        } else {
            // 如果笔没有被按下，等待一小段时间后重新检查
            std::this_thread::sleep_for(std::chrono::seconds(1));
            i--;  // 重试当前命令
        }
    }

    
    std::cout << "All commands processed" << std::endl;
	inklingRunning = false;
}

int main(int argc, char *argv[])
{
	msgUser("Motion Example starting. Press Enter to continue.");

	size_t portCount = 0;
	std::vector<std::string> comHubPorts;

	// Create the SysManager object. This object will coordinate actions among various ports
	//  and within nodes. In this example we use this object to setup and open our port.
	SysManager *myMgr = SysManager::Instance(); // Create System Manager myMgr

	// This will try to open the port. If there is an error/exception during the port opening,
	// the code will jump to the catch loop where detailed information regarding the error will be displayed;
	// otherwise the catch loop is skipped over
	try
	{

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

		for (size_t i = 0; i < portCount; i++)
		{
			IPort &myPort = myMgr->Ports(i);

			printf(" Port[%d]: state=%d, nodes=%d\n",
				   myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

			// Once the code gets past this point, it can be assumed that the Port has been opened without issue
			// Now we can get a reference to our port object which we will use to access the node objects

			for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++)
			{
				// Create a shortcut reference for a node
				INode &theNode = myPort.Nodes(iNode);

				theNode.EnableReq(false); // Ensure Node is disabled before loading config file

				myMgr->Delay(200);

				// theNode.Setup.ConfigLoad("Config File path");

				printf("   Node[%d]: type=%d\n", int(iNode), theNode.Info.NodeType());
				printf("            userID: %s\n", theNode.Info.UserID.Value());
				printf("        FW version: %s\n", theNode.Info.FirmwareVersion.Value());
				printf("          Serial #: %d\n", theNode.Info.SerialNumber.Value());
				printf("             Model: %s\n", theNode.Info.Model.Value());

				// The following statements will attempt to enable the node.  First,
				//  any shutdowns or NodeStops are cleared, finally the node is enabled
				theNode.Status.AlertsClear();	// Clear Alerts on node
				theNode.Motion.NodeStopClear(); // Clear Nodestops on Node
				theNode.EnableReq(true);		// Enable node
				// At this point the node is enabled
				printf("Node \t%zi enabled\n", iNode);
				double timeout = myMgr->TimeStampMsec() + TIME_TILL_TIMEOUT; // define a timeout in case the node is unable to enable
																			 // This will loop checking on the Real time values of the node's Ready status
				while (!theNode.Motion.IsReady())
				{
					if (myMgr->TimeStampMsec() > timeout)
					{
						if (IsBusPowerLow(theNode))
						{
							printf("Error: Bus Power low. Make sure 75V supply is powered on.\n");
							msgUser("Press any key to continue.");
							return -1;
						}
						printf("Error: Timed out waiting for Node %d to enable\n", iNode);
						msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key
						return -2;
					}
				}

				// Enable move interruption capability
				theNode.Info.Ex.Parameter(98, 1);
				theNode.Motion.MoveWentDone();

				// At this point the Node is enabled, and we will now check to see if the Node has been homed
				// Check the Node to see if it has already been homed,
				if (theNode.Motion.Homing.HomingValid())
				{
					if (theNode.Motion.Homing.WasHomed())
					{
						printf("Node %d has already been homed, current position is: \t%8.0f \n", iNode, theNode.Motion.PosnMeasured.Value());
						printf("Rehoming Node... \n");
					}
					else
					{
						printf("Node [%d] has not been homed.  Homing Node now...\n", iNode);
					}
					// Now we will home the Node
					theNode.Motion.Homing.Initiate();

					timeout = myMgr->TimeStampMsec() + TIME_TILL_TIMEOUT; // define a timeout in case the node is unable to enable
																		  //  Basic mode - Poll until disabled
					while (!theNode.Motion.Homing.WasHomed())
					{
						if (myMgr->TimeStampMsec() > timeout)
						{
							if (IsBusPowerLow(theNode))
							{
								printf("Error: Bus Power low. Make sure 75V supply is powered on.\n");
								msgUser("Press any key to continue.");
								return -1;
							}
							printf("Node did not complete homing:  \n\t -Ensure Homing settings have been defined through ClearView. \n\t -Check for alerts/Shutdowns \n\t -Ensure timeout is longer than the longest possible homing move.\n");
							msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key
							return -2;
						}
					}
					printf("Node completed homing\n");
				}
				else
				{
					printf("Node[%d] has not had homing setup through ClearView.  The node will not be homed.\n", iNode);
				}
			}

			moveMotor(myPort.Nodes(0), 23000);
			moveMotor(myPort.Nodes(1), 23000);

			///////////////////////////////////////////////////////////////////////////////////////
			// At this point we will execute moves based on user input
			//////////////////////////////////////////////////////////////////////////////////////

			// Initialize Wacom Inkling
			WacomInkling inkling;
			printf("Attempting to Initialize Wacom Inkling device...\n");
			if (!inkling.initialize())
			{
				printf("Failed to reset Wacom Inkling: %s\n", inkling.getLastError().c_str());
				printf("Please check:\n");
				printf("1. Device is properly connected via USB\n");
				printf("2. USB permissions are correctly set\n");
				printf("3. No other application is using the device\n");
				printf("4. Try unplugging and replugging the device\n");
				return -1;
			}

			printf("Wacom Inkling reset successfully\n");
			printf("Starting data acquisition...\n");
			inkling.start();
			printf("Data acquisition started\n");
			printf("Device is ready to track position\n");

			// 读取json文件

			std::string jsonFilePath = "./Test-Cactus-Pattern.json";
			Json2Route json2Route; // 先创建对象
			if (!json2Route.loadFromFile(jsonFilePath))
			{ // 然后加载文件
				printf("Failed to load json file\n");
				return -1;
			}

			std::vector<Command> commands = json2Route.getAllCommands();


			for (const auto &command : commands)
			{
				// Command 没有 toString 方法，需要使用 printCommand
				printCommand(command);
			}

			printf("Press 'q' to quit\n\n");

			// Start all threads
			std::thread inklingThread(inklingDataThread, std::ref(inkling));
			std::thread inklingTargetPositionThread(inklingTargetPositionDataThread, std::ref(commands));
			std::thread motorPositionThread(motorPositionDataThread, std::ref(myPort));
			std::thread motorThreadX(motorControlThreadX, std::ref(myPort));
			std::thread motorThreadY(motorControlThreadY, std::ref(myPort));
			// std::thread displayThread(displayDataThread);

			// Main loop
			while (true)
			{
				printf("\nPress 'q' to quit: ");
				std::string input;
				std::getline(std::cin, input);

				if (input == "q" || input == "Q")
				{
					inklingRunning = false; // Signal all threads to stop
					break;
				}

				if(inklingRunning == false) {
					break;
				}
			}

			// Cleanup
			printf("\nCleaning up...\n");
			inklingThread.join();
			inklingTargetPositionThread.join();
			motorPositionThread.join();
			motorThreadX.join();
			motorThreadY.join();
			// displayThread.join();
			inkling.stop();
			inkling.release(); // Release USB device before exiting
			printf("Cleanup completed\n");

			//////////////////////////////////////////////////////////////////////////////////////////////
			// After moves have completed Disable node, and close ports
			//////////////////////////////////////////////////////////////////////////////////////////////
			printf("Disabling nodes, and closing port\n");
			// Disable Nodes

			for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
				// Create a shortcut reference for a node
				myPort.Nodes(iNode).EnableReq(false);
			}
		}
	}
	catch (mnErr &theErr)
	{
		printf("Failed to disable Nodes n\n");
		// This statement will print the address of the error, the error code (defined by the mnErr class),
		// as well as the corresponding error message.Inkling Position: X=1, Y=70, Pressed=0
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);

		msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key
		return 0;							   // This terminates the main program
	}

	// Close down the ports
	myMgr->PortsClose();

	msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key
	return 0;							   // End program
}
