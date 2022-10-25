#!/usr/bin/python
import sys
import usb.core
import usb.util
# decimal vendor and product values
#dev = usb.core.find(idVendor=1118, idProduct=1917)
# or, uncomment the next line to search instead by the hexidecimal equivalent
dev = usb.core.find(idVendor=0x056A, idProduct=0x0221)
# first endpoint
interface = 0
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
while (collected < attempts or 1) :
    try:
        data = dev.read(endpoint.bEndpointAddress,endpoint.wMaxPacketSize)
        collected += 1
        x=data[1]+data[2]*256;
        y=data[3]+data[4]*256;
        pressed=data[5]
        print ('x:%d\ty:%d\tpressed:%d' % (x,y,pressed))
#        #print 'x:'+x+'\ty:'+y+'\tpressed:'+pressed;
        print("Break point 5")
        print (collected)
    except usb.core.USBError as e:
        data = None
        if e.args == ('Operation timed out',):
            continue
# release the device
usb.util.release_interface(dev, interface)

