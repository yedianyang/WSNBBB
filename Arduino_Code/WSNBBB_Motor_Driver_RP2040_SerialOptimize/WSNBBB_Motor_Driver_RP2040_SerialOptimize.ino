#include <AccelStepper.h>
#include <MultiStepper.h>

//MultiStepper steppers;

//This define is for ESP32 Board
//AccelStepper stepper1(1, D2, D3);
//AccelStepper stepper2(1, D4, D5);
//#define X_ENDSTOP D7
//#define Y_ENDSTOP D8


AccelStepper stepper1(1, 2, 3);
AccelStepper stepper2(1, 4, 5);

#define X_ENDSTOP 7
#define Y_ENDSTOP 8

int STEP = 8;
int MOTOR_SPEED = 4000;
int chanageble_speed = MOTOR_SPEED;
float X_STEP_PER_MM = 26.666666 * (STEP / 8);
float Y_STEP_PER_MM = 26.666666 * (STEP / 8);
float xMoveTo;
float yMoveTo;
float lastxMoveTo;
float lastyMoveTo;
float motorChangableSpeed = MOTOR_SPEED;

int lastValue = 0;
int last_loop_time = 0;
int last_loop_thread1 = 0;
int last_loop_thread2 = 0;

String serialReadString;
String serialData;
bool serialPackComplete = false;

// Status of the homing
bool Xhomed = false;
bool Yhomed = false;



void setup() {
  //init serial.port
  Serial.begin(115200);
  while (!Serial);
  Serial.setTimeout(250);

  //digitalWrite(ENABLEPIN, LOW);
  pinMode(X_ENDSTOP, INPUT);
  pinMode(Y_ENDSTOP, INPUT);

  //INIT stepper motorX
  // 1000 step = 50 mm
  // 1 step = 0.05 mm
  // 20step = 1 mm
  // 1/8 microstep
  stepper1.setMaxSpeed(10000.0);
  stepper1.setAcceleration(1600.0);

  //INIT stepper motorY
  // 1000 step = 38 mm
  // 1 step = 0.0375 mm
  // 26.3step = 1 mm
  // 1/8 microstep
  stepper2.setMaxSpeed(10000.0);
  stepper2.setAcceleration(1600.0);

  //  steppers.addStepper(stepper1);
  //  steppers.addStepper(stepper2);
}

void loop() {
  float distanceBetweenCurrentAndNew;
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

  if (millis() - last_loop_thread1 > 40) {
    //Reading the coming serail data for movement command
    if (serialReadString[0] == 'X') {
      int commaIndex = serialReadString.indexOf(",");
      //Serial.println(commaIndex);

      xMoveTo = serialReadString.substring(1, commaIndex).toFloat();
      yMoveTo = serialReadString.substring(commaIndex + 2).toFloat();
      distanceBetweenCurrentAndNew = sqrt(sq((stepper1.currentPosition() / X_STEP_PER_MM - xMoveTo)) + sq((stepper2.currentPosition() / Y_STEP_PER_MM - yMoveTo)));

      if (distanceBetweenCurrentAndNew < 20) {
        chanageble_speed = 2000;
      } else {
        if (distanceBetweenCurrentAndNew < 50) {
          chanageble_speed = 3000;
        }
        else {
          if (distanceBetweenCurrentAndNew < 100) {
            chanageble_speed = 4000;
          }
          else {
            chanageble_speed = 5000;
          }
        }
      }
      //chanageble_speed = 2000;
      serialReadString = "";
      Serial.write("ok\n");
      Serial.read();
    } else {
      serialReadString = "";
      //Serial.write("ok\n");
      Serial.read();
    }
    last_loop_thread1 = millis();
  }

  //Serial.println(distance);

  //  if (distanceBetweenCurrentAndNew > 20) {
  //    chanageble_speed = 2000;
  //  } else {
  //    chanageble_speed = 300;
  //  }
  //  Serial.println(chanageble_speed);
  stepper1.moveTo(xMoveTo * X_STEP_PER_MM);
  stepper1.setSpeed(chanageble_speed * 1.3333);
  stepper1.runSpeedToPosition();

  stepper2.moveTo(yMoveTo * Y_STEP_PER_MM);
  stepper2.setSpeed(chanageble_speed * 1.3333);
  stepper2.runSpeedToPosition();

  //Update realtime location to raspberrypi through serial port
  if (millis() - last_loop_thread2 > 40) {
    Serial.print("Current_PosX:");
    Serial.print(stepper1.currentPosition());
    Serial.print(":Y:");
    Serial.print(stepper2.currentPosition());
    Serial.print("\r\n");
    //    Serial.println();
    //    Serial.print("X ENDSTOP  ");
    //    Serial.println(digitalRead(X_ENDSTOP));
    //    Serial.print("Y ENDSTOP  ");
    //    Serial.println(digitalRead(Y_ENDSTOP));

    last_loop_thread2 = millis();
  }

  lastxMoveTo = xMoveTo;
  lastyMoveTo = yMoveTo;
}

//The homing function
void homingAllAxis() {

  int xInitHoming = 1;
  int yInitHoming = 1;

  while (digitalRead(X_ENDSTOP)) {
    stepper1.moveTo(xInitHoming);
    stepper1.setSpeed(800);
    xInitHoming -= 1;
    stepper1.runSpeedToPosition();

    if (millis() - last_loop_time > 50) {
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


  while (digitalRead(Y_ENDSTOP)) {
    stepper2.moveTo(yInitHoming);
    stepper2.setSpeed(800);
    yInitHoming -= 1;
    stepper2.runSpeedToPosition();

    if (millis() - last_loop_time > 20) {
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

int Lerp(float currentPos, float targetPos, float t) {
  return int(currentPos + (targetPos - currentPos) * t);
}
