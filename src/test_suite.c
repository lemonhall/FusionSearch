#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "trie.h"
#include "tokenizer.h"
#include "index.h"
#include "search.h"
#include "bm25.h"

/**
 * Test Trie data structure
 */
void test_trie(void) {
    TestSuite* suite = test_suite_create("Trie Data Structure");
    
    Trie* trie = trie_create();
    
    // Test: Insert and search
    trie_insert(trie, "hello", 1);
    trie_insert(trie, "world", 2);
    
    assert_true(suite, "Insert 'hello' and search", trie_search(trie, "hello"));
    assert_true(suite, "Insert 'world' and search", trie_search(trie, "world"));
    assert_false(suite, "Search non-existent word", trie_search(trie, "python"));
    
    // Test: Frequency tracking
    assert_equal_int(suite, "Get frequency of 'hello'", 1, trie_get_frequency(trie, "hello"));
    assert_equal_int(suite, "Get frequency of 'world'", 2, trie_get_frequency(trie, "world"));
    assert_equal_int(suite, "Get frequency of non-existent", 0, trie_get_frequency(trie, "python"));
    
    // Test: Word count
    assert_equal_int(suite, "Word count after 2 inserts", 2, trie_word_count(trie));
    
    trie_destroy(trie);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test Tokenizer
 */
void test_tokenizer(void) {
    TestSuite* suite = test_suite_create("Tokenizer");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Test: Simple tokenization
    TokenList* tokens = tokenizer_tokenize(tokenizer, "hello world");
    assert_equal_int(suite, "Tokenize 'hello world'", 2, (int)tokens->count);
    assert_equal_str(suite, "First token is 'hello'", "hello", tokens->tokens[0]);
    assert_equal_str(suite, "Second token is 'world'", "world", tokens->tokens[1]);
    tokenizer_free_tokens(tokens);
    
    // Test: Punctuation removal
    tokens = tokenizer_tokenize(tokenizer, "hello, world!");
    assert_equal_int(suite, "Tokenize with punctuation", 2, (int)tokens->count);
    assert_equal_str(suite, "First token without comma", "hello", tokens->tokens[0]);
    assert_equal_str(suite, "Second token without exclamation", "world", tokens->tokens[1]);
    tokenizer_free_tokens(tokens);
    
    // Test: Case insensitivity
    tokens = tokenizer_tokenize(tokenizer, "HELLO WORLD");
    assert_equal_str(suite, "Uppercase converted to lowercase", "hello", tokens->tokens[0]);
    tokenizer_free_tokens(tokens);
    
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test Inverted Index
 */
void test_inverted_index(void) {
    TestSuite* suite = test_suite_create("Inverted Index");
    
    InvertedIndex* index = index_create();
    
    // Add test documents
    const char* doc1_tokens[] = {"python", "programming", "language"};
    const char* doc2_tokens[] = {"java", "programming", "language"};
    const char* doc3_tokens[] = {"python", "javascript", "web"};
    
    index_add_document(index, 0, "Doc1", "python programming language", doc1_tokens, 3);
    index_add_document(index, 1, "Doc2", "java programming language", doc2_tokens, 3);
    index_add_document(index, 2, "Doc3", "python javascript web", doc3_tokens, 3);
    
    // Test: Search single term
    uint32_t results[10];
    size_t count = index_search_term(index, "python", results, 10);
    assert_equal_int(suite, "Search 'python' returns 2 docs", 2, (int)count);
    
    // Test: Search term in multiple docs
    count = index_search_term(index, "programming", results, 10);
    assert_equal_int(suite, "Search 'programming' returns 2 docs", 2, (int)count);
    
    // Test: Search non-existent term
    count = index_search_term(index, "nonexistent", results, 10);
    assert_equal_int(suite, "Search non-existent returns 0", 0, (int)count);
    
    // Test: Get document
    Document* doc = index_get_document(index, 0);
    assert_true(suite, "Get document by ID", doc != NULL);
    if (doc) {
        assert_equal_str(suite, "Document title matches", "Doc1", doc->title);
    }
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test Search Engine - AND mode
 */
void test_search_and(void) {
    TestSuite* suite = test_suite_create("Search Engine - AND Mode");
    
    // Setup
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Add documents
    const char* doc1_tokens[] = {"python", "programming", "language"};
    const char* doc2_tokens[] = {"java", "programming"};
    const char* doc3_tokens[] = {"python", "web", "development"};
    
    index_add_document(index, 0, "Doc1", "python programming language", doc1_tokens, 3);
    index_add_document(index, 1, "Doc2", "java programming", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "python web development", doc3_tokens, 3);
    
    // Test: AND search (both terms present)
    SearchResultSet* results = search_engine_search(engine, "python programming", SEARCH_AND, 10);
    assert_equal_int(suite, "AND search 'python programming' returns 1", 1, (int)results->count);
    search_free_results(results);
    
    // Test: AND search (one term not present in any doc)
    results = search_engine_search(engine, "python java", SEARCH_AND, 10);
    assert_equal_int(suite, "AND search 'python java' returns 0", 0, (int)results->count);
    search_free_results(results);
    
    // Test: AND search (different pair)
    results = search_engine_search(engine, "python web", SEARCH_AND, 10);
    assert_equal_int(suite, "AND search 'python web' returns 1", 1, (int)results->count);
    search_free_results(results);
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test Search Engine - OR mode
 */
void test_search_or(void) {
    TestSuite* suite = test_suite_create("Search Engine - OR Mode");
    
    // Setup
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Add documents
    const char* doc1_tokens[] = {"python", "programming", "language"};
    const char* doc2_tokens[] = {"java", "programming"};
    const char* doc3_tokens[] = {"javascript", "web", "development"};
    
    index_add_document(index, 0, "Doc1", "python programming language", doc1_tokens, 3);
    index_add_document(index, 1, "Doc2", "java programming", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "javascript web development", doc3_tokens, 3);
    
    // Test: OR search (both terms exist)
    SearchResultSet* results = search_engine_search(engine, "python java", SEARCH_OR, 10);
    assert_equal_int(suite, "OR search 'python java' returns 2", 2, (int)results->count);
    search_free_results(results);
    
    // Test: OR search (all three terms)
    results = search_engine_search(engine, "python java javascript", SEARCH_OR, 10);
    assert_equal_int(suite, "OR search 'python java javascript' returns 3", 3, (int)results->count);
    search_free_results(results);
    
    // Test: OR search (one term)
    results = search_engine_search(engine, "programming", SEARCH_OR, 10);
    assert_equal_int(suite, "OR search 'programming' returns 2", 2, (int)results->count);
    search_free_results(results);
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test TF-IDF Scoring
 */
void test_tfidf_scoring(void) {
    TestSuite* suite = test_suite_create("TF-IDF Scoring");
    
    // Test: TF-IDF calculation
    // termFreq=2, docLength=10, docsWithTerm=1, totalDocs=10
    float score = search_calculate_tfidf(2, 10, 1, 10);
    assert_true(suite, "TF-IDF score > 0", score > 0.0f);
    
    // Test: Higher term frequency gives higher score
    float score1 = search_calculate_tfidf(1, 10, 1, 10);
    float score2 = search_calculate_tfidf(5, 10, 1, 10);
    assert_true(suite, "Higher term frequency = higher score", score2 > score1);
    
    // Test: More documents with term gives lower score
    float score3 = search_calculate_tfidf(1, 10, 1, 10);
    float score4 = search_calculate_tfidf(1, 10, 5, 10);
    assert_true(suite, "More docs with term = lower score", score3 > score4);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test BM25 IDF Calculation
 */
void test_bm25_idf(void) {
    TestSuite* suite = test_suite_create("BM25 IDF Calculation");
    
    // Test: IDF calculation
    float idf1 = bm25_calculate_idf(10, 1);  // Rare term
    float idf2 = bm25_calculate_idf(10, 5);  // Common term
    
    assert_true(suite, "IDF for rare term > common term", idf1 > idf2);
    assert_true(suite, "IDF score > 0", idf1 > 0.0f);
    
    // Test: Edge cases
    float idf_all = bm25_calculate_idf(10, 10);  // Appears in all docs
    assert_true(suite, "IDF for ubiquitous term is low", idf_all < idf1);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test BM25 Scoring
 */
void test_bm25_scoring(void) {
    TestSuite* suite = test_suite_create("BM25 Scoring");
    
    BM25Params* params = bm25_params_create();
    float idf = 2.0f;
    float avgLen = 100.0f;
    
    // Test: Higher term frequency gives higher score
    float score1 = bm25_calculate_score(1, 100, avgLen, idf, params);
    float score2 = bm25_calculate_score(5, 100, avgLen, idf, params);
    assert_true(suite, "Higher TF = higher BM25 score", score2 > score1);
    
    // Test: Shorter docs get slight boost
    float score3 = bm25_calculate_score(1, 50, avgLen, idf, params);
    float score4 = bm25_calculate_score(1, 150, avgLen, idf, params);
    assert_true(suite, "Shorter docs get slight boost", score3 > score4);
    
    // Test: Score saturates with term frequency
    float score5 = bm25_calculate_score(10, 100, avgLen, idf, params);
    float score6 = bm25_calculate_score(100, 100, avgLen, idf, params);
    assert_true(suite, "Score saturates (sublinear growth)", 
                (score6 - score5) < (score2 - score1));
    
    bm25_params_destroy(params);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Test BM25 Search
 */
void test_search_bm25(void) {
    TestSuite* suite = test_suite_create("Search Engine - BM25 Mode");
    
    // Setup
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Add documents with different term frequencies
    const char* doc1_tokens[] = {"python", "programming", "language", "python", "python"};
    const char* doc2_tokens[] = {"java", "programming", "language"};
    const char* doc3_tokens[] = {"python", "web", "development"};
    
    index_add_document(index, 0, "Doc1", "python programming language python python", doc1_tokens, 5);
    index_add_document(index, 1, "Doc2", "java programming language", doc2_tokens, 3);
    index_add_document(index, 2, "Doc3", "python web development", doc3_tokens, 3);
    
    // Test: BM25 search with term frequency weighting
    SearchResultSet* results = search_engine_search(engine, "python programming", SEARCH_BM25, 10);
    assert_equal_int(suite, "BM25 search 'python programming' returns results", 
                    results->count > 0 ? 1 : 0, 1);
    
    // Doc1 should rank first (has multiple 'python' occurrences)
    if (results->count > 0) {
        assert_equal_int(suite, "Doc1 ranks first for 'python'", 0, (int)results->results[0].docId);
    }
    
    search_free_results(results);
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

/**
 * Main test runner
 */
int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     FusionSearch Unit Tests Suite      ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    test_trie();
    test_tokenizer();
    test_inverted_index();
    test_search_and();
    test_search_or();
    test_tfidf_scoring();
    test_bm25_idf();
    test_bm25_scoring();
    test_search_bm25();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         All tests completed!           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}
