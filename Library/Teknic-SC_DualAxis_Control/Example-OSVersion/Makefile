# Compiler settings
CXX = g++
CXXFLAGS = -Wall -std=c++11

# Target executable name
TARGET = CppVersionCheck

# Source files
SOURCES = Cpp-Version-Check.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Build all targets by default
all: $(TARGET)

# Link the program
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean up build artifacts
clean:
	rm -f *.o $(TARGET)

