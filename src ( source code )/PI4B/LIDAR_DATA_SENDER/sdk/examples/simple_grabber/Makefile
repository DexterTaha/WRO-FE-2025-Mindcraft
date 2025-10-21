# Simple Grabber Example Makefile (fixed paths)
CXX = g++
CXXFLAGS = -std=c++11 -Wall -I../../sdk/include -I../../sdk/src
LDFLAGS  = ../../sdk/librplidar.a -lpthread -lstdc++

SRC = main.cpp
TARGET = simple_grabber

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

