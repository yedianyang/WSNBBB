'''
09182023 Test

Hardware:
    Raspberry Pi 4B 2G
    24V Power supply
    Arduino XIAO <- 5600X position data
    ClearCore -> Driving motor
    Webcam Inklink

    Webcam Inklink(Pen Position Data) -->
    Raspberry Pi(Python script) -->
    Serial Port(Sending position to Arduino)
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

import sys
import serial.tools.list_ports
import usb.core
import usb.util
import serial

import time
from datetime import datetime

import filter

xAxis_Filter = filter.Filter()
yAxis_Filter = filter.Filter()


# Inituilaztion
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
print("XIAO Address:  ", serXIAOAddress)

serXIAO = serial.Serial(serXIAOAddress, 115200)
serClearCore = serial.Serial(serClearCoreAddress, 115200)
time.sleep(0.2)
serXIAO.close()
serClearCore.close()


def command(ser, command):
    start_time = datetime.now()
    ser.write(str.encode(command))
    time.sleep(0.1)

    # while True:

    #     line = ser.readline()
    #     if line == b'ok\n':
    #         break


# Begin -  USB Reading the pen data block
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

# End - USB Reading the pen data block End

start = time.time()

STEP = 8
X_BOARD = 190               # acctuate size of the drawming board sensor detection in mm
Y_BOARD = 145               # acctuate size of the drawming board sensor detection in mm
x_res_mm = 9                # 1 mm corrisponding to the pen resolution
y_res_mm = 13               # 1 mm corrisponing
x_tar = 960  # 180          # next target x location. getting from svg file
y_tar = 960  # 260          # next target y location. getting from svg file
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


last_update = 0


# Homing the Axis, and the
print("ClearCore Homing Begin")
serClearCore.open()
time.sleep(0.1)
#command(serClearCore, "homing\r\n") #Here could not use \r\n
serClearCore.write(b"homing\r\n")
#print(serClearCore.readline())
serClearCore.close()
print("ClearCore Homing Finished")
time.sleep(3)

serXIAO.open()
print("XIAO Homing Begin")
#(serXIAO, "homing\r\n")
serXIAO.write(b"homing\r\n")
print(serXIAO.readline())
print("XIAO Homing Finished")
print("Homing End")
time.sleep(0.2)
serXIAO.close()

lock1 = threading.Lock()
lock2 = threading.Lock()


def readingPenData():
    global x
    global y
    global pressed

    collected = 0
    # attempts = 50
    # while (collected < attempts or 1) :
    print("Waiting for pen")
    while True:

        try:
            # 记录系统时间1
            with lock1:
                # print(datetime.now(), "thread 1 begin")
                time1 = time.time()
                data = dev.read(endpoint.bEndpointAddress,
                                endpoint.wMaxPacketSize)
                # 记录系统时间2

                # time1 - time2 看看delay在哪里

                collected += 1
                x = data[1]+data[2]*256
                y = data[3]+data[4]*256
                pressed = data[5]

                time2 = time.time()
                # print(datetime.now(), 'thread1 x_pen:%d\ty_pen:%d\tpressed:%d' %
                #          (x, y, pressed))

                if (time2 - time1 > 0.1):
                    dump = dev.read(endpoint.bEndpointAddress,
                                    endpoint.wMaxPacketSize)
                    print(datetime.now(), "pen thread delay ", time2 - time1)

            # y_platform_current = y_platform_moveTo
            # print("thread1 Loop Begin \n")
            # print(datetime.now(), 'thread1 x_pen:%d\ty_pen:%d\tpressed:%d' %
            #       (x, y, pressed))
            # 业务代码 end
            # print("thread1 Loop End \n")
        except usb.core.USBError as e:
            data = None
            if e.args == ('Operation timed out',):
                continue
            # release the device

        except KeyboardInterrupt:
            dev.reset()
            # serClearCore.close()
            # serXIAO.close()
            # usb.util.release_interface(dev, interface)


def move_the_board():
    serClearCore.open()
    serXIAO.open()

    time.sleep(2)
    #command(serClearCore, "X5,Y5\r\n")
    #x_platform_current = 10
    #y_platform_current = 10
    #x_platform_moveTo = 10
    #y_platform_moveTo = 10

    while (1):
        try:
            #serial_reading = serXIAO.readline()
            # #print(serial_reading[-2])
            # #if serial_reading != 0:
            # if serial_reading[0] == 62:    # DEC number of ASCII Char ">"
            #     if serial_reading[-2] == 60:
            #         decoded_reading = serial_reading.decode("ascii")

            #         x_platform_current = round(float(decoded_reading.split(':')[1]),2) #* 75.1 / 4095
            #         y_platform_current = round(-float(decoded_reading.split(':')[3].split('<')[0]),2) # * 75.1 / 4095
            
            #print(datetime.now(), 'thread2 x_platform_current:%.2f\ty_platform_current:%.2f\tpressed:%d' % (x_platform_current, y_platform_current, pressed))


            x_pen = x # xAxis_Filter.average_filter(x)
            delta_x = x_pen - x_tar
            x_move = float(filter.mapping(delta_x, 0, 1920, 0, 193 ))
            x_platform_moveTo = x_platform_current + x_move

            y_pen = y# yAxis_Filter.average_filter(y)
            delta_y = y_pen - y_tar
            y_move = float(filter.mapping(delta_y, 0, 1920, 0, 145))
            y_platform_moveTo = y_platform_current + y_move


            print(datetime.now(), 'thread2 x_pen: %d\tx_move: %d\t pressed: %d'%(x_pen, y_pen, pressed))
            print(datetime.now(), 'thread2 x_plfm_current: %d\tx_move%d\tx_plfm_moveTo%d'%(x_platform_current, x_move, x_platform_moveTo))
            print(datetime.now(), 'thread2 x_platform_current:%d\ty_platform_current:%d\tpressed:%d' % (x_platform_current, y_platform_current, pressed))
            #print(datetime.now(),'thread2 x_move:\t%d\ty_move:%d\tpressed:%d' % (x_move, y_move, pressed))
            #print(datetime.now(),'thread2 x_platform_moveTo:\t', x_platform_moveTo, 'y_platform_moveTo:\t',y_platform_moveTo)

            if ((x_platform_current > -1 and x_platform_current <= 500)):
                if (((y_platform_current > -1 and y_platform_current < 500))):
                    # command_tmp = \
                    #     "X" + str(xAxisPlatform_Filter.average_filter(round(x_platform_moveTo, 0 ))) + \
                    #     ",Y" + str(yAxisPlatform_Filter.average_filter(round(y_platform_moveTo, 0))) + "\r\n"
                    # if (abs(x_move) > 2 or abs(y_move) > 2):
                        command_tmp = \
                            "X" + str(round(float(x_move),2)) + \
                            ",Y" + str(round(float(y_move),2)) + "\r\n"

                        #print(datetime.now(), "command_tmp:  ", command_tmp)
                        command(serClearCore, command_tmp)

        except KeyboardInterrupt:
            
            serClearCore.close()
            serXIAO.close()
            dev.reset()
            


def get_current_location():
    serXIAO.open()
    global x_platform_current
    global y_platform_current

    while (1):
        try:
            with lock2:

                serial_reading = serXIAO.readline()
                #print(serial_reading[-2])
                #if serial_reading != 0:
                if serial_reading[0] == 62:    # DEC number of ASCII Char ">"
                    if serial_reading[-2] == 60:
                        decoded_reading = serial_reading.decode("ascii")

                        x_platform_current = round(float(decoded_reading.split(':')[1]),2) #* 75.1 / 4095
                        y_platform_current = round(-float(decoded_reading.split(':')[3].split('<')[0]),2) # * 75.1 / 4095
                
                # print(datetime.now(), 'thread3 x_platform_current:%.2f\ty_platform_current:%.2f\tpressed:%d' % (x_platform_current, y_platform_current, pressed))
                # print(y_platform_current)
                # print(datetime.now(), 'thread3 x_platform_current:%d\ty_platform_current:%d\tpressed:%d' % (x_platform_current, y_platform_current, pressed))
                # print(decoded_reading)
            
        except KeyboardInterrupt:
            dev.reset()
            serClearCore.close()
            serXIAO.close()
            # usb.util.release_interface(dev, interface)

readpen = threading.Thread(target=readingPenData)
moveboard = threading.Thread(target=move_the_board)
getcurrent = threading.Thread(target=get_current_location)

# updatetarget = threading.Thread(target=update_target)
# test = threading.Thread(target=test_thread)

readpen.start()
moveboard.start()
getcurrent.start()
# updatetarget.start()
# getcurrent.start()
