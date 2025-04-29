# 检测操作系统
UNAME_S := $(shell uname -s)

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I.

# 根据操作系统设置包含路径和库路径
ifeq ($(UNAME_S),Darwin)
    # macOS 设置
    # Homebrew 包路径
    HOMEBREW_PREFIX := $(shell brew --prefix)
    # 包含路径
    CXXFLAGS += -I$(HOMEBREW_PREFIX)/include \
                -I$(HOMEBREW_PREFIX)/Cellar/nlohmann-json/3.12.0/include
    # 库路径
    LIBS = -L$(HOMEBREW_PREFIX)/lib
else
    # Linux (Debian/Ubuntu) 设置
    CXXFLAGS += -I/usr/include/nlohmann
    LIBS = -L/usr/lib
endif

# 目标文件
TARGET = json2Route_test

# 源文件
SRCS = json2Route.cpp json2Route_test.cpp

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

# 通用编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 运行目标
run: $(TARGET)
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
	@echo "  run      - 运行程序"
	@echo "  clean    - 清理编译文件"
	@echo "  rebuild  - 重新编译项目"
	@echo "  help     - 显示此帮助信息"
	@echo ""
	@echo "当前操作系统: $(UNAME_S)"
	@echo "Homebrew 前缀: $(HOMEBREW_PREFIX)"
	@echo "包含路径: $(CXXFLAGS)"
	@echo "库路径: $(LIBS)"

.PHONY: all run clean rebuild help 