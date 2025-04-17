#include <AccelStepper.h>
#include <MultiStepper.h>

AccelStepper stepper1(1, 2, 5);
AccelStepper stepper2(1, 3, 4);


#define X_ENDSTOP 8
#define Y_ENDSTOP 9

String serialReadString;
String serialData;
int last_loop_time = 0;
bool Xhomed = false;
bool Yhomed = false;

void setup() {
  //init serial.port
  Serial.begin(115200);
  while (!Serial);
  Serial.setTimeout(250);

  pinMode(X_ENDSTOP, INPUT);
  pinMode(Y_ENDSTOP, INPUT);

  stepper1.setMaxSpeed(1000.0);
  stepper2.setMaxSpeed(1000.0);
}

void loop() {
  // put your main code here, to run repeatedly:

  //Read char from serial, and cover into string.
  while (Serial.available() > 0) {
    char c = Serial.read();
    serialReadString += c;
    //delay(2);
    if (c == '\n') {
      break;
    }
  }

  //Reading the coming serail data for movement command
  if (serialReadString.length() > 0) {
    //Serial.print(serialReadString);
    Serial.write("ok\n");
    if (serialReadString == "homing\n") {

      //homing the platform
      homingAllAxis();

    }
  }
  serialReadString = "";
  Serial.read();
}




void homingAllAxis() {

  int xInitHoming = 1;
  int yInitHoming = 1;

  while (!digitalRead(X_ENDSTOP)) {
    stepper1.moveTo(xInitHoming);
    stepper1.setSpeed(500);
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
    stepper2.setSpeed(500);
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
  Serial.print(stepper2.currentPosition());

  int xInitHoming = 1;
  int yInitHoming = 1;
}

//    if (digitalRead(Y_ENDSTOP) == 0) {
//      stepper2.moveTo(-1000);
//      stepper2.setSpeed(100);
//      stepper2.runSpeedToPosition();
//    } else {
//      stepper2.setCurrentPosition(0);
//      Yhomed = true;
//    }
//    break;
//}
