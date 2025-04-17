# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra

# 源文件和目标文件
SRCS = Example-Motion.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = motion_example

# 默认目标
all: $(TARGET)

# 编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 链接规则
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 清理规则
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: all clean 