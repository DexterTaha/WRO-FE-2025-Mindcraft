# Ultra Simple Example Makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -I../sdk/include -I../sdk/src
LDFLAGS  = ../sdk/librplidar.a -lpthread -lstdc++

SRC = main.cpp
TARGET = ultra_simple

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

