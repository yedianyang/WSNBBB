import serial
import time
from datetime import datetime

import sys
import usb.core
import usb.util
from datetime import datetime


# function for transl
def translate(value, leftMin, leftMax, rightMin, rightMax):
    # Figure out how 'wide' each range is
    leftSpan = leftMax - leftMin
    rightSpan = rightMax - rightMin

    # Convert the left range into a 0-1 range (float)
    valueScaled = float(value - leftMin) / float(leftSpan)

    # Convert the 0-1 range into a value in the right range.
    return float(rightMin + (valueScaled * rightSpan))


def command(ser, command):
    start_time = datetime.now()
    ser.write(str.encode(command))
    # time.sleep(0.1)

    # while True:
    #     line = ser.readline()
    #     print(line)

    #     if line == b'ok\n':
    #         break


data_filter = [0, 0, 0, 0]


def lowpassFilter(signalIn):
    data_filter.pop(len(data_filter)-1)
    data_filter.insert(0, signalIn)
    return sum(data_filter)/len(data_filter)


ser = serial.Serial('/dev/ttyACM0', 115200)  # Port definition
time.sleep(0.1)
ser.close()


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
print("Break point 3")
print("\n\ndata")
print("Break point 4")


# if the OS kernel already claimed the device, which is most likely true
# thanks to http://stackoverflow.com/questions/8218683/pyusb-cannot-set-configurati
collected = 0
attempts = 50

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


ser.open()
command(ser, "G0 X0 F5000 \r\n")
command(ser, "G0 X100 F5000 \r\n")
command(ser, "G0 X0 F5000 \r\n")
command(ser, "M400 \r\n")
print("break point 4.5")

while (collected < attempts or 1):
    try:
        time1 = time.time()
        data = dev.read(endpoint.bEndpointAddress, endpoint.wMaxPacketSize)
        collected += 1
        x = data[1]+data[2]*256
        y = data[3]+data[4]*256
        pressed = data[5]
        time2 = time.time()
        # print("data list" , data)
        # print ('x:%d\ty:%d\tpressed:%d' % (x,y,pressed))
        # print 'x:'+x+'\ty:'+y+'\tpressed:'+pressed;
        x_pen = lowpassFilter(x)
        y_pen = y
        # 业务代码 start
        # x_pre = x_pen
        # y_pre = y_pen

        delta_x = x_pen - x_tar
        delta_y = y_pen - y_tar
        # number base of calibration
        x_move = float(translate(delta_x, 0, 1920, 0, 240))
        y_move = float(translate(delta_y, 0, 1920, 0, Y_BOARD))

        # update all the from relative to abs

        if (abs(x_move) > 5):

            x_platform_moveTo = x_platform_current + x_move
            y_platform_moveTo = y_platform_current + y_move
            # command(ser, "G1 X"+str(x_move)+ " Y"+ str(y_move) + " F20000 \r\n")
            if (x_platform_moveTo > 0 and x_platform_moveTo <= 240):
                # print("Loop Begin")
                command_tmp = "G0 X" + \
                    str(round(x_platform_moveTo, 2)) + " F20000 \r\n"
                print(datetime.now(), "G0 X" +
                      str(round(x_platform_moveTo, 2)) + " F20000")

                command(ser, command_tmp)
                #command(ser, "M400 \r\n")

                # print(datetime.now(),"x_platform_moveTo:   %d" % x_platform_moveTo)
                x_platform_current = x_platform_moveTo

                print(datetime.now(), 'x_pen:%d\ty_pen:%d\tpressed:%d' %
                    (x_pen, y_pen, pressed))
                print(datetime.now(), 'delta_x:%d\tdelta_y:%d\tpressed:%d' %
                    (delta_x, delta_y, pressed))
                print(datetime.now(), 'x_move:%d\ty_move:%d\tpressed:%d' %
                     (x_move, y_move, pressed))
            if (time2 - time1 > 0.1):
                print(datetime.now(), "pen thread x delay ", time2 - time1)
        

        # 业务代码 end
        # print("Break point 5")
        # print("Connected: ", collected)
        # print("Loop End \n")
    except usb.core.USBError as e:
        data = None
        if e.args == ('Operation timed out',):
            continue
# release the device
# usb.util.release_interface(dev, interface)
ser.close()
