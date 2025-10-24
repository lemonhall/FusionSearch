# FusionSearch Test Suite Makefile
# 
# Complete test coverage for all modules

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -I../include
LDFLAGS = -lm

# Source directories
SRC_DIR = ../src
INCLUDE_DIR = ../include

# Common source files (all modules except main.c)
COMMON_SRCS = $(SRC_DIR)/trie.c \
              $(SRC_DIR)/tokenizer.c \
              $(SRC_DIR)/cjk_tokenizer.c \
              $(SRC_DIR)/index.c \
              $(SRC_DIR)/search.c \
              $(SRC_DIR)/bm25.c \
              $(SRC_DIR)/vector_index.c \
              $(SRC_DIR)/utils.c \
              $(SRC_DIR)/snippet.c \
              $(SRC_DIR)/file_loader.c \
              $(SRC_DIR)/test.c

COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Test executables
TESTS = test_trie \
        test_tokenizer \
        test_index \
        test_vector_index \
        test_search \
        test_bm25 \
        test_utils \
        test_integration

# Special test runners
RUNNERS = run_all_tests

.PHONY: all clean run help

# Default target
all: $(TESTS) $(RUNNERS)
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║   All tests compiled successfully!     ║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@echo "Run 'make run' to execute all tests"
	@echo ""

# Individual test targets
test_trie: test_trie.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_tokenizer: test_tokenizer.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_index: test_index.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_vector_index: test_vector_index.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_search: test_search.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_bm25: test_bm25.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_utils: test_utils.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_integration: test_integration.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run_all_tests: run_all_tests.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Pattern rule for test object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule for source object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run all tests
run: all
	@echo ""
	@echo "╔════════════════════════════════════════╗"
	@echo "║      Running FusionSearch Tests        ║"
	@echo "╚════════════════════════════════════════╝"
	@echo ""
	@./run_all_tests

# Run individual test suites
run-trie: test_trie
	@./test_trie

run-tokenizer: test_tokenizer
	@./test_tokenizer

run-index: test_index
	@./test_index

run-vector: test_vector_index
	@./test_vector_index

run-search: test_search
	@./test_search

run-bm25: test_bm25
	@./test_bm25

run-utils: test_utils
	@./test_utils

run-integration: test_integration
	@./test_integration

# Quick test (fastest tests only)
quick: test_trie test_tokenizer test_utils
	@echo "Running quick tests..."
	@./test_trie
	@./test_tokenizer
	@./test_utils

# Memory leak check (requires valgrind)
memcheck: all
	@echo "Running memory leak detection..."
	@for test in $(TESTS); do \
		echo "Checking $$test..."; \
		valgrind --leak-check=full --show-leak-kinds=all ./$$test 2>&1 | grep -E "ERROR SUMMARY|lost:"; \
	done

# Coverage report (requires gcov)
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov --coverage
coverage: clean all run
	@echo "Generating coverage report..."
	@gcov $(COMMON_SRCS)
	@echo "Coverage data written to *.gcov files"

# Clean build artifacts
clean:
	rm -f *.o $(SRC_DIR)/*.o
	rm -f $(TESTS) $(RUNNERS)
	rm -f *.gcda *.gcno *.gcov
	rm -f test_temp_file.txt
	@echo "Clean complete"

# Rebuild everything
rebuild: clean all

# Help message
help:
	@echo "FusionSearch Test Suite Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all tests"
	@echo "  make run          - Run all tests"
	@echo "  make quick        - Run quick tests only"
	@echo "  make run-<name>   - Run specific test (e.g., make run-trie)"
	@echo "  make memcheck     - Check for memory leaks (requires valgrind)"
	@echo "  make coverage     - Generate code coverage report (requires gcov)"
	@echo "  make clean        - Remove all build artifacts"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Individual tests:"
	@echo "  test_trie         - Trie data structure tests"
	@echo "  test_tokenizer    - Tokenizer tests"
	@echo "  test_index        - Inverted index tests"
	@echo "  test_vector_index - Vector index tests"
	@echo "  test_search       - Search engine tests"
	@echo "  test_bm25         - BM25 algorithm tests"
	@echo "  test_utils        - Utility functions tests"
	@echo "  test_integration  - Integration tests"
	@echo ""
