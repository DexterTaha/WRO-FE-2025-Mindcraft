# C++ compiler
CXX = g++
CXXFLAGS = -std=c++11 -Wall `pkg-config --cflags opencv4`
LDFLAGS  = -lpthread `pkg-config --libs opencv4`

SRC = main.cpp
TARGET = dashboard 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

