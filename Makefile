# Makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -I/usr/include/libusb-1.0 -I./Library/ClearPath-SC-Development/include
LDFLAGS = -L/usr/lib -L/usr/local/lib -lusb-1.0 -lsFoundation20 -lpthread

# 源文件
SOURCES = cpp-Version/main.cpp cpp-Version/WacomInkling.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = inkling_control

# 默认目标
all: $(TARGET)

# 链接
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET)
