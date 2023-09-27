#include <SPI.h>


volatile boolean received;
volatile byte receivedData;





const int slaveSelectPin = 2;
const byte START_BYTE = 0xAA;
const byte END_BYTE = 0x55;

float number1 = -123.45;
float number2 = 67.89;

ISR(SPI_STC_vect)  //Inerrrput routine function
{
  receivedData = SPDR;  // Get the received data from SPDR register
  received = true;      // Sets received as True
}

void setup() {

  Serial.begin(115200);

  pinMode(MISO, OUTPUT);  //Sets MISO as OUTPUT
  SPCR |= _BV(SPE);       //Turn on SPI in Slave Mode
  received = false;
  SPI.attachInterrupt();  //Activate SPI Interuupt

  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
}

void sendFloat(float number) {
  byte *data = (byte *)&number;
  for (int i = 0; i < sizeof(float); i++) {
    SPI.transfer(data[i]);
  }
}

void loop() {
  digitalWrite(slaveSelectPin, LOW);
  SPI.transfer(START_BYTE);
  sendFloat(number1);
  sendFloat(number2);
  SPI.transfer(END_BYTE);
  digitalWrite(slaveSelectPin, HIGH);
  delay(1000);
}