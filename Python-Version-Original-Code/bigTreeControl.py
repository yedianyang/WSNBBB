from datetime import datetime
import serial
import time



def init_serial_connection(ser):
    ser = serial.Serial('/dev/ttyACM0', 115200)  # Port definition
    time.sleep(0.1)
    ser.close()


def command(ser, command):
    start_time = datetime.now()
    ser.write(str.encode(command))
    # time.sleep(0.1)

    while True:
        line = ser.readline()
        print(line)

        if line == b'ok\n':
            break
