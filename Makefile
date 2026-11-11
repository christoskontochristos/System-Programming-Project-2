CC = gcc
CFLAGS = -g
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

#list of source files (excluding jobCommander.c and jobExecutorServer.c)
SRC = $(filter-out $(SRC_DIR)/jobCommander.c $(SRC_DIR)/jobExecutorServer.c, $(wildcard $(SRC_DIR)/*.c))
#object files for the above source files
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SRC))
#headers
HEADERS = $(wildcard $(INC_DIR)/*.h)

all: $(BIN_DIR)/jobExecutorServer $(BIN_DIR)/jobCommander

$(BIN_DIR)/jobExecutorServer: $(BIN_DIR)/jobExecutorServer.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

$(BIN_DIR)/jobCommander: $(BIN_DIR)/jobCommander.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ -I$(INC_DIR)

clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/jobExecutorServer $(BIN_DIR)/jobCommander

.PHONY: all clean



# CC = gcc
# CFLAGS = -g
# SRC_DIR = src
# INC_DIR = include
# BIN_DIR = bin

# SRC = $(wildcard $(SRC_DIR)/*.c)
# OBJ = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SRC))
# HEADERS = $(wildcard $(INC_DIR)/*.h)

# all: $(BIN_DIR)/jobExecutorServer $(BIN_DIR)/jobCommander

# $(BIN_DIR)/jobExecutorServer: $(OBJ)
# 	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

# $(BIN_DIR)/jobCommander: $(OBJ)
# 	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lm

# $(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
# 	$(CC) $(CFLAGS) -c $< -o $@ -I$(INC_DIR)

# clean:
# 	rm -f $(OBJ) $(BIN_DIR)/jobExecutorServer $(BIN_DIR)/jobCommander

# .PHONY: all clean






# CC = gcc

# CFLAGS = -g 

# SRC = buffer.c threads.c commands.c utility.c jobCommander.c jobExecutorServer.c

# OBJ = buffer.o threads.o commands.o utility.o jobExecutorServer.o jobCommander.o

# HEADERS = buffer.h Info.h threads.h commands.h utility.h jobinfo.h

# all: jobExecutorServer jobCommander

# jobExecutorServer: buffer.o threads.o commands.o utility.o jobExecutorServer.o
# 	$(CC) $(CFLAGS) -o $@ buffer.o threads.o commands.o utility.o jobExecutorServer.o -lpthread -lm

# jobCommander: buffer.o threads.o commands.o utility.o jobCommander.o
# 	$(CC) $(CFLAGS) -o $@ buffer.o threads.o commands.o utility.o jobCommander.o -lpthread -lm

# %.o: %.c $(HEADERS)
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJ) jobExecutorServer jobCommander

# .PHONY: all clean