#include <AccelStepper.h>
#include <MultiStepper.h>
#include "AS5600.h"
#include "Wire.h"

#define X_ENDSTOP 7
AccelStepper stepper1(1, 2, 3);
AS5600 as5600X;



double STEP = 4;
int MOTOR_SPEED = 5000;

double MM_PER_STEP = 0.0488 * (8 / STEP);
double MM_PER_ENCD_RES = 0.0191; // Encoder resolution
double xMoveTo = 0;
float motorCuttentPosition = 0;
float lastxMoveTo = 0;

float motorChangableSpeed = MOTOR_SPEED;

/*
   Threading Parameter
*/

int lastValue = 0;
int last_loop_time = 0;
int last_loop_thread1 = 0;
int last_loop_thread2 = 0;


String serialReadString;

bool serialPackComplete = false;

void setup() {

  //Init serial.port
  Serial.begin(115200);
  while (!Serial);
  Serial.setTimeout(250);

  //Init Zero End  Sensor
  pinMode(X_ENDSTOP, INPUT);

  //Init stepper motorX
  // 1000 step = 50 mm
  // 1 step = 0.05 mm
  // 20step = 1 mm
  // 1/8 microstep
  stepper1.setMaxSpeed(10000.0);
  stepper1.setAcceleration(5000.0);
  Serial.println("Motor is initialed");

  //Init Encoder
  as5600X.begin(6);  //  set direction pin.
  //as5600X.setDirection(AS5600_COUNTERCLOCK_WISE);  // default, just be explicit.
  as5600X.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.
  int b = as5600X.isConnected();
  Serial.print("Connect: ");
  Serial.println(b);

  //  set initial position
  as5600X.resetCumulativePosition(0);

}

void loop() {
  ListenSerialPort();
  //Serial.println(MM_PER_STEP);
  //Serial.println(xMoveTo / MM_PER_STEP);
  if (serialReadString == "homing\n") {
    //homing the platform
    Homeming();
  }


  //  Read Position Data from the AS5600
  //
  if (millis() - last_loop_thread1 >= 40)
  {
    motorCuttentPosition = as5600X.getCumulativePosition() * MM_PER_ENCD_RES;
    Serial.print("cX:");
    Serial.print(motorCuttentPosition);
    Serial.print(":cXr:");
    Serial.println(as5600X.getRevolutions());
    last_loop_thread1 = millis();
  }

  if (millis() - last_loop_thread2 > 40) {
    last_loop_thread2 = millis();
    //Reading the coming serail data for movement command
    if (serialReadString[0] == 'X') {
      int commaIndex = serialReadString.indexOf(",");
      //Serial.println(commaIndex);

      xMoveTo = serialReadString.substring(1, commaIndex).toFloat();

      serialReadString = "";
      Serial.write("ok\n");
      Serial.read();

    } else {
      serialReadString = "";
      //Serial.write("ok\n");
      Serial.read();
    }
  }
  stepper1.moveTo((int)(xMoveTo / MM_PER_STEP));
  stepper1.setSpeed(10000);
  stepper1.runSpeedToPosition();
  lastxMoveTo = xMoveTo;
}


// Function Definition
void Homeming() {
  //MotorHoming();
  stepper1.setCurrentPosition(0);
  xMoveTo = 0;
  SensorHomeing();

  Serial.write("Homing is finished\n");
  // Operation finished, send a finished command to the control board.
  Serial.write("ok\n");
  Serial.read();
}

void MotorHoming() {
  int xInitHoming = 1;
  while (digitalRead(X_ENDSTOP)) {
    stepper1.moveTo(xInitHoming);
    stepper1.setSpeed(800);
    xInitHoming -= 1;
    stepper1.runSpeedToPosition();

    if (millis() - last_loop_time > 40) {
      Serial.print("Current_PosX:");
      Serial.print(stepper1.currentPosition());

      Serial.print("X ENDSTOP  ");
      Serial.println(digitalRead(X_ENDSTOP));

      last_loop_time = millis();
    }
  }

  stepper1.setCurrentPosition(0);
  Serial.print("X is homed    ");
  Serial.print("Current_PosX:");
  Serial.print(stepper1.currentPosition());

  xMoveTo = 0;
}

void SensorHomeing() {
  delay(50);
  as5600X.resetCumulativePosition(0);

}

void ListenSerialPort() {
  /*
    Listen the command from seial port. If with a '\n' end, serial communication package finished.
  */
  while (Serial.available() > 0) {
    char c = Serial.read();
    serialReadString += c;
    //delay(2);
    if (c == '\n') {
      break;
    }
  }
}
