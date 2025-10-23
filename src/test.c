#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TestSuite* test_suite_create(const char* name) {
    TestSuite* suite = (TestSuite*)malloc(sizeof(TestSuite));
    suite->name = name;
    suite->passed = 0;
    suite->failed = 0;
    suite->total = 0;
    return suite;
}

void test_suite_destroy(TestSuite* suite) {
    if (suite) free(suite);
}

void assert_equal_int(TestSuite* suite, const char* test_name,
                      int expected, int actual) {
    suite->total++;
    
    if (expected == actual) {
        suite->passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        suite->failed++;
        printf("  ✗ %s\n    Expected: %d, Got: %d\n", test_name, expected, actual);
    }
}

void assert_equal_str(TestSuite* suite, const char* test_name,
                      const char* expected, const char* actual) {
    suite->total++;
    
    if (strcmp(expected, actual) == 0) {
        suite->passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        suite->failed++;
        printf("  ✗ %s\n    Expected: %s, Got: %s\n", test_name, expected, actual);
    }
}

void assert_true(TestSuite* suite, const char* test_name, bool condition) {
    suite->total++;
    
    if (condition) {
        suite->passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        suite->failed++;
        printf("  ✗ %s (condition was false)\n", test_name);
    }
}

void assert_false(TestSuite* suite, const char* test_name, bool condition) {
    suite->total++;
    
    if (!condition) {
        suite->passed++;
        printf("  ✓ %s\n", test_name);
    } else {
        suite->failed++;
        printf("  ✗ %s (condition was true)\n", test_name);
    }
}

void test_suite_print_results(TestSuite* suite) {
    printf("\n");
    printf("================================\n");
    printf("Test Suite: %s\n", suite->name);
    printf("================================\n");
    printf("Passed: %d/%d\n", suite->passed, suite->total);
    printf("Failed: %d/%d\n", suite->failed, suite->total);
    
    if (suite->failed == 0) {
        printf("Status: ✓ ALL PASSED\n");
    } else {
        printf("Status: ✗ FAILED\n");
    }
    printf("================================\n\n");
}
