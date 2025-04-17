import smbus
import time

class AS5600:
    PCA9548A_ADDR = 0x70  # 根据A0, A1, A2的连接情况进行调整
    AS5600_ADDR = 0x36  # AS5600的默认地址
    AS5600_ANGLE = 0x0E  # 用于获取角度的寄存器地址

    def __init__(self, channel, bus_id=1):
        self.channel = channel
        self.bus = smbus.SMBus(bus_id)
        self._lastPosition = self._read_angle()
        self._position = 0

    def _select_channel(self):
        self.bus.write_byte(self.PCA9548A_ADDR, 1 << self.channel)

    def _read_angle(self):
        self._select_channel()
        data = self.bus.read_i2c_block_data(self.AS5600_ADDR, self.AS5600_ANGLE, 2)
        angle = (data[0] << 8) + data[1]
        return angle

    def getCumulativePosition(self):
        value = self._read_angle()

        # 整圈顺时针旋转？
        if self._lastPosition > 2048 and value < self._lastPosition - 2048:
            self._position += 4096 - self._lastPosition + value
        # 整圈逆时针旋转？
        elif value > 2048 and self._lastPosition < value - 2048:
            self._position -= 4096 - self._lastPosition + value
        else:
            self._position += value - self._lastPosition

        self._lastPosition = value
        return self._position
    


sensor1 = AS5600(channel=4)
sensor2 = AS5600(channel=2)

while True:
    print(f"Sensor 1 Cumulative Angle: {sensor1.getCumulativePosition()}")
    print(f"Sensor 2 Cumulative Angle: {sensor2.getCumulativePosition()}")
    time.sleep(0.1)