#include "ClearCore.h"
#include <MotorDriver.h>

// Specifies which motor to move.
// Options are: ConnectorM0, ConnectorM1, ConnectorM2, or ConnectorM3.
#define motorXAxis ConnectorM0
#define motorYAxis ConnectorM1
#define xAxisZeroEnd IO1
#define xAxisMaxEnd IO0


// Select the baud rate to match the target serial device
#define baudRate 115200

// Define the velocity and acceleration limits to be used for each move
int velocityLimit = 2000; // pulses per sec
int accelerationLimit = 60000; // pulses per sec^2

bool XACTIVE = true;

bool YACTIVE = true;

bool MoveAbsolutePosition(int32_t position);

float xMoveTo = 0;
float yMoveTo = 0;
int xCommandPosition;
int yCommandPosition;

String serialReadString = "";


void setup() {
  MotorMgr.MotorInputClocking(MotorManager::CLOCK_RATE_NORMAL);

  // Sets all motor connectors into step and direction mode.
  MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,
                        Connector::CPM_MODE_STEP_AND_DIR);

  // Sets up serial communication and waits up to 5 seconds for a port to open.
  // Serial communication is not required for this example to run.
  Serial.begin(baudRate);
  uint32_t timeout = 5000;
  uint32_t startTime = millis();
  while (!Serial && millis() - startTime < timeout) {
    continue;
  }

  InitializeMotor();
  Serial.println("Motor Ready");
}

long last_loop_thread1;

void loop() {

  ListenSerialPort();



  if (millis() - last_loop_thread1 > 1) {

    if (serialReadString[0] == 'X') {
      int commaIndex = serialReadString.indexOf(',');
      xMoveTo = serialReadString.substring(1, commaIndex).toFloat();
      yMoveTo = serialReadString.substring(commaIndex + 2).toFloat();

      xCommandPosition = linerMapper_MMToCount(xMoveTo);
      yCommandPosition = linerMapper_MMToCount(yMoveTo);


      Serial.write("ok\n");
      serialReadString = "";
      Serial.read();
    } else {
      serialReadString = "";
      Serial.read();
    }

    //Move Command
    MoveXAbsolutePosition(xCommandPosition);
    xCommandPosition = 0;
    MoveYAbsolutePosition(yCommandPosition);
    yCommandPosition = 0;
    last_loop_thread1 = millis();

  }
  //Serial.read();

  if (serialReadString == "homing\n") {
    //Serial.println(serialReadString);
    //Serial.write("ok\n");
    HomingMotors();
    Serial.println("Motor Homing is Finished");
    serialReadString = "";
    Serial.read();
    delay(500);
  }
}

/*-----------------------------------------------------------------------------------------
  INITIALIZE MOTOR BLOCK
  -----------------------------------------------------------------------------------------*/
void InitializeMotorXAxis() {
  // Set the motor's HLFB mode to bipolar PWM
  motorXAxis.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  // Set the HFLB carrier frequency to 482 Hz
  motorXAxis.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

  // Sets the maximum velocity for each move
  motorXAxis.VelMax(velocityLimit);

  // Set the maximum acceleration for each move
  motorXAxis.AccelMax(accelerationLimit);

  // Enables the motor; homing will begin automatically if enabled
  motorXAxis.EnableRequest(true);
  Serial.println("Motor Enabled");

  // Waits for HLFB to assert (waits for homing to complete if applicable)
  Serial.println("Waiting for HLFB...");
  while (motorXAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
    continue;
  }
  Serial.println("motorXAxis Ready");
}

void InitializeMotorYAxis() {
  //Set the motorYAxis's HLFB mode to bipolar PWM
  motorYAxis.HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
  // Set the HFLB carrier frequency to 482 Hz
  motorYAxis.HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);

  // Sets the maximum velocity for each move
  motorYAxis.VelMax(velocityLimit);

  // Set the maximum acceleration for each move
  motorYAxis.AccelMax(accelerationLimit);

  //Enables the motorYAxis; homing will begin automatically
  motorYAxis.EnableRequest(true);
  Serial.println("motorYAxis Enabled");

  // Waits for HLFB to assert (waits for homing to complete if applicable)
  Serial.println("Waiting for HLFB...");
  while (motorYAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
    continue;
  }
  Serial.println("motorYAxis Ready");
}

