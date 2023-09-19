from threading import Thread, Event
import threading
import time
from datetime import datetime
import numpy as np
import math

import sys
import usb.core
import usb.util

import serial
import time
from datetime import datetime



ser = serial.Serial('/dev/cu.usbmodem112301', 115200)
# ser = serial.Serial('/dev/ttyUSB0', 115200)   # This one is for ESP32
time.sleep(0.2)
ser.close()

print("Break point 1")
def command(ser, command):
    start_time = datetime.now()
    ser.write(str.encode(command))
    time.sleep(0.1)

    while True:

        line = ser.readline()
        # print(line)
        # if (len(line) > 20):
        #     return line
        #     break
        if line == b'ok\n':
            break

print("Break point 2")
ser.close()

STEP = 8
X_BOARD = 190               # acctuate size of the sensor detection in mm
Y_BOARD = 145               # acctuate size of the sensor detection in mm
x_res_mm = 9                # 1 mm corrisponding to the pen resolution
y_res_mm = 13               # 1 mm corrisponing
x_tar = 960  # 180            # next target x location. getting from svg file
y_tar = 960  # 260             # next target y location. getting from svg file
delta_x = 0                 # change of the x location
delta_y = 0                 # change of the y location
x_platform_current = 0      # current x platform location
y_platform_current = 0      # current y platform location
x_move = 0                  # relative position x should move
y_move = 0                  # distance the y should move
x_platform_moveTo = 0       # absolute position x should move
y_platform_moveTo = 0       # absolute position y should move
x = 0
y = 0
pressed = 0
x_platform_current = 0
y_platform_current = 0


def readSerialData():
    ser.open()
    while (1):
        try:
            serial_reading = ser.readline()
            if serial_reading != 0:
                if serial_reading != b'ok\n':
                    if serial_reading[0] == 67:
                        decoded_reading = serial_reading.decode("utf-8")
                        # print("raw_reading:   ",serial_reading)
                        # print("decoded_reading:   ", decoded_reading)
                        x_current = int(decoded_reading.split(':')[1])
                        y_current = int(decoded_reading.split(':')[
                                        3].split('\\')[0])
                        # print("Current position from Serial Port: X%d  Y%d" % x_current, y_current)
                        # This shit is change by the STEP resolution
                        x_platform_current = float(
                            x_current) / (20.0 * (STEP/8))
                        y_platform_current = float(
                            y_current) / (26.66 * (STEP/8))
                # print(datetime.now(), "thread2 delta: X %f Y %f" %
                #       (delta_x, delta_y))

                # print(datetime.now(), "thread2 move: X %f Y %f" %
                #       (x_move, y_move))

                # print(datetime.now(), "thread2 platform_moveTo:  X %f Y %f" %
                #       (x_platform_moveTo, y_platform_moveTo))

                print(datetime.now(), "thread2 current: X %f Y %f  thread2 Loop End" %
                        (x_platform_current, y_platform_current))

        except KeyboardInterrupt:
            ser.close()
            #usb.util.release_interface(dev, interface)
            break

    ser.close()


readSerialDataTask = threading.Thread(target=readSerialData)
readSerialDataTask.start()