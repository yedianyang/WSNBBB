#include <AccelStepper.h>
#include <MultiStepper.h>

AccelStepper stepper1(1, 2, 5);
AccelStepper stepper2(1, 3, 4);

MultiStepper steppers;

#define X_ENDSTOP 8
#define Y_ENDSTOP 9

float X_STEP_PER_MM = 20;
float Y_STEP_PER_MM = 26.66;
float xMoveTo;
float yMoveTo;

int lastValue = 0;
int last_loop_time = 0;
String serialReadString;
String serialData;
// Status of the homing
bool Xhomed = false;
bool Yhomed = false;

void setup() {

  //init serial.port
  Serial.begin(115200);
  while (!Serial);
  Serial.setTimeout(250);

  //INIT stepper motorX
  // 1000 step = 50 mm
  // 1 step = 0.05 mm
  // 20step = 1 mm
  // 1/8 microstep
  stepper1.setMaxSpeed(5000.0);
  //stepper1.setAcceleration(1000.0);

  //INIT stepper motorY
  // 1000 step = 38 mm
  // 1 step = 0.0375 mm
  // 26.3step = 1 mm
  // 1/8 microstep
  stepper2.setMaxSpeed(5000.0);
  //stepper2.setAcceleration(1000.0);

  //  steppers.addStepper(stepper1);
  //  steppers.addStepper(stepper2);

  pinMode(X_ENDSTOP, INPUT);
  pinMode(Y_ENDSTOP, INPUT);
}

void loop() {

  //Read char from serial, and cover into string.
  while (Serial.available() > 0) {
    char c = Serial.read();
    serialReadString += c;
    //delay(2);
    if (c == '\n') {
      break;
    }
  }

  if (serialReadString == "homing\n") {
    //homing the platform
    homingAllAxis();

    serialReadString = "";
    Serial.write("Homing is finished");
    Serial.read();
  }

  //Reading the coming serail data for movement command
  if (serialReadString[0] == 'X') {
    int commaIndex = serialReadString.indexOf(",");
    //Serial.println(commaIndex);

    xMoveTo = serialReadString.substring(1, commaIndex).toFloat();
    yMoveTo = serialReadString.substring(commaIndex + 2).toFloat();

    serialReadString = "";
    Serial.write("ok\n");
    Serial.read();
  }

  stepper1.moveTo(xMoveTo * X_STEP_PER_MM);
  stepper1.setSpeed(2000);
  stepper1.runSpeedToPosition();

  stepper2.moveTo(yMoveTo * Y_STEP_PER_MM);
  stepper2.setSpeed(2666);
  stepper2.runSpeedToPosition();

  //Update realtime location to raspberrypi through serial port
  if (millis() - last_loop_time > 80) {
    Serial.print("Current_PosX:");
    Serial.print(stepper1.currentPosition());
    Serial.print(":Y:");
    Serial.print(stepper2.currentPosition());
    Serial.print("\r\n");
    last_loop_time = millis();
  }
}


//The homing function
void homingAllAxis() {

  int xInitHoming = 1;
  int yInitHoming = 1;

  while (!digitalRead(X_ENDSTOP)) {
    stepper1.moveTo(xInitHoming);
    stepper1.setSpeed(200);
    xInitHoming -= 1;
    stepper1.runSpeedToPosition();

    if (millis() - last_loop_time > 80) {
      Serial.print("Current_PosX:");
      Serial.print(stepper1.currentPosition());
      Serial.print(":Y:");
      Serial.print(stepper2.currentPosition());
      Serial.print("\r\n");

      Serial.print("X ENDSTOP  ");
      Serial.println(digitalRead(X_ENDSTOP));

      last_loop_time = millis();
    }
  }

  stepper1.setCurrentPosition(0);
  Serial.print("X is homed    ");
  Serial.print("Current_PosX:");
  Serial.print(stepper1.currentPosition());


  while (!digitalRead(Y_ENDSTOP)) {
    stepper2.moveTo(yInitHoming);
    stepper2.setSpeed(266);
    yInitHoming -= 1;
    stepper2.runSpeedToPosition();

    if (millis() - last_loop_time > 80) {
      Serial.print("Current_PosY:");
      Serial.print(stepper2.currentPosition());
      Serial.print(":Y:");
      Serial.print(stepper2.currentPosition());
      Serial.print("\r\n");

      Serial.print("Y ENDSTOP  ");
      Serial.println(digitalRead(Y_ENDSTOP));

      last_loop_time = millis();
    }
  }

  stepper2.setCurrentPosition(0);
  Serial.print("Y is homed    ");
  Serial.print("Current_PosY:");
  Serial.println(stepper2.currentPosition());
  xInitHoming = 1;
  yInitHoming = 1;
  xMoveTo = 0;
  yMoveTo = 0;

  Serial.write("ok\n");
}
