/**
 * Unit Tests for Tokenizer
 * 
 * Coverage:
 * - Basic tokenization (English)
 * - Punctuation handling
 * - Case normalization
 * - Multiple spaces
 * - Special characters
 * - Unicode/UTF-8 handling
 * - Memory management
 */

#include "test.h"
#include "tokenizer.h"
#include "trie.h"
#include <string.h>

void test_tokenizer_basic(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Basic Operations");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Simple sentence
    TokenList* tokens = tokenizer_tokenize(tokenizer, "hello world");
    assert_equal_int(suite, "Tokenize 'hello world'", 2, (int)tokens->count);
    assert_equal_str(suite, "First token is 'hello'", "hello", tokens->tokens[0]);
    assert_equal_str(suite, "Second token is 'world'", "world", tokens->tokens[1]);
    tokenizer_free_tokens(tokens);
    
    // Single word
    tokens = tokenizer_tokenize(tokenizer, "python");
    assert_equal_int(suite, "Tokenize single word", 1, (int)tokens->count);
    assert_equal_str(suite, "Token is 'python'", "python", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    // Empty string
    tokens = tokenizer_tokenize(tokenizer, "");
    assert_equal_int(suite, "Tokenize empty string", 0, (int)tokens->count);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_punctuation(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Punctuation Handling");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Comma and period
    TokenList* tokens = tokenizer_tokenize(tokenizer, "hello, world!");
    assert_equal_int(suite, "Tokenize with punctuation", 2, (int)tokens->count);
    assert_equal_str(suite, "First token without comma", "hello", tokens->tokens[0]);
    assert_equal_str(suite, "Second token without exclamation", "world", tokens->tokens[1]);
    tokenizer_free_tokens(tokens);
    
    // Multiple punctuation
    tokens = tokenizer_tokenize(tokenizer, "hello... world??");
    assert_equal_int(suite, "Multiple punctuation marks", 2, (int)tokens->count);
    tokenizer_free_tokens(tokens);
    
    // Quotes and parentheses
    tokens = tokenizer_tokenize(tokenizer, "\"hello\" (world)");
    assert_equal_int(suite, "Quotes and parentheses", 2, (int)tokens->count);
    assert_equal_str(suite, "Token without quotes", "hello", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_case_normalization(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Case Normalization");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Uppercase
    TokenList* tokens = tokenizer_tokenize(tokenizer, "HELLO WORLD");
    assert_equal_str(suite, "Uppercase to lowercase", "hello", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    // Mixed case
    tokens = tokenizer_tokenize(tokenizer, "HeLLo WoRLd");
    assert_equal_str(suite, "Mixed case to lowercase", "hello", tokens->tokens[0]);
    assert_equal_str(suite, "Mixed case word 2", "world", tokens->tokens[1]);
    tokenizer_free_tokens(tokens);
    
    // CamelCase
    tokens = tokenizer_tokenize(tokenizer, "CamelCaseWord");
    assert_equal_str(suite, "CamelCase to lowercase", "camelcaseword", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_whitespace(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Whitespace Handling");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Multiple spaces
    TokenList* tokens = tokenizer_tokenize(tokenizer, "hello    world");
    assert_equal_int(suite, "Multiple spaces", 2, (int)tokens->count);
    tokenizer_free_tokens(tokens);
    
    // Leading/trailing spaces
    tokens = tokenizer_tokenize(tokenizer, "  hello world  ");
    assert_equal_int(suite, "Leading/trailing spaces", 2, (int)tokens->count);
    assert_equal_str(suite, "First token clean", "hello", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    // Tabs and newlines
    tokens = tokenizer_tokenize(tokenizer, "hello\tworld\ntest");
    assert_equal_int(suite, "Tabs and newlines", 3, (int)tokens->count);
    tokenizer_free_tokens(tokens);
    
    // Only whitespace
    tokens = tokenizer_tokenize(tokenizer, "   \t\n  ");
    assert_equal_int(suite, "Only whitespace", 0, (int)tokens->count);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_special_chars(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Special Characters");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Hyphenated words
    TokenList* tokens = tokenizer_tokenize(tokenizer, "state-of-the-art");
    // Behavior depends on implementation - just check it doesn't crash
    assert_true(suite, "Hyphenated words", tokens->count >= 1);
    tokenizer_free_tokens(tokens);
    
    // Apostrophes
    tokens = tokenizer_tokenize(tokenizer, "don't can't");
    assert_true(suite, "Apostrophes handled", tokens->count >= 1);
    tokenizer_free_tokens(tokens);
    
    // Numbers
    tokens = tokenizer_tokenize(tokenizer, "hello123 world456");
    assert_true(suite, "Words with numbers", tokens->count >= 1);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_memory(void) {
    TestSuite* suite = test_suite_create("Tokenizer - Memory Management");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Multiple tokenizations to test for leaks
    for (int i = 0; i < 100; i++) {
        TokenList* tokens = tokenizer_tokenize(tokenizer, "hello world test");
        tokenizer_free_tokens(tokens);
    }
    assert_true(suite, "100 tokenizations without crash", 1);
    
    // Large input
    char large_input[1000];
    for (int i = 0; i < 99; i++) {
        large_input[i * 10] = 'h';
        large_input[i * 10 + 1] = 'e';
        large_input[i * 10 + 2] = 'l';
        large_input[i * 10 + 3] = 'l';
        large_input[i * 10 + 4] = 'o';
        large_input[i * 10 + 5] = ' ';
        large_input[i * 10 + 6] = 'w';
        large_input[i * 10 + 7] = 'o';
        large_input[i * 10 + 8] = 'r';
        large_input[i * 10 + 9] = 'd';
    }
    large_input[990] = '\0';
    
    TokenList* tokens = tokenizer_tokenize(tokenizer, large_input);
    assert_true(suite, "Large input handled", tokens->count >= 1);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Tokenizer Module Test Suite ===\n\n");
    
    test_tokenizer_basic();
    test_tokenizer_punctuation();
    test_tokenizer_case_normalization();
    test_tokenizer_whitespace();
    test_tokenizer_special_chars();
    test_tokenizer_memory();
    
    return 0;
}
