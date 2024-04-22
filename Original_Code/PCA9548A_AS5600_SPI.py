import spidev
import time
import struct

spi = spidev.SpiDev()
spi.open(0,0)
spi.max_speed_hz = 500000

START_BYTE = 0xAA
END_BYTE = 0x55

def readFloats():
    rawData = spi.readbytes(2 + 2 * 4)  # Start + End bytes + 2 floats
    print(rawData)
    if rawData[0] != START_BYTE or rawData[-1] != END_BYTE:
        print("Data format error!")
        return None, None

    float1 = struct.unpack('f', bytes(rawData[1:5]))[0]
    float2 = struct.unpack('f', bytes(rawData[5:9]))[0]
    return float1, float2

while True:
    float1, float2 = readFloats()
    if float1 is not None and float2 is not None:
        print("Float 1:", float1, "Float 2:", float2)
    time.sleep(1)