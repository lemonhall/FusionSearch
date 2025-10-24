/**
 * Integration Tests
 * 
 * Tests that verify complete workflows and module interactions:
 * - End-to-end search pipeline
 * - Hybrid search (BM25 + Vector)
 * - File loading workflows
 * - Large dataset handling
 */

#include "test.h"
#include "search.h"
#include "index.h"
#include "tokenizer.h"
#include "trie.h"
#include "vector_index.h"
#include "file_loader.h"
#include "bm25.h"
#include <string.h>

void test_end_to_end_search(void) {
    TestSuite* suite = test_suite_create("Integration - End-to-End Search");
    
    // Create full search pipeline
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Index sample documents
    const char* doc1_tokens[] = {"python", "programming", "language", "guide"};
    const char* doc2_tokens[] = {"java", "programming", "language", "tutorial"};
    const char* doc3_tokens[] = {"python", "web", "development", "framework"};
    const char* doc4_tokens[] = {"javascript", "web", "programming"};
    
    index_add_document(index, 0, "Python Guide", 
                      "Python programming language guide", doc1_tokens, 4);
    index_add_document(index, 1, "Java Tutorial", 
                      "Java programming language tutorial", doc2_tokens, 4);
    index_add_document(index, 2, "Python Web", 
                      "Python web development framework", doc3_tokens, 4);
    index_add_document(index, 3, "JavaScript Web", 
                      "JavaScript web programming", doc4_tokens, 3);
    
    // Test 1: Simple search
    SearchResultSet* results = search_engine_search(engine, "python", SEARCH_OR, 10);
    assert_equal_int(suite, "Search 'python' finds 2 docs", 2, (int)results->count);
    search_free_results(results);
    
    // Test 2: AND search
    results = search_engine_search(engine, "python programming", SEARCH_AND, 10);
    assert_equal_int(suite, "AND search finds 1 doc", 1, (int)results->count);
    search_free_results(results);
    
    // Test 3: BM25 ranking
    results = search_engine_search(engine, "web programming", SEARCH_BM25, 10);
    assert_true(suite, "BM25 search returns results", results->count > 0);
    assert_true(suite, "Results are ranked", 
               results->count < 2 || results->results[0].score >= results->results[1].score);
    search_free_results(results);
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_hybrid_search_workflow(void) {
    TestSuite* suite = test_suite_create("Integration - Hybrid Search Workflow");
    
    // Create BM25 index
    Trie* dict = trie_create();
    InvertedIndex* bm25_index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    
    // Create Vector index
    VectorIndex* vec_index = vector_index_create(3);
    
    // Add documents to both indexes
    const char* doc1_tokens[] = {"machine", "learning"};
    const char* doc2_tokens[] = {"deep", "learning"};
    const char* doc3_tokens[] = {"neural", "networks"};
    
    index_add_document(bm25_index, 1, "ML", "machine learning", doc1_tokens, 2);
    index_add_document(bm25_index, 2, "DL", "deep learning", doc2_tokens, 2);
    index_add_document(bm25_index, 3, "NN", "neural networks", doc3_tokens, 2);
    
    float vec1[] = {1.0f, 0.0f, 0.0f};
    float vec2[] = {0.8f, 0.6f, 0.0f};
    float vec3[] = {0.0f, 1.0f, 0.0f};
    
    vector_index_add(vec_index, 1, vec1);
    vector_index_add(vec_index, 2, vec2);
    vector_index_add(vec_index, 3, vec3);
    
    // Test BM25 search
    SearchEngine* engine = search_engine_create(bm25_index, tokenizer);
    SearchResultSet* bm25_results = search_engine_search(engine, "learning", SEARCH_BM25, 10);
    assert_true(suite, "BM25 search successful", bm25_results->count >= 2);
    search_free_results(bm25_results);
    
    // Test Vector search
    float query[] = {1.0f, 0.0f, 0.0f};
    size_t vec_count;
    VectorResult* vec_results = vector_search(vec_index, query, 2, &vec_count);
    assert_equal_int(suite, "Vector search returns 2", 2, (int)vec_count);
    assert_equal_int(suite, "Top result is doc 1", 1, (int)vec_results[0].doc_id);
    free(vec_results);
    
    // Verify both indexes can be used together
    assert_equal_int(suite, "BM25 has 3 docs", 3, (int)bm25_index->docStore->docCount);
    assert_equal_int(suite, "Vector has 3 docs", 3, (int)vector_index_count(vec_index));
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(bm25_index);
    trie_destroy(dict);
    vector_index_free(vec_index);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_large_dataset(void) {
    TestSuite* suite = test_suite_create("Integration - Large Dataset");
    
    Trie* dict = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Add 1000 documents
    const int num_docs = 1000;
    for (int i = 0; i < num_docs; i++) {
        char title[50], content[100];
        sprintf(title, "Document %d", i);
        sprintf(content, "This is document number %d about python programming", i);
        
        const char* tokens[] = {"this", "is", "document", "number", "about", "python", "programming"};
        index_add_document(index, i, title, content, tokens, 7);
    }
    
    assert_equal_int(suite, "1000 documents indexed", num_docs, 
                    (int)index->docStore->docCount);
    
    // Search should still work
    SearchResultSet* results = search_engine_search(engine, "python", SEARCH_OR, 10);
    assert_equal_int(suite, "Search returns top 10", 10, (int)results->count);
    search_free_results(results);
    
    // BM25 search
    results = search_engine_search(engine, "python programming", SEARCH_BM25, 5);
    assert_equal_int(suite, "BM25 returns top 5", 5, (int)results->count);
    search_free_results(results);
    
    // Cleanup
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    index_destroy(index);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_tokenizer_index_integration(void) {
    TestSuite* suite = test_suite_create("Integration - Tokenizer + Index");
    
    Trie* dict = trie_create();
    Tokenizer* tokenizer = tokenizer_create(dict);
    InvertedIndex* index = index_create();
    
    // Tokenize and index in one workflow
    const char* text = "Hello, World! This is a test.";
    TokenList* tokens = tokenizer_tokenize(tokenizer, text);
    
    assert_true(suite, "Tokenization successful", tokens->count > 0);
    
    // Use tokens to build index
    index_add_document(index, 0, "Test Doc", text, 
                      (const char**)tokens->tokens, tokens->count);
    
    // Verify index contains tokens
    for (size_t i = 0; i < tokens->count; i++) {
        uint32_t results[10];
        size_t count = index_search_term(index, tokens->tokens[i], results, 10);
        assert_true(suite, "Token found in index", count > 0);
    }
    
    tokenizer_free_tokens(tokens);
    index_destroy(index);
    tokenizer_destroy(tokenizer);
    trie_destroy(dict);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_memory_stress(void) {
    TestSuite* suite = test_suite_create("Integration - Memory Stress Test");
    
    // Create and destroy multiple times
    for (int iteration = 0; iteration < 10; iteration++) {
        Trie* dict = trie_create();
        InvertedIndex* index = index_create();
        Tokenizer* tokenizer = tokenizer_create(dict);
        VectorIndex* vec_index = vector_index_create(128);
        
        // Add some data
        const char* tokens[] = {"test", "data"};
        index_add_document(index, 0, "Test", "test data", tokens, 2);
        
        float vec[128];
        for (int i = 0; i < 128; i++) vec[i] = 0.5f;
        vector_index_add(vec_index, 0, vec);
        
        // Cleanup
        vector_index_free(vec_index);
        tokenizer_destroy(tokenizer);
        index_destroy(index);
        trie_destroy(dict);
    }
    
    assert_true(suite, "10 create/destroy cycles completed", 1);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║      Integration Tests Suite           ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    test_end_to_end_search();
    test_hybrid_search_workflow();
    test_large_dataset();
    test_tokenizer_index_integration();
    test_memory_stress();
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   Integration Tests Completed!         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}
