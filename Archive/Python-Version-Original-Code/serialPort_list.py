import serial.tools.list_ports

# 获取所有可用的串口设备
ports = serial.tools.list_ports.comports()

serXiaoAddress = ""
serClearCoreAddress = ""

# 遍历并打印每个串口设备的信息
for port in ports:
    print(f"设备名：{port.device}\t描述：{port.description}\t硬件ID：{port.hwid}")
    if("ClearCore" in port.description):
        serClearCoreAddress = port.device
    if("XIAO" in port.description):
        serXiaoAddress = port.device


print("ClearCore Address:  ", serClearCoreAddress)
print("XIAO Address:  ", serXiaoAddress)