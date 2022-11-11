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


# init
ser = serial.Serial('/dev/ttyACM0', 115200)  # Port definition
time.sleep(0.2)
ser.close()


def command(ser, command):
    start_time = datetime.now()
    # ser.write(str.encode(command))
    # time.sleep(1)

    while True:
        ser.write(str.encode(command))
        line = ser.readline()
        # print(line)

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

x_tar = 960     # next target x location. getting from svg file
y_tar = 960     # next target y location. getting from svg file
x_pre = 0       # default x location
y_pre = 0       # default y location
delta_x = 0     # change of the x location
delta_y = 0     # change of the y location
x_platform_current = 0   #
y_platform_current = 0   #
X_BOARD = 190   # acctuate size of the sensor detection in mm
Y_BOARD = 145   # acctuate size of the sensor detection in mm

y_platform_moveTo = 0


ser.open()
command(ser, "G0 X0 Y0 F5000 \r\n")
command(ser, "G0 X100 Y0 F5000 \r\n")
command(ser, "G0 X0 Y0 F5000 \r\n")
print("break point 4.5")
ser.close()



def readingPenData():
    global x_platform_moveTo

    collected = 0
    # attempts = 50
    # while (collected < attempts or 1) :
    while True:

        try:
            data = dev.read(endpoint.bEndpointAddress, endpoint.wMaxPacketSize)
            collected += 1
            x = data[1]+data[2]*256
            y = data[3]+data[4]*256
            pressed = data[5]

            x_pen = xAxis_Filter.average_filter(x)
            delta_x = x_pen - x_tar
            x_move = float(filter.mapping(delta_x, 0, 985, 0, 100))
            x_platform_moveTo = x_platform_current + x_move
        
    
            print('x:%d\ty:%d\tpressed:%d' % (x, y, pressed))
            #print 'x:'+x+'\ty:'+y+'\tpressed:'+pressed;
            print("Break point 5")
            print(collected)
        
        except usb.core.USBError as e:
            data = None
            if e.args == ('Operation timed out',):
                continue
    #release the device
    usb.util.release_interface(dev, interface)


def move_the_board():

    ser.open()
    while (1):
        try:
            #print("In While moveboard")
            #command(ser, "G0 X"+str(x_platform_moveTo)+ " F20000 \r\n")
            print("X platform moveto: ", x_platform_moveTo)
        except KeyboardInterrupt:
            ser.close()
            break
            

    ser.close()

x_platform_moveTo = 0
readpen = threading.Thread(target=readingPenData,args=(x_platform_moveTo,))
moveboard = threading.Thread(target=move_the_board)

readpen.start()

move_the_board()

readpen.join()


#moveboard.start()
