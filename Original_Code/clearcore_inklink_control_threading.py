'''
ClearCore 20230724 Version

Hardware:
    Raspberry Pi 4B 2G
    24V Power supply
    ClearCore
    Webcam Inklink

    Webcam Inklink(Pen Position Data) -->
    Raspberry Pi(Python script) -->
    Serial Port(Sending position to ClearCore)
    Arduinio(Driving Motor) -->
    Return(motor stepped signal) -->
    Raspberry Pi
'''

from threading import Thread, Event
import threading
import time
from datetime import datetime
import numpy as np
import math

import serial
import time
from datetime import datetime

import filter

import sys
import usb.core
import usb.util


xAxis_Filter = filter.Filter()  # Filter is for pen
yAxis_Filter = filter.Filter()  # Filter is for pen

"""
Initial the Serail Device
"""
# Port definition this one is for seeduino XIAO
ser = serial.Serial('/dev/ttyACM0', 115200)
# ser = serial.Serial('/dev/ttyUSB0', 115200)   # This one is for ESP32
serXIAO = serial.Serial('/dev/ttyACM1',115200)
ser.close()
serXIAO.close()
time.sleep(0.2)


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


"""
Initial Pen
"""

dev = usb.core.find(idVendor=0x056A, idProduct=0x0221)
# All the inklink data feeding data
# first endpoint
interface = 0
# usb.util.release_interface(dev, interface)

endpoint = dev[0][(0, 0)][0]
print("ENDPOINT"+str(endpoint.bEndpointAddress))
print("Break point 1")
if dev.is_kernel_driver_active(interface) is True:
    # tell the kernel to detach
    dev.detach_kernel_driver(interface)
    # claim the device
    usb.util.claim_interface(dev, interface)

USBRQ_HID_GET_REPORT = 0x01
USBRQ_HID_SET_REPORT = 0x09

buf = [0x80, 0x01, 0x03, 0x01, 0x01]
buf.extend([0]*(33-len(buf)))
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)
# lack of this will using digitizer as output

buf = [0x80, 0x01, 0x0A, 0x01, 0x01, 0x0B, 0x01]
buf.extend([0]*(33-len(buf)))
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

print(dev.ctrl_transfer(0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, data_or_wLength=33))
print("Break point 2")
buf = [0x80, 0x01, 0x0B, 0x01]
buf.extend([0]*(33-len(buf)))
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

buf = [0x80, 0x01, 0x02, 0x01, 0x01]
buf.extend([0]*(33-len(buf)))
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

buf = [0x80, 0x01, 0x0A, 0x01, 0x01, 0x02, 0x01]
buf.extend([0]*(33-len(buf)))
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

print(dev.ctrl_transfer(0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, data_or_wLength=33))
print("\n\ndata")


start = time.time()

# STEP = 8                   # When using Stepper motor, the resolution of the micro step
X_BOARD = 190               # acctuate size of the sensor detection in mm  10.11res/mm
Y_BOARD = 145               # acctuate size of the sensor detection in mm. 13.24res/mm
x_res_mm = 9                # 1 mm corrisponding to the pen resolution
y_res_mm = 13               # 1 mm corrisponing
x_tar = 960  # 180          # next target x location. getting from svg file
y_tar = 960  # 260          # next target y location. getting from svg file
delta_x = 0                 # change of the x location
delta_y = 0                 # change of the y location
x_platform_current = 0      # current x platform location
y_platform_current = 0      # current y platform location
x_platform_moveTo = 0       # absolute position x should move
y_platform_moveTo = 0       # absolute position y should move
x_move = 0                  # relative distance x should move
y_move = 0                  # relative distance y should move

x = 0                       # pen raw data from inkling
y = 0                       # pen raw data from inkling
pressed = 0                 # pen raw data from inkling

"""
Start Homing
"""

print("break point 4")
ser.open()
serXIAO.open()
command(ser, "homing\n")
command(serXIAO, "homing\n")
print("Homeing Before Work")
ser.close()
serXIAO.close()
print("break point 4.5")


# Thead lock for the veriable in the thread
lock1 = threading.Lock()
lock2 = threading.Lock()


