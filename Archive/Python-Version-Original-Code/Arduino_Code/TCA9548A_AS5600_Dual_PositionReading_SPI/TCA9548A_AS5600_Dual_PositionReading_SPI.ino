/*-------------------------------------------------------------
* Arduino 2
* This code is use for receive dual AS5600 encoder to read the platform position.
* The data sending to raspberry pi board by using serial communaction.
* The format of the data is ""

-------------------------------------------------------------*/

// include as5600 and wire libraries
#include <Wire.h>
#include <AS5600.h>
#include <SPI.h>

const byte START_BYTE = 0xAA;
const byte END_BYTE = 0x55;
const int slaveSelectPin = 7;

// Define two as5600 objects as X and Y for further use
AS5600 stpA;
AS5600 stpB;

int I2CA = 4;
int I2CB = 2;

float X = 0, Y = 0;

String serialReadString = "";

// assign constants values
// float pitch = 8 / (2*M_PI);   // mm/radian of stepper motor
// float radius = 12.73 / 2;    //3D printer A and B stepper polley radius

void setup() {
  // Start serial communication
  Serial.print("setup initializing");
  Serial.begin(115200);
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }
  Wire.begin();

  //Setup Spi
  SPISetup();


  // Set up and check if X is connected
  Serial.println("X setup initializing");
  TCA9548(I2CA);
  // stpA.begin(4);
  // stpA.setDirection(AS5600_CLOCK_WISE);
  int b = stpA.isConnected();
  Serial.print("Connect: ");
  Serial.println(b);
  Serial.println("X setup done");
  delay(20);

  // Set up and check if Y is connected
  Serial.println("Y setup initializing");
  TCA9548(I2CB);
  // stpB.begin(4);
  // stpB.setDirection(AS5600_COUNTERCLOCK_WISE);
  int c = stpB.isConnected();
  Serial.print("Connect: ");
  Serial.println(c);
  Serial.println("y setup done");
  delay(20);

  X = 0;
  Y = 0;

  delay(5000);
  Serial.println("setup done");
}

void loop() {

  // ListenSerialPort();

  // if (serialReadString == "homing\n") {
  //   //homing the platform
  //   sensorHomeing();
  //   Serial.write("Homing is finished\n");
  //   Serial.read();
  // }


  static uint32_t lastTime = 0;
  if (millis() - lastTime >= 5) {
    // Switch the address the I2CA
    TCA9548(I2CA);
    X = stpA.getCumulativePosition();
    // Switch the address the I2CB
    TCA9548(I2CB);
    Y = stpB.getCumulativePosition();


    lastTime = millis();

    sendNumbers(X, Y);

    // Serial.print("cX:");
    // Serial.print(X);
    // Serial.print(":Y:");
    // Serial.println(Y);
    //Serial.print("\n");

    // serialReadString = "";
  }
}

void TCA9548(int bus) {
  Wire.beginTransmission(0x70);
  Wire.write(1 << bus);
  Wire.endTransmission();
}


void sensorHomeing() {
  delay(3000);
  TCA9548(I2CA);
  stpA.resetCumulativePosition(0);
  delay(5);
  TCA9548(I2CB);
  stpB.resetCumulativePosition(0);

  Serial.write("ok\n");
}


void ListenSerialPort() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    serialReadString += c;
    //delay(2);
    if (c == '\n') {
      break;
    }
  }
}

void SPISetup() {
  SPI.begin();
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
}

void sendNumbers(int num1, int num2) {
  Serial.print("cX:");
  Serial.print(X);
  Serial.print(":Y:");
  Serial.println(Y);
  //Serial.print("\n");

  digitalWrite(slaveSelectPin, LOW);

  SPI.transfer(START_BYTE);
  SPI.transfer(highByte(num1));
  SPI.transfer(lowByte(num1));
  SPI.transfer(highByte(num2));
  SPI.transfer(lowByte(num2));
  SPI.transfer(END_BYTE);

  digitalWrite(slaveSelectPin, HIGH);
}