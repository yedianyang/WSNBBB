import serial
import time
from datetime import datetime


def command(ser, command):
    start_time = datetime.now()

    #print("Command send")
    # time.sleep(0.1)

    while True:
        ser.write(str.encode(command))
        line = ser.readline()
        #print(line)

        if line == b'ok\n':
            break


ser = serial.Serial('/dev/ttyACM0', 115200)
time.sleep(0.1)

ser.close()


# 200mm movement in one command

# command(ser, "G0 X180 F20000 \r\n")
# ser.close()

# time.sleep(2)

# Segmenting 200mm movement into 2mm in 100 times.
# for i in range(60):
#   #ser.open()
#   command(ser, "G1 X"+str(-0.1*i)+" F20000 \r\n")
#   #time.sleep(1)
#   print(str(i))
#   #ser.close()


# command(ser,"M204 P500 \r\n")
movetime = 1
a = 10

# while (True):
#     ser.open()

#     print(datetime.now(), " New + command send")
#     #for i in range(1100):
#     command(ser, "G0 X100 F10000 \r\n")
#     command(ser, "M400 \r\n")
#     #print(datetime.now(), "times ",i)

#     #time.sleep(.2)

#     # for i in range(1100):
#     command(ser, "G0 X0 F10000 \r\n")
#     command(ser, "M400 \r\n")

#     print(datetime.now(), " New - command send")
#     print(datetime.now(), "   Movetime count: ", movetime)
#     movetime += 1
#     ser.close()
ser.open()
command(ser, "G0 X0 F30000 \r\n")
print(datetime.now())
ser.close()