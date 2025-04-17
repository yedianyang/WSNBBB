import usb.core
import usb.util
import serial.tools.list_ports

# Find device by VID and PID
dev = usb.core.find(idVendor=0x303A, idProduct=0x1001)

if dev is None:
    raise ValueError('Device not found')

# Get the device description
device_desc = usb.util.get_string(dev, dev.iProduct)

# Search serial ports
ports = list(serial.tools.list_ports.grep(device_desc))

# Print the ports
for port in ports:
    print(port)