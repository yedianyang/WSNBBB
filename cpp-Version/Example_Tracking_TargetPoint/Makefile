# Makefile for Example-Tracking_TargetPoint
# Creates an executable called tracking_example_targetpoint

EXEC_NAME := "tracking_example_targetpoint"

# Detect operating system and set appropriate include path
ifeq ($(shell uname), Darwin)
    # macOS specific settings
    INCLUDE_DIR := "../Library/ClearPath-SC-Development/Reference/sdk/sFoundation Source/inc/inc-pub"
    LIBUSB_INCLUDE := "/opt/homebrew/opt/libusb/include"
    LIBUSB_LIB := "/opt/homebrew/opt/libusb/lib"
    LIBS := -lsFoundation20 -lpthread -L$(LIBUSB_LIB) -lusb-1.0
    CXXFLAGS := -I$(INCLUDE_DIR) -I$(LIBUSB_INCLUDE) -I. $(OPTIMIZATION) -std=c++11
    # For macOS, we use DYLD_LIBRARY_PATH instead of RPATH
    LDFLAGS := -L$(LIBUSB_LIB)
else ifeq ($(shell uname), Linux)
    # Linux (Raspberry Pi) specific settings
    INCLUDE_DIR := "~/src/Teknic/inc/inc-pub"
    LIBS := -lsFoundation20 -lpthread -lusb-1.0 -latomic
    CXXFLAGS := -I$(INCLUDE_DIR) -I. $(OPTIMIZATION)
else
    $(error Unsupported operating system)
endif

CC := g++
OPTIMIZATION := -O3
DEBUG_OPTIMIZATION := -O0
DEBUG_CXXFLAGS := $(CXXFLAGS) -Wall -Wextra -pedantic -g3 $(DEBUG_OPTIMIZATION)

# Specify source files here
SRC_FILES := Example-Tracking_TargetPoint.cpp WacomInkling.cpp
OBJS := $(patsubst %.cpp,%.o,$(SRC_FILES))

# Default target
all: $(EXEC_NAME)

# Generic rule to compile a CPP file into an object
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o "$@" $<

$(EXEC_NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $(EXEC_NAME) $(OBJS) $(LIBS)

# Remove all object files
.PHONY: clean
clean:
	-find . -type f -name "*.o" -delete

# Sayonara. Viciously destroys all build artifacts, including the executable.
.PHONY: real_clean
real_clean: clean
	-rm $(EXEC_NAME) 