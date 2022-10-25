import usb.core
dev = usb.core.find(idVendor=0x056a, idProduct=0x0221)

endpoint = dev[0][(0,0)][0]

print ("ENDPOINT"+str(endpoint.bEndpointAddress))