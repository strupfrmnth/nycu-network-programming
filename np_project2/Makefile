CC = g++
CFLAGS = -std=c++11 -Wall
TARGET1 = np_simple
TARGET2 = np_single_proc
TARGET3 = np_multi_proc
TARGET1_DIR = ./simple
TARGET2_DIR = ./single_proc
TARGET3_DIR = ./multi_proc
LIB_DIR = lib
SRC_DIR = src
OUT_DIR = obj
SRCS1 = $(wildcard $(TARGET1_DIR)/$(SRC_DIR)/*.cpp)
SRCS2 = $(wildcard $(TARGET2_DIR)/$(SRC_DIR)/*.cpp)
SRCS3 = $(wildcard $(TARGET3_DIR)/$(SRC_DIR)/*.cpp)
OBJS1 = $(SRCS1:$(TARGET1_DIR)/$(SRC_DIR)/%.cpp=$(TARGET1_DIR)/$(OUT_DIR)/%.o) $(TARGET1_DIR)/$(OUT_DIR)/$(TARGET1).o
OBJS2 = $(SRCS2:$(TARGET2_DIR)/$(SRC_DIR)/%.cpp=$(TARGET2_DIR)/$(OUT_DIR)/%.o) $(TARGET2_DIR)/$(OUT_DIR)/$(TARGET2).o
OBJS3 = $(SRCS3:$(TARGET3_DIR)/$(SRC_DIR)/%.cpp=$(TARGET3_DIR)/$(OUT_DIR)/%.o) $(TARGET3_DIR)/$(OUT_DIR)/$(TARGET3).o

.PHONY: clean

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1)

$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2)

$(TARGET3): $(OBJS3)
	$(CC) -o $(TARGET3) $(OBJS3)

$(TARGET1_DIR)/$(OUT_DIR)/$(TARGET1).o: $(TARGET1).cpp
	@mkdir -p $(dir $@)
	$(CC) -I./ $(CFLAGS) -c $(TARGET1).cpp -o $@

$(TARGET2_DIR)/$(OUT_DIR)/$(TARGET2).o: $(TARGET2).cpp
	@mkdir -p $(dir $@)
	$(CC) -I./ $(CFLAGS) -c $(TARGET2).cpp -o $@

$(TARGET3_DIR)/$(OUT_DIR)/$(TARGET3).o: $(TARGET3).cpp
	@mkdir -p $(dir $@)
	$(CC) -I./ $(CFLAGS) -c $(TARGET3).cpp -o $@

$(TARGET1_DIR)/$(OUT_DIR)/%.o: $(TARGET1_DIR)/$(SRC_DIR)/%.cpp $(TARGET1_DIR)/$(LIB_DIR)/%.h
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET1_DIR)/$(LIB_DIR) $(CFLAGS) -c $< -o $@

$(TARGET2_DIR)/$(OUT_DIR)/%.o: $(TARGET2_DIR)/$(SRC_DIR)/%.cpp $(TARGET2_DIR)/$(LIB_DIR)/%.h
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET2_DIR)/$(LIB_DIR) $(CFLAGS) -c $< -o $@

$(TARGET3_DIR)/$(OUT_DIR)/%.o: $(TARGET3_DIR)/$(SRC_DIR)/%.cpp $(TARGET3_DIR)/$(LIB_DIR)/%.h
	@mkdir -p $(dir $@)
	$(CC) -I$(TARGET3_DIR)/$(LIB_DIR) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET1_DIR)/$(OUT_DIR)
	rm -rf $(TARGET2_DIR)/$(OUT_DIR)
	rm -rf $(TARGET3_DIR)/$(OUT_DIR)
	rm $(TARGET1)
	rm $(TARGET2)
	rm $(TARGET3)