def readingPenData():
    global x
    global y
    global pressed

    collected = 0

    while True:
        try:
            #with lock1:

            time1 = time.time()
            data = dev.read(endpoint.bEndpointAddress,
                            endpoint.wMaxPacketSize)

            collected += 1
            x = data[1]+data[2]*256
            y = data[3]+data[4]*256
            pressed = data[5]

            time2 = time.time()

            if (time2 - time1 > 0.1):
                dump = dev.read(endpoint.bEndpointAddress,
                                endpoint.wMaxPacketSize)
                print(datetime.now(), "pen thread delay ", time2 - time1)

            # print(datetime.now(), 'thread1 x_pen:%d\ty_pen:%d\tpressed:%d' %
            #       (x, y, pressed))
            # print("thread1 Loop Begin \n")
            # print(datetime.now(), 'thread1 x_pen:%d\ty_pen:%d\tpressed:%d' %
            #       (x, y, pressed))
            # 业务代码 end

        except usb.core.USBError as e:
            data = None
            if e.args == ('Operation timed out',):
                continue
            # release the device

        except KeyboardInterrupt:
            usb.util.release_interface(dev, interface)
            break
    # release the device
    #usb.util.release_interface(dev, interface)


def move_the_board():
    ser.open()
    x_platform_current = 0
    y_platform_current = 0
    # Move to the center
    # command(ser, "X120,Y120\r\n")
    # x_platform_moveTo = 250
    # y_platform_moveTo = 250

    while (1):
        try:
            x_pen = xAxis_Filter.average_filter(x)
            delta_x = x_pen - x_tar
            # x_move = float(filter.mapping(delta_x, 0, 985, 0, 100))
            # mapping relationship need to check
            x_move = float(filter.mapping(delta_x, 0, 1920, 0, 193))
            x_platform_moveTo = x_platform_current + x_move

            y_pen = yAxis_Filter.average_filter(y)
            delta_y = y_pen - y_tar
            # y_move = float(filter.mapping(delta_y, 0, 1920, 0, 145))
            # mapping relationship need to check
            y_move = float(filter.mapping(delta_y, 0, 1920, 0, 145))

            # if (abs(y_move) > 5):
            y_platform_moveTo = y_platform_current + y_move

            # print(datetime.now(), 'thread2 x_move:%d\ty_move:%d\tpressed:%d' %
            #       (x_move, y_move, pressed))
            # print(datetime.now(), 'thread2 x_tar:%d\ty_tar:%d\tpressed:%d' %
            #       (x_tar, y_tar, pressed))
            # reading serial data from arduino, and getting thlse current position of the xy platform

            if ((x_platform_moveTo > 0 and x_platform_moveTo <= 507)):
                if (((y_platform_moveTo > 0 and y_platform_moveTo < 240))):
                    # print("thread2 Loop Begin")
                    # pdb.set_trace()
                    # command_tmp = \
                    #     "X" + str(xAxisPlatform_Filter.average_filter(round(x_platform_moveTo, 0 ))) + \
                    #     ",Y" + str(yAxisPlatform_Filter.average_filter(round(y_platform_moveTo, 0))) + "\r\n"
                    # if (abs(x_move) > 2 or abs(y_move) > 2):
                    command_tmp = \
                        "X" + str(int(x_platform_moveTo)) + \
                        ",Y" + str(int(y_platform_moveTo)) + "\r"

                    command(ser, command_tmp)

            # This block is use for reading the comming current location

            # serial_reading = ser.readline()
            # if serial_reading != 0:
            #     if serial_reading != b'ok\n':
            #         if serial_reading[0] == 67:
            #             decoded_reading = serial_reading.decode("utf-8")
            #             # print("raw_reading:   ",serial_reading)
            #             # print("decoded_reading:   ", decoded_reading)
            #             x_current = int(decoded_reading.split(':')[1])
            #             y_current = int(decoded_reading.split(':')[
            #                             3].split('\\')[0])
            #             # print("Current position from Serial Port: X%d  Y%d" % x_current, y_current)
            #             # This shit is change by the STEP resolution
            #             x_platform_current = float(
            #                 x_current) / (20.0 * (STEP/8))
            #             y_platform_current = float(
            #                 y_current) / (26.66 * (STEP/8))
                # print(datetime.now(), "thread2 delta: X %f Y %f" %
                #       (delta_x, delta_y))

                # print(datetime.now(), "thread2 move: X %f Y %f" %
                #       (x_move, y_move))

            print(datetime.now(), "thread2 platform_moveTo:  X %f Y %f" %
                    (x_platform_moveTo, y_platform_moveTo))

            # print(datetime.now(), "thread2 current: X %f Y %f  thread2 Loop End" %
            #         (x_platform_current, y_platform_current))

        except KeyboardInterrupt:
            ser.close()
            break

    ser.close()


