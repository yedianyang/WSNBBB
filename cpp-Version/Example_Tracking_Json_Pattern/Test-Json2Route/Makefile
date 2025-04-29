# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.

# 目标文件
TARGET = json2Route_test

# 源文件
SRCS = json2Route.cpp json2Route_test.cpp

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 依赖库
LIBS = -lgtest -lgtest_main -pthread

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

# 通用编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 测试目标
test: $(TARGET)
	./$(TARGET)

# 清理目标
clean:
	rm -f $(OBJS) $(TARGET)

# 重新编译
rebuild: clean all

# 帮助信息
help:
	@echo "可用的目标:"
	@echo "  all      - 编译项目"
	@echo "  test     - 运行测试"
	@echo "  clean    - 清理编译文件"
	@echo "  rebuild  - 重新编译项目"
	@echo "  help     - 显示此帮助信息"

.PHONY: all test clean rebuild help 