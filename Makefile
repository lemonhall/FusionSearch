# Makefile for FusionSearch C Search Engine

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

# Source files
SOURCES = src/main.c src/trie.c src/tokenizer.c src/index.c src/search.c src/utils.c
HEADERS = include/trie.h include/tokenizer.h include/index.h include/search.h include/utils.h

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Output
TARGET = search_engine

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned build artifacts"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run with debug info
debug: CFLAGS += -DDEBUG
debug: clean $(TARGET) run

# Rebuild everything
rebuild: clean all

.PHONY: all clean run debug rebuild
