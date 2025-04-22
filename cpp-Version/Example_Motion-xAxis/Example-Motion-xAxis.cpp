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
#include <cstdlib>  // Add for system() call

using namespace sFnd;

// Global variables for motor control
std::atomic<bool> isMoving(false);
std::mutex motorMutex;
std::atomic<bool> inklingRunning(true);

// Atomic struct for thread-safe access
std::atomic<InklingState> latestInklingState{InklingState{0, 0, false, 0}};
std::atomic<long long> motorLoopTime(0);

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

#define ACC_LIM_RPM_PER_SEC 10000
#define VEL_LIM_RPM 1000
#define MOVE_DISTANCE_CNTS 5000
#define NUM_MOVES 100
#define TIME_TILL_TIMEOUT 10000 // The timeout used for homing(ms)

void moveMotor(INode& theNode, int position) {
	auto start_time = std::chrono::high_resolution_clock::now();
	std::lock_guard<std::mutex> lock(motorMutex);
	
	theNode.AccUnit(INode::RPM_PER_SEC);
	theNode.VelUnit(INode::RPM);
	theNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	theNode.Motion.VelLimit = VEL_LIM_RPM;
	
	theNode.Motion.MovePosnStart(position, true);
	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

	motorLoopTime.store(duration.count());
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
		printf("│   Current Position: n/a                  │\n");
		printf("│   Target Position: %-8d                  │\n", latestInklingState.load().x * 10);
		printf("├─────────────────────────────────────────────┤\n");
		printf("│ Performance Metrics:                        │\n");
		printf("│   Inkling Loop Time: %-6lld us             │\n", latestInklingState.load().loopTime);
		printf("│   Motor Loop Time: %-6lld us             │\n", motorLoopTime.load());
		printf("└─────────────────────────────────────────────┘\n");
		
		std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
	}
}

void motorControlThread(IPort& myPort) {
	while (inklingRunning) {
		InklingState currentState = latestInklingState.load();
		int motorPositionX = currentState.x * 10;
		int motorPositionY = currentState.y * 10;

		// 新实现：直接在当前线程中调用 moveMotor
		moveMotor(myPort.Nodes(0), motorPositionX);
		moveMotor(myPort.Nodes(1), motorPositionY);
		/* 原实现：每次循环创建新线程
		std::thread motorThread([&myPort, motorPosition]() {
			moveMotor(myPort.Nodes(0), motorPosition);
		});
		motorThread.detach();
		*/
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
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

			///////////////////////////////////////////////////////////////////////////////////////
			// At this point we will execute moves based on user input
			//////////////////////////////////////////////////////////////////////////////////////

			// Initialize Wacom Inkling
			WacomInkling inkling;
			printf("Attempting to Initialize Wacom Inkling device...\n");
			if (inkling.initialize()) {
				if (!inkling.reset()) {
					printf("Failed to reset Wacom Inkling: %s\n", inkling.getLastError().c_str());
					printf("Please check:\n");
					printf("1. Device is properly connected via USB\n");
					printf("2. USB permissions are correctly set\n");
					printf("3. No other application is using the device\n");
					printf("4. Try unplugging and replugging the device\n");
					return -1;
				}
			}
			
			printf("Wacom Inkling reset successfully\n");
			printf("Starting data acquisition...\n");
			inkling.start();
			printf("Data acquisition started\n");
			printf("Device is ready to track position\n");
			printf("Press 'q' to quit\n\n");

			// Start all threads
			std::thread inklingThread(inklingDataThread, std::ref(inkling));
			std::thread motorThread(motorControlThread, std::ref(myPort));
			std::thread displayThread(displayDataThread);

			// Main loop
			while (true) {
				printf("\nPress 'q' to quit: ");
				std::string input;
				std::getline(std::cin, input);

				if (input == "q" || input == "Q") {
					inklingRunning = false;  // Signal all threads to stop
					break;
				}
			}

			// Cleanup
			printf("\nCleaning up...\n");
			inklingThread.join();
			motorThread.join();
			displayThread.join();
			inkling.stop();
			inkling.release();  // Release USB device before exiting
			printf("Cleanup completed\n");


			//////////////////////////////////////////////////////////////////////////////////////////////
			// After moves have completed Disable node, and close ports
			//////////////////////////////////////////////////////////////////////////////////////////////
			printf("Disabling nodes, and closing port\n");
			// Disable Nodes

			// for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
			// 	// Create a shortcut reference for a node
			// 	myPort.Nodes(iNode).EnableReq(false);
			// }
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
