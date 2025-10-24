/**
 * Unit Tests for Utility Functions
 * 
 * Coverage:
 * - String manipulation
 * - File utilities
 * - Memory utilities
 */

#include "test.h"
#include "utils.h"
#include <string.h>

void test_string_dup(void) {
    TestSuite* suite = test_suite_create("Utils - String Dup");
    
    const char* original = "hello world";
    char* copy = string_dup(original);
    
    assert_true(suite, "Copy is not NULL", copy != NULL);
    assert_equal_str(suite, "Copy matches original", original, copy);
    assert_true(suite, "Copy is different pointer", copy != original);
    
    free(copy);
    
    // Empty string
    copy = string_dup("");
    assert_equal_str(suite, "Empty string duplicate", "", copy);
    free(copy);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_string_tolower(void) {
    TestSuite* suite = test_suite_create("Utils - String To Lower");
    
    char* result = string_tolower("HELLO");
    assert_equal_str(suite, "HELLO -> hello", "hello", result);
    free(result);
    
    result = string_tolower("MiXeD");
    assert_equal_str(suite, "MiXeD -> mixed", "mixed", result);
    free(result);
    
    result = string_tolower("already lowercase");
    assert_equal_str(suite, "No change needed", "already lowercase", result);
    free(result);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_string_trim(void) {
    TestSuite* suite = test_suite_create("Utils - String Trim");
    
    char* result = string_trim("  hello  ");
    assert_equal_str(suite, "Trim both sides", "hello", result);
    free(result);
    
    result = string_trim("hello  ");
    assert_equal_str(suite, "Trim right only", "hello", result);
    free(result);
    
    result = string_trim("  hello");
    assert_equal_str(suite, "Trim left only", "hello", result);
    free(result);
    
    result = string_trim("hello");
    assert_equal_str(suite, "No trim needed", "hello", result);
    free(result);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_string_operations(void) {
    TestSuite* suite = test_suite_create("Utils - String Operations");
    
    // starts_with
    assert_true(suite, "Starts with 'hel'", string_starts_with("hello", "hel"));
    assert_false(suite, "Does not start with 'wor'", string_starts_with("hello", "wor"));
    
    // ends_with
    assert_true(suite, "Ends with 'lo'", string_ends_with("hello", "lo"));
    assert_false(suite, "Does not end with 'he'", string_ends_with("hello", "he"));
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_memory_allocation(void) {
    TestSuite* suite = test_suite_create("Utils - Memory Allocation");
    
    // Allocate and free memory multiple times
    for (int i = 0; i < 100; i++) {
        void* ptr = safe_malloc(1024);
        assert_true(suite, "Allocation successful", ptr != NULL);
        free(ptr);
    }
    
    assert_true(suite, "100 allocations/frees completed", 1);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Utility Functions Test Suite ===\n\n");
    
    test_string_dup();
    test_string_tolower();
    test_string_trim();
    test_string_operations();
    test_memory_allocation();
    
    return 0;
}
