# Makefile for FusionSearch C Search Engine

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude -D_POSIX_C_SOURCE=199309L
LDFLAGS = -lm

# ICU support (optional)
# Detect if ICU is available
ICU_AVAILABLE := $(shell pkg-config --exists icu-uc icu-i18n 2>/dev/null && echo 1 || echo 0)

ifeq ($(ICU_AVAILABLE),1)
    ICU_CFLAGS := $(shell pkg-config --cflags icu-uc icu-i18n)
    ICU_LIBS := $(shell pkg-config --libs icu-uc icu-i18n)
    CFLAGS += $(ICU_CFLAGS) -DENABLE_ICU
    LDFLAGS += $(ICU_LIBS)
    $(info ✓ ICU support enabled)
else
    $(info ⚠ ICU not found - CJK tokenizer will be disabled)
    $(info   Install: sudo apt-get install libicu-dev)
endif

# Source files
SOURCES = src/main.c src/trie.c src/tokenizer.c src/index.c src/search.c src/bm25.c src/snippet.c src/file_loader.c src/cjk_tokenizer.c src/vector_index.c src/utils.c
TEST_SOURCES = src/test_suite.c src/trie.c src/tokenizer.c src/index.c src/search.c src/bm25.c src/snippet.c src/file_loader.c src/cjk_tokenizer.c src/vector_index.c src/utils.c src/test.c
HEADERS = include/trie.h include/tokenizer.h include/index.h include/search.h include/bm25.h include/snippet.h include/file_loader.h include/cjk_tokenizer.h include/vector_index.h include/utils.h include/test.h

# Object files
OBJECTS = $(SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Output
TARGET = search_engine
TEST_TARGET = test_runner
LIB_TARGET = libfusion.so
DEMO_TARGET = demo_hybrid

# Default target
all: $(TARGET)

# Build demo program
demo: $(DEMO_TARGET)

$(DEMO_TARGET): demo_hybrid.o $(filter-out src/main.o, $(OBJECTS))
	$(CC) demo_hybrid.o $(filter-out src/main.o, $(OBJECTS)) -o $(DEMO_TARGET) $(LDFLAGS)
	@echo "Build complete: $(DEMO_TARGET)"

# Build shared library for Python FFI
lib: $(LIB_TARGET)

$(LIB_TARGET): CFLAGS += -fPIC
$(LIB_TARGET): $(filter-out src/main.o, $(OBJECTS))
	$(CC) -shared $(filter-out src/main.o, $(OBJECTS)) -o $(LIB_TARGET) $(LDFLAGS)
	@echo "Build complete: $(LIB_TARGET)"

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
