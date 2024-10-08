# Compiler and flags
CC = clang
INCDIR = ./bin/include
CFLAGS = -MMD -MP -Wall -Wextra -g -I./include -fsanitize=address,undefined -DINCDIR=\"$(INCDIR)\"
LDFLAGS = -fsanitize=address,undefined

# Directories
SRC_DIR = src
OBJ_DIR = obj
TARGET = bin/a

# Source files
SRC_FILES = $(filter-out $(SRC_DIR)/scanner.c, $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)) $(SRC_DIR)/scanner.c
# Corresponding object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Default target
all: $(TARGET)

# Rule to link object files into the final executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $(OBJ_FILES) -o $@

# Rule to compile each source file to an object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/scanner.c: $(SRC_DIR)/em.l
	flex -o $@ $<

# Include the dependency files
-include $(OBJ_FILES:.o=.d)

# Clean up build files
clean:
	rm -rf ./$(OBJ_DIR)/*.o ./$(OBJ_DIR)/*.d $(SRC_DIR)/scanner.c

.PHONY: all clean
