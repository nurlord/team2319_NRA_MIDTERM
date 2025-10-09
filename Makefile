CXX = g++
CC  = gcc

CXXFLAGS = -Iinclude -Wall -Wextra -std=c++17 -g
CFLAGS   = -Iinclude -Wall -Wextra -std=c11 -g

LDFLAGS = -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl

TARGET = game

# Find all .cpp and .c files in subfolders
SRC_CPP := $(shell find src -name "*.cpp")
SRC_C   := $(shell find src -name "*.c")
OBJ     := $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)
