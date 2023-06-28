from threading import Thread, Event
import threading
import time
from datetime import datetime

import sys
import usb.core
import usb.util

import serial
import time
from datetime import datetime

import filter


xAxis_Filter = filter.Filter()
yAxis_Filter = filter.Filter()
xAxisPlatform_Filter = filter.Filter()
yAxisPlatform_Filter = filter.Filter()


# init
#ser = serial.Serial('/dev/ttyACM0', 115200)  # Port definition this one is for seeduino XIAO
ser = serial.Serial('/dev/ttyUSB0', 115200)   # This one is for ESP32
time.sleep(0.2)
ser.close()


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

STEP = 16                   # Micro step detail
X_BOARD = 190               # acctuate size of the sensor detection in mm
Y_BOARD = 145               # acctuate size of the sensor detection in mm
x_tar = 960                 # next target x location. getting from svg file
y_tar = 960                 # next target y location. getting from svg file
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


ser.open()

# new homing function

# command(ser, "G90 \r\n")
# command(ser, "G0 X0 Y0 F3000 \r\n")
# command(ser, "G0 X100 F3000 \r\n")
# command(ser, "G0 X0 F3000 \r\n")
print("break point 4")
command(ser, "homing\n")

print("break point 4.5")
ser.close()

lock = threading.Lock()


def readingPenData():
    global x
    global y
    global pressed

    collected = 0
    # attempts = 50
    # while (collected < attempts or 1) :

    while True:

        try:
            # 记录系统时间1
            with lock:
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
                #        (x, y, pressed))

                if (time2 - time1 > 0.1):

                    print(datetime.now(), "pen thread x delay ", time2 - time1)

            # y_platform_current = y_platform_moveTo

            # print("thread1 Loop Begin \n")
            #print(datetime.now(), 'thread1 x_pen:%d\ty_pen:%d\tpressed:%d' %
            #     (x, y, pressed))

            # print(datetime.now(), 'thread1 x_move:%d\ty_move:%d\tpressed:%d' %
            #     (x_move, y_move, pressed))
            # print(datetime.now(), 'thread1 x_curre:%d\ty_curre:%d\tpressed:%d' %
            #      (x_platform_moveTo, y_platform_moveTo, pressed))
            # 业务代码 end
            # print("Break point 5")
            # print("Connected: ", collected)
            # print("thread1 Loop End \n")
            # pdb.set_trace()
        except usb.core.USBError as e:
            data = None
            if e.args == ('Operation timed out',):
                continue
            # release the device


def move_the_board():
    x_platform_current = 0
    y_platform_current = 0
    x_current = 0
    y_current = 0
    ser.open()
    while (1):
        try:
            x_pen = x#xAxis_Filter.average_filter(x)
            delta_x = x_pen - x_tar
            # x_move = float(filter.mapping(delta_x, 0, 985, 0, 100))
            # mapping relationship need to check
            x_move = float(filter.mapping(delta_x, 0, 1920, 0, 193))

            # if (abs(x_move) > 5):
            x_platform_moveTo = x_platform_current + x_move

            y_pen = y#yAxis_Filter.average_filter(y)
            delta_y = y_pen - y_tar
            # y_move = float(filter.mapping(delta_y, 0, 1920, 0, 145))
            # mapping relationship need to check
            # Y axis is invert
            y_move = -float(filter.mapping(delta_y, 0, 1920, 0, 145))

            # if (abs(y_move) > 5):
            y_platform_moveTo = y_platform_current + y_move

            # print(datetime.now(), 'thread1 x_move:%d\ty_move:%d\tpressed:%d' %
            #       (x_move, y_move, pressed))

            # reading serial data from arduino, and getting thlse current position of the xy platform

            if ((x_platform_moveTo > 0 and x_platform_moveTo <= 240)):
                if (((y_platform_moveTo > 0 and y_platform_moveTo < 240))):
                    print("thread2 Loop Begin")
                    # pdb.set_trace()
                    # command_tmp = \
                    #     "X" + str(xAxisPlatform_Filter.average_filter(round(x_platform_moveTo, 0 ))) + \
                    #     ",Y" + str(yAxisPlatform_Filter.average_filter(round(y_platform_moveTo, 0))) + "\r\n"
                    command_tmp = \
                         "X" + str(int(x_platform_moveTo)) + \
                         ",Y" + str(int(y_platform_moveTo)) + "\r\n"

                    #print(datetime.now(),"command_tmp:  ", command_tmp)

                    command(ser, command_tmp)

            # This block is use for reading the comming current location
            serial_reading = ser.readline()
            if serial_reading != 0:
                if serial_reading != b'ok\n':
                    if serial_reading[0] == 67:
                        decoded_reading = serial_reading.decode("utf-8")
                        # print("raw_reading:   ",serial_reading)
                        #print("decoded_reading:   ", decoded_reading)
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

                # print(datetime.now(), "thread2 current: X %f Y %f  thread2 Loop End" %
                #        (x_platform_current, y_platform_current))

        except KeyboardInterrupt:
            ser.close()
            usb.util.release_interface(dev, interface)
            break

    ser.close()


readpen = threading.Thread(target=readingPenData)
moveboard = threading.Thread(target=move_the_board)
# test = threading.Thread(target=test_thread)


readpen.start()
moveboard.start()
# test.start()


# def test_thread():
#     timeStart = 0
#     prevX = 0
#     while True:
#         try:
#             time5 = time.time()
#             timeStart = time5
#             if (abs(x-prevX) > 10):

#                 print("test thread x  ", x)
#                 prevX = x
#             time6 = time.time()

#             if (time6-time5 > 0.01):
#                 print(datetime.now(), " thread3 delay: ", time6-time5)

#             timeEnd = time.time()
#             if (timeEnd - timeStart > 0.1):
#                 print("running time ", timeEnd - timeStart)
#             timeStart = timeEnd

#         except KeyboardInterrupt:

#             break
