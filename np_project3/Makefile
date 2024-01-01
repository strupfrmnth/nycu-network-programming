CC = g++
CFLAGS = -std=c++11 -Wall -pedantic -pthread -lboost_system
TARGET1 = http_server
TARGET2 = console
TARGET1_DIR = ./np_http_server
TARGET2_DIR = ./np_console
LIB_DIR = lib
SRC_DIR = src
OUT_DIR = obj
SRCS1 = $(wildcard $(TARGET1_DIR)/$(SRC_DIR)/*.cpp)
SRCS2 = $(wildcard $(TARGET2_DIR)/$(SRC_DIR)/*.cpp)
OBJS1 = $(SRCS1:$(TARGET1_DIR)/$(SRC_DIR)/%.cpp=$(TARGET1_DIR)/$(OUT_DIR)/%.o) $(TARGET1_DIR)/$(OUT_DIR)/$(TARGET1).o
OBJS2 = $(SRCS2:$(TARGET2_DIR)/$(SRC_DIR)/%.cpp=$(TARGET2_DIR)/$(OUT_DIR)/%.o) $(TARGET2_DIR)/$(OUT_DIR)/$(TARGET2).o

.PHONY: clean

all: $(TARGET1) $(TARGET2).cgi

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1) -pedantic -pthread -lboost_system

$(TARGET2).cgi: $(OBJS2)
	$(CC) -o $(TARGET2).cgi $(OBJS2) -pedantic -pthread -lboost_system

$(TARGET1_DIR)/$(OUT_DIR)/$(TARGET1).o: $(TARGET1).cpp
	@mkdir -p $(dir $@)
	$(CC) -I./ $(CFLAGS) -c $(TARGET1).cpp -o $@

$(TARGET2_DIR)/$(OUT_DIR)/$(TARGET2).o: $(TARGET2).cpp
	@mkdir -p $(dir $@)
	$(CC) -I./ $(CFLAGS) -c $(TARGET2).cpp -o $@

$(TARGET1_DIR)/$(OUT_DIR)/%.o: $(TARGET1_DIR)/$(SRC_DIR)/%.cpp $(TARGET1_DIR)/$(LIB_DIR)/%.h
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET1_DIR)/$(LIB_DIR) $(CFLAGS) -c $< -o $@

$(TARGET2_DIR)/$(OUT_DIR)/%.o: $(TARGET2_DIR)/$(SRC_DIR)/%.cpp $(TARGET2_DIR)/$(LIB_DIR)/%.h
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET2_DIR)/$(LIB_DIR) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET1_DIR)/$(OUT_DIR)
	rm -rf $(TARGET2_DIR)/$(OUT_DIR)
	rm $(TARGET1)
	rm $(TARGET2).cgi