# Compiler and flags
CC := gcc
CFLAGS := -Wall -g -std=c99 -Wfatal-errors

# Executable
EXEC := raylib_tutorial

# Directories
SRC_DIR := src
BIN_DIR := bin

# Include and linking flags
INCLUDES := -I ./libs/ -I ./src/
LIBRARIES := -lraylib

# Source files and object files
SOURCES := $(shell find $(SRC_DIR) -name '*.c')
OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)
DEPENDS := $(OBJECTS:.o=.d)

# Default target
all: build run

# Build target: compile and link the source files
build: $(BIN_DIR)/$(EXEC)

$(BIN_DIR)/$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(INCLUDES) $(LIBRARIES) -o $@

-include $(DEPENDS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -c $< -o $@

# Run target: run the executable
run: $(BIN_DIR)/$(EXEC)
	./$(BIN_DIR)/$(EXEC)

# Clean target: remove the compiled object files and executable
clean:
	rm -rf $(BIN_DIR)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

.PHONY: all build run clean
