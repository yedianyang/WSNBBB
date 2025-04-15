# Define the C++ compiler to use
CXX = g++

# Define any compile-time flags
CXXFLAGS = -Wall -g `pkg-config --cflags libusb-1.0`

# Define any directories containing header files other than /usr/include
INCLUDES =

# Define library paths in addition to /usr/lib
LFLAGS = `pkg-config --libs libusb-1.0`

# Define any libraries to link into executable:
LIBS =

# Define the C++ source files
SRCS = inklink-read.cpp

# Define the C++ object files
OBJS = $(SRCS:.cpp=.o)

# Define the executable file
MAIN = inklink-read

.PHONY: depend clean

all:    $(MAIN)
	@echo  Compiling has been completed.

$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# This is a suffix replacement rule for building .o's from .cpp's
# It uses automatic variables $<: the name of the prerequisite of
# the rule(a .cpp file) and $@: the name of the target of the rule (a .o file)
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
