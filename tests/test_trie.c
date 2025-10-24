/**
 * Unit Tests for Trie Data Structure
 * 
 * Coverage:
 * - Basic insert/search operations
 * - Frequency tracking
 * - Word counting
 * - Memory management
 * - Edge cases (empty strings, duplicates, long words)
 */

#include "test.h"
#include "trie.h"
#include <string.h>

void test_trie_basic_operations(void) {
    TestSuite* suite = test_suite_create("Trie - Basic Operations");
    
    Trie* trie = trie_create();
    assert_true(suite, "Trie creation", trie != NULL);
    
    // Insert and search
    trie_insert(trie, "hello", 1);
    assert_true(suite, "Search after insert 'hello'", trie_search(trie, "hello"));
    
    trie_insert(trie, "world", 2);
    assert_true(suite, "Search after insert 'world'", trie_search(trie, "world"));
    
    // Search non-existent
    assert_false(suite, "Search non-existent 'python'", trie_search(trie, "python"));
    
    trie_destroy(trie);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_trie_frequency_tracking(void) {
    TestSuite* suite = test_suite_create("Trie - Frequency Tracking");
    
    Trie* trie = trie_create();
    
    // Initial frequency
    trie_insert(trie, "hello", 1);
    assert_equal_int(suite, "Frequency of 'hello' is 1", 1, trie_get_frequency(trie, "hello"));
    
    // Increment frequency
    trie_insert(trie, "hello", 1);
    assert_equal_int(suite, "Frequency incremented to 2", 2, trie_get_frequency(trie, "hello"));
    
    // Different word
    trie_insert(trie, "world", 5);
    assert_equal_int(suite, "Frequency of 'world' is 5", 5, trie_get_frequency(trie, "world"));
    
    // Non-existent word
    assert_equal_int(suite, "Frequency of non-existent is 0", 0, 
                    trie_get_frequency(trie, "nonexistent"));
    
    trie_destroy(trie);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_trie_word_count(void) {
    TestSuite* suite = test_suite_create("Trie - Word Count");
    
    Trie* trie = trie_create();
    
    assert_equal_int(suite, "Empty trie has 0 words", 0, trie_word_count(trie));
    
    trie_insert(trie, "hello", 1);
    assert_equal_int(suite, "Word count after 1 insert", 1, trie_word_count(trie));
    
    trie_insert(trie, "world", 1);
    assert_equal_int(suite, "Word count after 2 inserts", 2, trie_word_count(trie));
    
    // Duplicate insert doesn't increase count
    trie_insert(trie, "hello", 1);
    assert_equal_int(suite, "Duplicate insert doesn't increase count", 2, 
                    trie_word_count(trie));
    
    trie_destroy(trie);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_trie_edge_cases(void) {
    TestSuite* suite = test_suite_create("Trie - Edge Cases");
    
    Trie* trie = trie_create();
    
    // Single character
    trie_insert(trie, "a", 1);
    assert_true(suite, "Single character word", trie_search(trie, "a"));
    
    // Long word
    const char* long_word = "supercalifragilisticexpialidocious";
    trie_insert(trie, long_word, 1);
    assert_true(suite, "Very long word", trie_search(trie, long_word));
    
    // Similar words (prefix relationship)
    trie_insert(trie, "test", 1);
    trie_insert(trie, "testing", 1);
    trie_insert(trie, "tester", 1);
    assert_true(suite, "Word 'test'", trie_search(trie, "test"));
    assert_true(suite, "Word 'testing'", trie_search(trie, "testing"));
    assert_true(suite, "Word 'tester'", trie_search(trie, "tester"));
    assert_false(suite, "Non-existent 'tested'", trie_search(trie, "tested"));
    
    // Case sensitivity
    trie_insert(trie, "hello", 1);
    assert_false(suite, "Case sensitive - 'HELLO' not found", trie_search(trie, "HELLO"));
    
    trie_destroy(trie);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_trie_null_safety(void) {
    TestSuite* suite = test_suite_create("Trie - NULL Safety");
    
    Trie* trie = trie_create();
    
    // Search with NULL should not crash (implementation dependent)
    // Just ensure function handles it gracefully
    assert_false(suite, "Search empty string", trie_search(trie, ""));
    assert_equal_int(suite, "Frequency of empty string", 0, trie_get_frequency(trie, ""));
    
    trie_destroy(trie);
    
    // Destroy NULL trie should not crash
    trie_destroy(NULL);
    assert_true(suite, "Destroy NULL trie doesn't crash", 1);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Trie Module Test Suite ===\n\n");
    
    test_trie_basic_operations();
    test_trie_frequency_tracking();
    test_trie_word_count();
    test_trie_edge_cases();
    test_trie_null_safety();
    
    return 0;
}
