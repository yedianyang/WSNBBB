import serial
import time
from datetime import datetime

def command(ser, command):
  start_time = datetime.now()
  ser.write(str.encode(command)) 
  #time.sleep(1)

  while True:
    line = ser.readline()
    print(line)

    if line == b'ok\n':
      break

ser = serial.Serial('/dev/ttyACM0', 115200)
time.sleep(0.1)

ser.close()

ser.open()

#200mm movement in one command

command(ser, "G0 X180 F20000 \r\n")
#ser.close()

time.sleep(2)

#Segmenting 200mm movement into 2mm in 100 times.
for i in range(60):
  #ser.open()
  command(ser, "G1 X"+str(-0.1*i)+" F20000 \r\n")
  #time.sleep(1)
  print(str(i))
  #ser.close()

ser.close()