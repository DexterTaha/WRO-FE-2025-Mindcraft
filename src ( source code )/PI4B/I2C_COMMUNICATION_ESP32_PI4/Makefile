# Makefile for controll.cpp

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -O2

# Target executable
TARGET = i2c_esp32 

# Source files
SRCS = main.cpp

# Default target
all: $(TARGET)

# Link
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Clean
clean:
	rm -f $(TARGET) *.o

# Phony targets
.PHONY: all clean

