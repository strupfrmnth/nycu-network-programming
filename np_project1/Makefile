CC = g++
CFLAGS = -std=c++11 -Wall
TARGET = npshell
LIB_DIR = ./lib
SRC_DIR = ./src
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# LIBS = $(wildcard $(LIB_DIR)/*.h)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=%.o) $(TARGET).o

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

$(TARGET).o: $(TARGET).cpp
	$(CC) -I./ $(CFLAGS) -c $(TARGET).cpp

%.o: $(SRC_DIR)/%.cpp $(LIB_DIR)/%.h
	$(CC) -I$(LIB_DIR) $(CFLAGS) -c $< -o $@

clean:
	rm *.o
	rm $(TARGET)