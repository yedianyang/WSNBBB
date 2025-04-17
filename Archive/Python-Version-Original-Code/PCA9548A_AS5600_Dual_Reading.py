import smbus

# Constants
PCA9548A_I2C_ADDRESS = 0x70  # Default I2C address for PCA9548A
AS5600_I2C_ADDRESS   = 0x36  # Default I2C address for AS5600
AS5600_ANGLE         = 0x0C  # ANGLE register address for AS5600

# Initialize I2C (use I2C bus 1 on Raspberry Pi)
bus = smbus.SMBus(1)

def set_pca9548a_channel(channel):
    """Set the desired channel on PCA9548A."""
    if 0 <= channel <= 7:
        bus.write_byte(PCA9548A_I2C_ADDRESS, 1 << channel)
    else:
        raise ValueError("PCA9548A channel out of range (0-7)")

def read_as5600_angle():
    """Read angle from AS5600."""
    data = bus.read_i2c_block_data(AS5600_I2C_ADDRESS, AS5600_ANGLE, 2)
    angle = (data[0] << 8) + data[1]
    return angle

def main():
    # Read data from the first AS5600
    set_pca9548a_channel(0)  # assuming the first AS5600 is connected to channel 0
    angle1 = read_as5600_angle()
    
    # Read data from the second AS5600
    set_pca9548a_channel(1)  # assuming the second AS5600 is connected to channel 1
    angle2 = read_as5600_angle()

    print(f"Angle from AS5600 #1: {angle1}")
    print(f"Angle from AS5600 #2: {angle2}")

if __name__ == "__main__":
    main()