def get_current_location():
    global x_platform_current
    global y_platform_current

    serXIAO.open()
    while (1):
        try:
            serial_reading = serXIAO.readline()
            # print(serial_reading)
            if serial_reading != 0:
                if serial_reading != b'ok\n':
                    if serial_reading[0] == 99:    # DEC number of ASCII Char C
                        decoded_reading = serial_reading.decode("utf-8")
            # print("raw_reading:   ",serial_reading)
            # print("decoded_reading:   ", decoded_reading)
            x_current_resolution = int(decoded_reading.split(':')[
                                       1])  # Return Resolution
            y_current_resolution = int(decoded_reading.split(':')[
                                       3].split('\\')[0])

            x_current_position = filter.mapping(
                x_current_resolution, 0, 27640,  0, 508)
            y_current_position = filter.mapping(
                y_current_resolution, 0, 27640,  0, 508)
            # print(datetime.now(), "thread2 current: X %i Y %i  thread2 Loop End" %
            #       (x_current_resolution, y_current_resolution))
            print(datetime.now(), "thread3 current: X %i Y %i  thread2 Loop End" %
                  (x_current_position, y_current_position))

        except KeyboardInterrupt:
            serXIAO.close()




readpenTask = threading.Thread(target=readingPenData)
moveboardTask = threading.Thread(target=move_the_board)
getcurrentTask = threading.Thread(target=get_current_location)
# updatetarget = threading.Thread(target=update_target)
# test = threading.Thread(target=test_thread)

readpenTask.start()
moveboardTask.start()
getcurrentTask.start()


def Lerp(lastTarget, nextTarget, v):
    return lastTarget + (nextTarget - lastTarget) * v

def update_target():
    state = 0

    global x_tar
    global y_tar

    point1 = [100, 100]
    point2 = [100, 150]
    point3 = [150, 150]
    point4 = [150, 100]

    last_update_time = time.time()
    x_tar = point1[0]
    y_tar = point1[1]

    prop = 0

    while (1):
        try:

            if (time.time() - last_update_time > 5):
                tmp = math.sqrt(abs(x-x_tar)**2 + abs(y-y_tar)**2)
                last_update_time = time.time()
                print("Distance ", tmp)

            if (pressed):
                if (tmp < 25):
                    #         prop += 0.01

                    #     if (prop >= 1):
                    #         prop = 0
                    #         state += 1

                    #     if (state == 0):
                    #         x_tar = Lerp(point1[0], point2[0], prop)
                    #         y_tar = Lerp(point1[1], point2[1], prop)

                    #     if (state == 1):
                    #         x_tar = Lerp(point2[0], point3[0], prop)
                    #         y_tar = Lerp(point2[1], point3[1], prop)

                    #     if (state == 2):
                    #         x_tar = Lerp(point3[0], point4[0], prop)
                    #         y_tar = Lerp(point3[1], point4[1], prop)

                    #     if (state == 3):
                    #         x_tar = Lerp(point4[0], point1[0], prop)
                    #         y_tar = Lerp(point4[1], point1[1], prop)

                    # print("Threading 3:  tarX %d tarY %d" % (x_tar, y_tar))
                    if (state == 0):
                        if (y_tar < 100*y_res_mm):
                            # time.sleep(0.5)
                            y_tar += y_res_mm * 1
                            last_update_time = time.time()
                            # print("updateing y_tar into: ", y_tar)
                        else:
                            state = 1
                    if (state == 1):
                        if (x_tar < 100*x_res_mm):
                            x_tar += x_res_mm * 1
                        else:
                            state = 2
                    if (state == 2):
                        if (y_tar > 20*y_res_mm):
                            y_tar -= y_res_mm * 1
                        else:
                            state = 3
                    if (state == 3):
                        if (x_tar > 20*x_res_mm):
                            x_tar -= x_res_mm * 1
                        else:
                            state = 4
                    if (state == 4):
                        break

                # if pressed update the target data
        except KeyboardInterrupt:
            ser.close()
            break