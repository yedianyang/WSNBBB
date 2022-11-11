import threading
from datetime import datetime
import time


x_platform_moveTo =0

# def thread_1():
#     global x_platform_moveTo

#     while(1):
#         try:
#             x_platform_moveTo += 1
#             print(datetime.now(), "threading 1", x_platform_moveTo)
#         except KeyboardInterrupt:
#             break

# def thread_2():
#     global x_platform_moveTo
#     while(1):
#         try:
#             #time.sleep(0.01)
#             print(datetime.now(), " threading 2 " ,x_platform_moveTo)
#         except KeyboardInterrupt:
#             break

def thread_1():
    global x_platform_moveTo

   
    x_platform_moveTo += 1
    print(datetime.now(), "threading 1", x_platform_moveTo)

def thread_2():

    #time.sleep(0.01)
    print(datetime.now(), " threading 2 " ,x_platform_moveTo)



threading1 = threading.Thread(target=thread_1)
threading2 = threading.Thread(target=thread_2)

threading1.start()
threading2.start()


#threading2.join()



#threading1.join()

