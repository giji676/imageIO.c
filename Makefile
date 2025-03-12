# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I./src/bmp -I./src/png

# Directories
SRC_DIR = src
BUILD_DIR = build

# Find all source files in src/ and subdirectories
SRC = $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)

# Convert source file paths to object file paths in build/
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

# Target executable
TARGET = main

# Ensure build directory structure exists
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)  # Ensure directory structure exists
	$(CC) $(CFLAGS) -c $< -o $@

# Link all object files into the final executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Clean build directory and executable
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