/*-----------------------------------------------------------------------------------------
  HOMING MOTOR CODE BLOCK
  -----------------------------------------------------------------------------------------*/

void HomingMotors() {
  //Disable Motor first, and the Enable the motor again.
  if (XACTIVE) {
    xMoveTo = 0;
    xCommandPosition = 0;
    delay(5);
    motorXAxis.EnableRequest(false);
    delay(50);
    MoveXAbsolutePosition(0);
    motorXAxis.EnableRequest(true);
    delay(5);
    Serial.println("Waiting for X HLFB... in Homing");
    while (motorXAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
      //Serial.println("Debug, X Axis HLFB jam...");
      continue;
    }
    motorXAxis.ClearAlerts();
    Serial.println("motorXAxis Enabled after Hominmg");

    delay(5);
  }

  if (YACTIVE) {
    yMoveTo = 0;
    yCommandPosition = 0;
    delay(5);
    motorYAxis.EnableRequest(false);
    delay(50);
    MoveYAbsolutePosition(0);
    motorYAxis.EnableRequest(true);
    delay(5);
    Serial.println("Waiting for Y HLFB... in Homing");
    while (motorYAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
      //Serial.println("Debug, Y Axis HLFB jam...");
      continue;
    }
    Serial.println("motorYAxis Enabled after Hominmg");
    motorYAxis.ClearAlerts();

    delay(5);
  }
  Serial.write("ok\n");
}
/*-----------------------------------------------------------------------------------------
  Listen Serial Port CODE BLOCK
  -----------------------------------------------------------------------------------------*/

void ListenSerialPort() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    serialReadString += c;
    if (c == '\n') {
      //Serial.println("Debug, Data In finished");
      break;
    }
  }
}

void InitializeMotor() {
  if (XACTIVE) {
    InitializeMotorXAxis();
  }
  if (YACTIVE) {
    InitializeMotorYAxis();
  }
}


/*------------------------------------------------------------------------------
   MoveAbsolutePosition

      Command step pulses to move the motor's current position to the absolute
      position specified by "position"
      Prints the move status to the USB serial port
      Returns when HLFB asserts (indicating the motor has reached the commanded
      position)

   Parameters:
      int position  - The absolute position, in step pulses, to move to

   Returns: True/False depending on whether the move was successfully triggered.
*/

bool MoveYAbsolutePosition(int position) {
  // Check if an alert is currently preventing motion
  if (motorYAxis.StatusReg().bit.AlertsPresent) {
    //Serial.println(motorYAxis.StatusReg().bit.AlertsPresent);
    Serial.println("Motor Y status: 'In Alert'. Move Canceled.");
    return false;
  }

  //  Serial.print("Moving Y Axis to absolute position: ");
  //  Serial.println(_position);
  // Command the move of absolute distance
  motorYAxis.Move(position);

  // Waits for HLFB to assert (signaling the move has successfully completed)
  //  Serial.println("Moving.. Waiting for HLFB");
  //  while (!motorYAxis.StepsComplete() || motorYAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
  //    continue;
  //  }
  //Serial.println("Move Done");
  return true;
}

bool MoveXAbsolutePosition(int position) {
  // Check if an alert is currently preventing motion
  if (motorXAxis.StatusReg().bit.AlertsPresent) {
    Serial.println("Motor X status: 'In Alert'. Move Canceled.");
    return false;
  }

  //  Serial.print("Moving X Axis to absolute position: ");
  //  Serial.println(_position);

  // Command the move of absolute distance
  motorXAxis.Move(position);

  // Waits for HLFB to assert (signaling the move has successfully completed)
  //  Serial.println("Moving.. Waiting for HLFB");
  //  while (!motorYAxis.StepsComplete() || motorYAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
  //    continue;
  //  }
  //Serial.println("Move Done");
  return true;
}

int linerMapper_MMToCount(float mm) {
  //800count / resolution, 75mm/resolution
  return (mm * 32 / 3);
}

//------------------------------------------------------------------------------
