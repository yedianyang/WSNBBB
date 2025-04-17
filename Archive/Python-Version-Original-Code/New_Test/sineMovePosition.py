import math
import matplotlib.pyplot as plt
import time
# from threading import Thread, Event
# import threading

# 创建一个列表来存储坐标
x_values = []
y_values = []

x_aimPoint = 0
y_aimPoint = 0

# 定义步长，如0.1
step = 0.01

# 通过改变x的值从0到2π，计算对应的sin(x)值
x = 0
while x < 2*math.pi:
    y = (math.sin(x)) * 40 + 50
    x_values.append(x)
    y_values.append(y)
    x += step


def positionUpdater():
    global x_aimPoint
    global y_aimPoint
    # 列表的索引
    index = 0
    # 记录开始时间
    start_time = time.time()
    # 保存上一次循环的时间
    last_loop_time = start_time

    while index < len(y_values):
        current_time = time.time()  # 获取当前时间
        elapsed_time = current_time - last_loop_time  # 计算经过的时间

        if elapsed_time > 0.04:  # 10ms
            #print(index, y_values[index])

            x_aimPoint = 5
            y_aimPoint = y_values[index]
            index += 2
            last_loop_time = current_time  # 更新上次循环的时间
            #print(y_aimPoint)
        
        index = index % len(y_values)
        


