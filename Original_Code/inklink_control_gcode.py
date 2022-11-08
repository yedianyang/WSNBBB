import serial
import time
from datetime import datetime

import sys
import usb.core
import usb.util



def translate(value, leftMin, leftMax, rightMin, rightMax):
    # Figure out how 'wide' each range is
    leftSpan = leftMax - leftMin
    rightSpan = rightMax - rightMin

    # Convert the left range into a 0-1 range (float)
    valueScaled = float(value - leftMin) / float(leftSpan)

    # Convert the 0-1 range into a value in the right range.
    return rightMin + (valueScaled * rightSpan)


def command(ser, command):
  start_time = datetime.now()
  ser.write(str.encode(command)) 
  #time.sleep(1)

  while True:
    line = ser.readline()
    print(line)

    if line == b'ok\n':
      break

ser = serial.Serial('/dev/ttyACM0', 115200) #Port definition
time.sleep(0.1)
ser.close()


dev = usb.core.find(idVendor=0x056A, idProduct=0x0221)

#All the inklink data feeding data
# first endpoint
interface = 0
#usb.util.release_interface(dev, interface)

endpoint = dev[0][(0,0)][0]

print ("ENDPOINT"+str(endpoint.bEndpointAddress))
print("Break point 1")
if dev.is_kernel_driver_active(interface) is True:
  # tell the kernel to detach
  dev.detach_kernel_driver(interface)
  # claim the device
  usb.util.claim_interface(dev, interface)

USBRQ_HID_GET_REPORT = 0x01;
USBRQ_HID_SET_REPORT = 0x09;

buf=[0x80,0x01,0x03,0x01,0x01];
buf.extend([0]*(33-len(buf)));
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)
#lack of this will using digitizer as output


buf=[0x80,0x01,0x0A,0x01,0x01,0x0B,0x01];
buf.extend([0]*(33-len(buf)));
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

print (dev.ctrl_transfer(0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, data_or_wLength=33))
print("Break point 2")
buf=[0x80,0x01,0x0B,0x01];
buf.extend([0]*(33-len(buf)));
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

buf=[0x80,0x01,0x02,0x01,0x01];
buf.extend([0]*(33-len(buf)));
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

buf=[0x80,0x01,0x0A,0x01,0x01,0x02,0x01];
buf.extend([0]*(33-len(buf)));
dev.ctrl_transfer(0x21, USBRQ_HID_SET_REPORT, 0x0380, 0, data_or_wLength=buf)

print (dev.ctrl_transfer(0xA1, USBRQ_HID_GET_REPORT, 0x0380, 0, data_or_wLength=33))
print("Break point 3")
print ("\n\ndata")
print("Break point 4")


# if the OS kernel already claimed the device, which is most likely true
# thanks to http://stackoverflow.com/questions/8218683/pyusb-cannot-set-configurati
collected = 0
attempts = 50

x_tar = 0 #next target x location. getting from svg file
y_tar = 0 #next target y location. getting from svg file
x_pre = 960 #default x location
y_pre = 960 #default y location
delta_x = 0 #change of the x location
delta_y = 0 #change of the y location
x_current = 0
y_current = 0

ser.open()

i=0
while (collected < attempts or 1) :
    try:
        data = dev.read(endpoint.bEndpointAddress,endpoint.wMaxPacketSize)
        collected += 1
        x=data[1]+data[2]*256
        y=data[3]+data[4]*256
        pressed=data[5]
        #print ('x:%d\ty:%d\tpressed:%d' % (x,y,pressed))
        #print 'x:'+x+'\ty:'+y+'\tpressed:'+pressed;

        # 业务代码 start
        delta_x = x - x_pre
        delta_y = y - y_pre
        

        x_pre = x
        y_pre = y

        print("delta_x:%d",delta_x)
        
            
        tmp_x = translate(x,0,1920,40,210)
        tmp_y = translate(y,0,1920,40,210)

        #command(ser, "G1  X" + str(delta_x) + " F20000 \r\n") #Y" + str(delta_x) + "
       
        command(ser, "G1 X"+str(-0.1*i)+" F20000 \r\n")
        i += 1

        #ser.close() 
        print ('x:%d\ty:%d\tpressed:%d' % (x,y,pressed))
        print ('mapped_x:%d\tmapped_y:%d\tpressed:%d' % (tmp_x,tmp_y,pressed))
        
        # 业务代码 end
        #print("Break point 5")
        print ("Connected:%d index: %d", collected, i)


        if(i > 60):
          #command(ser, "G1 X-138 F20000 \r\n")
          break
        
    except usb.core.USBError as e:
        data = None
        if e.args == ('Operation timed out',):
            continue
# release the device
#usb.util.release_interface(dev, interface)
ser.close() 
