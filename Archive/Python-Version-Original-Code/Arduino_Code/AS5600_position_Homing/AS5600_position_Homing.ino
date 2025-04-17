//
//    FILE: AS5600_position.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: demo
//    DATE: 2022-12-20


#include "AS5600.h"
#include "Wire.h"

AS5600 as5600;   //  use default Wire
static String serialReadString = "";

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("AS5600_LIB_VERSION: ");
  Serial.println(AS5600_LIB_VERSION);

  //  ESP32
  //  as5600.begin(14, 15);
  //  AVR
  as5600.begin(6);  //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.

  Serial.println(as5600.getAddress());
  //  as5600.setAddress(0x40);  // AS5600L only
  int b = as5600.isConnected();
  Serial.print("Connect: ");
  Serial.println(b);

  sensorHomeing();
}



void loop()
{
  ListenSerialPort();

  if (serialReadString == "homing\n") {
    //homing the platform
    sensorHomeing();
    Serial.write("Homing is finished\n");
    Serial.read();
  }

  static uint32_t lastTime = 0;

  //  set initial position
  //XXas5600.getCumulativePosition();

  //  update every 100 ms
  //  should be enough up to ~200 RPM
  if (millis() - lastTime >= 40)
  {
    lastTime = millis();
    Serial.print("cX:");
    Serial.print(as5600.getCumulativePosition());
    Serial.print(":cY:");
    Serial.println(as5600.getRevolutions());
  }


  serialReadString = "";
}

void sensorHomeing() {
  delay(3000);
  as5600.resetCumulativePosition(0);


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

// -- END OF FILE --
