# Makefile for FusionSearch C Search Engine

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

# Source files
SOURCES = src/main.c src/trie.c src/tokenizer.c src/index.c src/search.c src/utils.c
TEST_SOURCES = src/test_suite.c src/trie.c src/tokenizer.c src/index.c src/search.c src/utils.c src/test.c
HEADERS = include/trie.h include/tokenizer.h include/index.h include/search.h include/utils.h include/test.h

# Object files
OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Output
TARGET = search_engine
TEST_TARGET = test_runner

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Build test runner
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CC) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "Build complete: $(TEST_TARGET)"

# Run tests
test-run: test
	./$(TEST_TARGET)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(TARGET) $(TEST_TARGET)
	@echo "Cleaned build artifacts"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run with debug info
debug: CFLAGS += -DDEBUG
debug: clean $(TARGET) run

# Rebuild everything
rebuild: clean all

.PHONY: all clean run debug rebuild test test-run
