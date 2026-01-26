# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I./src/bmp -I./src/png
LDFLAGS = -lX11

# Enable debug flags when DEBUG=1
ifeq ($(DEBUG),1)
    CFLAGS += -g -O0
endif

ifeq ($(PROFILE),1)
    CFLAGS += -g -O2 -pg
    LDFLAGS += -pg
endif

# Directories
SRC_DIR = src
BUILD_DIR = build

# Find all source files in src/ and subdirectories
SRC = $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)

# Convert source file paths to object file paths in build/
OBJ = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

# Target executable
TARGET = parser

# Ensure build directory structure exists
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link all object files into the final executable
$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

# Clean build directory and executable
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
