/**
 * Unit Tests for Search Engine
 * 
 * Coverage:
 * - AND search mode
 * - OR search mode
 * - BM25 search mode
 * - Ranking and scoring
 * - Result ordering
 */

#include "test.h"
#include "search.h"
#include "index.h"
#include "tokenizer.h"
#include "trie.h"
#include <string.h>

void test_search_and_mode(void) {
    TestSuite* suite = test_suite_create("Search - AND Mode");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    const char* doc1_tokens[] = {"python", "programming", "language"};
    const char* doc2_tokens[] = {"java", "programming", "language"};
    const char* doc3_tokens[] = {"python", "web", "development"};
    
    index_add_document(index, 0, "Doc1", "python programming language", doc1_tokens, 3);
    index_add_document(index, 1, "Doc2", "java programming language", doc2_tokens, 3);
    index_add_document(index, 2, "Doc3", "python web development", doc3_tokens, 3);
    
    // Both terms in same doc
    SearchResultSet* results = search_engine_search(engine, "python programming", SEARCH_AND, 10);
    assert_equal_int(suite, "AND 'python programming' returns 1", 1, (int)results->count);
    if (results->count > 0) {
        assert_equal_int(suite, "Result is doc 0", 0, (int)results->results[0].docId);
    }
    search_free_results(results);
    
    // Terms in different docs
    results = search_engine_search(engine, "python java", SEARCH_AND, 10);
    assert_equal_int(suite, "AND 'python java' returns 0", 0, (int)results->count);
    search_free_results(results);
    
    // Three terms, two match
    results = search_engine_search(engine, "python programming web", SEARCH_AND, 10);
    assert_equal_int(suite, "AND 'python programming web' returns 0", 0, (int)results->count);
    search_free_results(results);
    
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_search_or_mode(void) {
    TestSuite* suite = test_suite_create("Search - OR Mode");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    const char* doc1_tokens[] = {"python", "programming"};
    const char* doc2_tokens[] = {"java", "programming"};
    const char* doc3_tokens[] = {"javascript", "web"};
    
    index_add_document(index, 0, "Doc1", "python programming", doc1_tokens, 2);
    index_add_document(index, 1, "Doc2", "java programming", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "javascript web", doc3_tokens, 2);
    
    // Single term
    SearchResultSet* results = search_engine_search(engine, "python", SEARCH_OR, 10);
    assert_equal_int(suite, "OR 'python' returns 1", 1, (int)results->count);
    search_free_results(results);
    
    // Two terms, both exist
    results = search_engine_search(engine, "python java", SEARCH_OR, 10);
    assert_equal_int(suite, "OR 'python java' returns 2", 2, (int)results->count);
    search_free_results(results);
    
    // Three terms
    results = search_engine_search(engine, "python java javascript", SEARCH_OR, 10);
    assert_equal_int(suite, "OR 'python java javascript' returns 3", 3, (int)results->count);
    search_free_results(results);
    
    // One term appears in multiple docs
    results = search_engine_search(engine, "programming", SEARCH_OR, 10);
    assert_equal_int(suite, "OR 'programming' returns 2", 2, (int)results->count);
    search_free_results(results);
    
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_search_bm25_ranking(void) {
    TestSuite* suite = test_suite_create("Search - BM25 Ranking");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Doc1: 3 occurrences of 'python'
    const char* doc1_tokens[] = {"python", "python", "python", "guide"};
    // Doc2: 1 occurrence of 'python'
    const char* doc2_tokens[] = {"python", "tutorial"};
    // Doc3: No 'python'
    const char* doc3_tokens[] = {"java", "programming"};
    
    index_add_document(index, 0, "Doc1", "python python python guide", doc1_tokens, 4);
    index_add_document(index, 1, "Doc2", "python tutorial", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "java programming", doc3_tokens, 2);
    
    SearchResultSet* results = search_engine_search(engine, "python", SEARCH_BM25, 10);
    
    assert_true(suite, "BM25 'python' returns results", results->count >= 2);
    
    if (results->count >= 2) {
        // Doc1 should rank higher (more occurrences)
        assert_equal_int(suite, "Doc1 ranks first", 0, (int)results->results[0].docId);
        assert_equal_int(suite, "Doc2 ranks second", 1, (int)results->results[1].docId);
        
        // Scores should be in descending order
        assert_true(suite, "Scores in descending order", 
                   results->results[0].score >= results->results[1].score);
    }
    
    search_free_results(results);
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_search_empty_query(void) {
    TestSuite* suite = test_suite_create("Search - Empty Query");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    const char* tokens[] = {"hello", "world"};
    index_add_document(index, 0, "Doc1", "hello world", tokens, 2);
    
    // Empty query
    SearchResultSet* results = search_engine_search(engine, "", SEARCH_OR, 10);
    assert_equal_int(suite, "Empty query returns 0", 0, (int)results->count);
    search_free_results(results);
    
    // Whitespace only
    results = search_engine_search(engine, "   ", SEARCH_OR, 10);
    assert_equal_int(suite, "Whitespace query returns 0", 0, (int)results->count);
    search_free_results(results);
    
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_search_nonexistent_terms(void) {
    TestSuite* suite = test_suite_create("Search - Non-existent Terms");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    const char* tokens[] = {"hello", "world"};
    index_add_document(index, 0, "Doc1", "hello world", tokens, 2);
    
    // Term doesn't exist
    SearchResultSet* results = search_engine_search(engine, "nonexistent", SEARCH_OR, 10);
    assert_equal_int(suite, "Non-existent term returns 0", 0, (int)results->count);
    search_free_results(results);
    
    // Mixed existent and non-existent (OR)
    results = search_engine_search(engine, "hello nonexistent", SEARCH_OR, 10);
    assert_equal_int(suite, "OR with non-existent returns 1", 1, (int)results->count);
    search_free_results(results);
    
    // Mixed existent and non-existent (AND)
    results = search_engine_search(engine, "hello nonexistent", SEARCH_AND, 10);
    assert_equal_int(suite, "AND with non-existent returns 0", 0, (int)results->count);
    search_free_results(results);
    
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_search_result_limit(void) {
    TestSuite* suite = test_suite_create("Search - Result Limit");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Add 10 documents with 'python'
    for (int i = 0; i < 10; i++) {
        const char* tokens[] = {"python"};
        char title[20], content[20];
        sprintf(title, "Doc%d", i);
        sprintf(content, "python");
        index_add_document(index, i, title, content, tokens, 1);
    }
    
    // Request top 5
    SearchResultSet* results = search_engine_search(engine, "python", SEARCH_OR, 5);
    assert_equal_int(suite, "Limit to 5 results", 5, (int)results->count);
    search_free_results(results);
    
    // Request top 3
    results = search_engine_search(engine, "python", SEARCH_BM25, 3);
    assert_equal_int(suite, "Limit to 3 results", 3, (int)results->count);
    search_free_results(results);
    
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Search Engine Module Test Suite ===\n\n");
    
    test_search_and_mode();
    test_search_or_mode();
    test_search_bm25_ranking();
    test_search_empty_query();
    test_search_nonexistent_terms();
    test_search_result_limit();
    
    return 0;
}
