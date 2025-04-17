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
int accelerationLimit = 900000; // pulses per sec^2

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

void loop() {

  ListenSerialPort();

  if (serialReadString == "homing\n") {
    //Serial.println(serialReadString);
    //Serial.write("ok\n");
    HomingMotors();
    Serial.println("Motor Homing is Finished");
    serialReadString = "";
    Serial.read();
    //delay(500);
  }
}


/*-----------------------------------------------------------------------------------------
  HOMING MOTOR CODE BLOCK
  -----------------------------------------------------------------------------------------*/

void HomingMotors() {
  //Disable Motor first, and the Enable the motor again.
  if (XACTIVE) {
    xMoveTo = 0;
    xCommandPosition = 0;
    motorXAxis.EnableRequest(false);
    delay(50);
    motorXAxis.EnableRequest(true);
    //MoveXAbsolutePosition(xCommandPosition);
    delay(50);
    Serial.println("Waiting for HLFB...");
    while (motorXAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
      continue;
    }
    motorXAxis.ClearAlerts();
    Serial.println("motorXAxis Enabled after Hominmg");
    delay(1);
  }

  if (YACTIVE) {
    yMoveTo = 0;
    yCommandPosition = 0;
    motorYAxis.EnableRequest(false);
    delay(50);
    motorYAxis.EnableRequest(true);
    //MoveYAbsolutePosition(yCommandPosition);
    delay(50);
    Serial.println("Waiting for HLFB...");
    while (motorYAxis.HlfbState() != MotorDriver::HLFB_ASSERTED) {
      continue;
    }
    motorYAxis.ClearAlerts();
    Serial.println("motorYAxis Enabled after Hominmg");
    delay(1);
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
      break;
    }
  }
}

/*-----------------------------------------------------------------------------------------
  Initalize Motors
  -----------------------------------------------------------------------------------------*/

void InitializeMotor() {
  if (XACTIVE) {
    InitializeMotorXAxis();
  }
  if (YACTIVE) {
    InitializeMotorYAxis();
  }
}


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
