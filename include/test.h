#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/**
 * Simple unit testing framework
 */

typedef struct {
    const char* name;
    int passed;
    int failed;
    int total;
} TestSuite;

/**
 * Create a test suite
 */
TestSuite* test_suite_create(const char* name);

/**
 * Destroy test suite
 */
void test_suite_destroy(TestSuite* suite);

/**
 * Assert equal (integers)
 */
void assert_equal_int(TestSuite* suite, const char* test_name, 
                      int expected, int actual);

/**
 * Assert equal (strings)
 */
void assert_equal_str(TestSuite* suite, const char* test_name,
                      const char* expected, const char* actual);

/**
 * Assert true
 */
void assert_true(TestSuite* suite, const char* test_name, bool condition);

/**
 * Assert false
 */
void assert_false(TestSuite* suite, const char* test_name, bool condition);

/**
 * Print test results
 */
void test_suite_print_results(TestSuite* suite);

#endif // TEST_H
