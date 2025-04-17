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
#include "MotorAxis.h"

using namespace sFnd;

// Global variables for motor control
std::atomic<bool> isMoving(false);
std::mutex motorMutex;

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
	std::lock_guard<std::mutex> lock(motorMutex);
	
	theNode.AccUnit(INode::RPM_PER_SEC);
	theNode.VelUnit(INode::RPM);
	theNode.Motion.AccLimit = ACC_LIM_RPM_PER_SEC;
	theNode.Motion.VelLimit = VEL_LIM_RPM;

	printf("Moving Node to position: %d\n", position);
	theNode.Motion.MovePosnStart(position, true);
	
	// Don't wait for move to complete, just start the move and return
	// The next move command will interrupt this one if needed
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

				// 创建MotorAxis实例
				MotorAxis motorAxis(theNode, *myMgr);
				
				// 初始化电机
				if (!motorAxis.Initialize()) {
					printf("Failed to initialize motor\n");
					continue;
				}
				
				// 执行归零
				if (!motorAxis.Home()) {
					printf("Failed to home motor\n");
					continue;
				}

				// 主循环
				while (true) {
					printf("\nEnter position (or 'q' to quit): ");
					std::string input;
					std::getline(std::cin, input);

					if (input == "q" || input == "Q") {
						break;
					}

					try {
						int position = std::stoi(input) * 1000;
						
						// 使用MotorAxis类控制运动
						std::thread motorThread([&motorAxis, position]() {
							motorAxis.MoveToPosition(position);
						});
						motorThread.detach();
					}
					catch (const std::invalid_argument&) {
						printf("Invalid input. Please enter a valid number.\n");
					}
					catch (const std::out_of_range&) {
						printf("Input number is too large. Please enter a smaller number.\n");
					}
				}
			}
		}
	}
	catch (mnErr &theErr)
	{
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n", 
			   theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		msgUser("Press any key to continue.");
		return 0;
	}

	// Close down the ports
	myMgr->PortsClose();

	msgUser("Press any key to continue."); // pause so the user can see the error message; waits for user to press a key
	return 0;							   // End program
}

