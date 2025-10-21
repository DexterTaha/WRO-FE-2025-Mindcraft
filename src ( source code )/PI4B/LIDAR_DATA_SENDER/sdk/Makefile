# Compiler and flags
CXX = g++
CXXFLAGS = -O2 -std=c++17 -Iinclude -Isrc

# Target library
TARGET = librplidar.a

# Collect all source files
SRC = \
    $(wildcard src/*.cpp) \
    $(wildcard src/hal/*.cpp) \
    $(wildcard src/arch/linux/*.cpp) \
    $(wildcard src/dataunpacker/*.cpp) \
    $(wildcard src/dataunpacker/unpacker/*.cpp)

# Convert .cpp to .o
OBJS = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Build static library
$(TARGET): $(OBJS)
	ar rcs $@ $^

# Compile .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

