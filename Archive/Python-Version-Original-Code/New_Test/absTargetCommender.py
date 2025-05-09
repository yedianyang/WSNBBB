from threading import Thread, Event
import threading
import time
from datetime import datetime
import numpy as np
import math

import sys
import serial.tools.list_ports
import usb.core
import usb.util
import serial

import serial
import time
from datetime import datetime
import sineMovePosition
import filter


def command(ser, command):
    start_time = datetime.now()
    ser.write(str.encode(command))
    time.sleep(0.01)

    # while True:

    #     line = ser.readline()
    #     # print(line)
    #     # if (len(line) > 20):
    #     #     return line
    #     #     break
    #     if line == b'ok\n':
    #         break

ports = serial.tools.list_ports.comports()
serXIAOAddress = ""
serClearCoreAddress = ""

# 遍历并打印每个串口设备的信息
for port in ports:
    # print(f"设备名：{port.device}\t描述：{port.description}\t硬件ID：{port.hwid}")
    if ("ClearCore" in port.description):
        serClearCoreAddress = port.device
    if ("XIAO" in port.description):
        serXIAOAddress = port.device

print("ClearCore Address:  ", serClearCoreAddress)
#print("XIAO Address:  ", serXIAOAddress)

#serXIAO = serial.Serial(serXIAOAddress, 115200)
serClearCore = serial.Serial(serClearCoreAddress, 115200)


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

print("Break point 1")

print("ClearCore Homing Begin")
command(serClearCore, "homing\n")
print(serClearCore.readline())

time.sleep(5)
#serClearCore.close()

def sendSerialData():
    #serClearCore.open()
    print("Break3")
    #command(serClearCore, "X50,Y50\r\n")
    time.sleep(1)
    while (1):
        try:
            # print("Break4")
            command_tmp = ""
            x_platform_moveTo = 5  # int(sineMovePosition.x_aimPoint)
            y_platform_moveTo = float(sineMovePosition.y_aimPoint)

            if ((x_platform_moveTo >= 0 and x_platform_moveTo <= 500)):
                if (((y_platform_moveTo >= 0 and y_platform_moveTo < 500))):

                    # if (abs(x_move) > 2 or abs(y_move) > 2):
                    command_tmp = \
                        "X250,Y" + str(round(float(y_platform_moveTo),2)) + "\r\n"
                        #"X" + str(float(y_platform_moveTo)) + \

                    print(datetime.now(), "command_tmp:  ", command_tmp)

                    #command(ser, command_tmp)
                    command(serClearCore,command_tmp)
                    print(command_tmp)
                    # print("Break5")

        except KeyboardInterrupt:
            #ser.close()

            # usb.util.release_interface(dev, interface)
            break


def sendSerialData1():
    print("Break3")
    #serClearCore.open()
    while (1):
        try:
            # print("Break4")
            command_tmp = ""
            x_platform_moveTo = 5  # int(sineMovePosition.x_aimPoint)
            y_platform_moveTo = int(sineMovePosition.y_aimPoint)

            if ((x_platform_moveTo >= 0 and x_platform_moveTo <= 240)):
                if (((y_platform_moveTo >= 0 and y_platform_moveTo < 240))):

                    # if (abs(x_move) > 2 or abs(y_move) > 2):
                    command_tmp = \
                        "X" + str(int(y_platform_moveTo)) + \
                        ",Y" + str(int(y_platform_moveTo)) + "\r\n"

                    print(datetime.now(), "command_tmp:  ", command_tmp)

                    command(serClearCore, command_tmp)
                  
                    # print(command_tmp)
                    # print("Break5")

        except KeyboardInterrupt:
            #ser.close()

            # usb.util.release_interface(dev, interface)
            break

def readSerialData():
    global x_current_position

    print("ReadSerail Break1")
    #serClearCore.open()
    while (1):
        try:
            serial_reading = serXIAO.readline()
            # print(serial_reading)
            if serial_reading != 0:
                if serial_reading != b'ok\n':
                    if serial_reading[0] == 99:    # DEC number of ASCII Char C
                        decoded_reading = serial_reading.decode("utf-8")
            #print("raw_reading:   ",serial_reading)
            #print("decoded_reading:   ", decoded_reading)
            x_current_resolution = int(decoded_reading.split(':')[
                                       1])  # Return Resolution
            # x_current_rev=int(decoded_reading.split(':')[2].split('\\')[0])

            x_current_position = filter.mapping(x_current_resolution, 0, 27640,  0, 507)
            # print("Current position from Serial Port: X%d  Y%d" % x_current, y_current)
            # This shit is change by the STEP resolution
            # x_platform_current = float(
            #     x_current) / (26.66 * (STEP/8))
            # y_platform_current = float(
            #     y_current) / (26.66 * (STEP/8))
            # print(datetime.now(), "thread2 delta: X %f Y %f" %
            #       (delta_x, delta_y))
            # print(datetime.now(), "thread2 move: X %f Y %f" %
            #       (x_move, y_move))

            # print(datetime.now(), "thread2 platform_moveTo:  X %f Y %f" %
            #       (x_platform_moveTo, y_platform_moveTo))
            # print(datetime.now(), "thread2 current: X %i Y %i  thread2 Loop End" %
            #     (x_current_resolution, x_current_resolution))
            print(datetime.now(), "thread2 current: X %i Y %i  thread2 Loop End" %
                  (x_current_position, y_platform_current))

        except KeyboardInterrupt:
            #serXIAO.close()
            break


PositionUpdaterTask = threading.Thread(target=sineMovePosition.positionUpdater)
PositionUpdaterTask.start()

#readSerialDataTask = threading.Thread(target=readSerialData)
#readSerialDataTask.start()

sendSerialDataTask = threading.Thread(target=sendSerialData)
sendSerialDataTask.start()

#sendSerialDataTask1 = threading.Thread(target=sendSerialData1)
#sendSerialDataTask1.start()

