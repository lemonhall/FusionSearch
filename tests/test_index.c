/**
 * Unit Tests for Inverted Index
 * 
 * Coverage:
 * - Document addition
 * - Term search
 * - Document retrieval
 * - Posting list management
 * - Edge cases (large docs, many docs)
 */

#include "test.h"
#include "index.h"
#include <string.h>

void test_index_basic_operations(void) {
    TestSuite* suite = test_suite_create("Index - Basic Operations");
    
    InvertedIndex* index = index_create();
    assert_true(suite, "Index creation", index != NULL);
    
    // Add single document
    const char* tokens[] = {"hello", "world"};
    index_add_document(index, 0, "Test Doc", "hello world", tokens, 2);
    
    // Search for term
    uint32_t results[10];
    size_t count = index_search_term(index, "hello", results, 10);
    assert_equal_int(suite, "Search 'hello' returns 1 doc", 1, (int)count);
    assert_equal_int(suite, "Doc ID is 0", 0, (int)results[0]);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_multiple_documents(void) {
    TestSuite* suite = test_suite_create("Index - Multiple Documents");
    
    InvertedIndex* index = index_create();
    
    const char* doc1_tokens[] = {"python", "programming"};
    const char* doc2_tokens[] = {"java", "programming"};
    const char* doc3_tokens[] = {"python", "web"};
    
    index_add_document(index, 0, "Doc1", "python programming", doc1_tokens, 2);
    index_add_document(index, 1, "Doc2", "java programming", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "python web", doc3_tokens, 2);
    
    // Search term in multiple docs
    uint32_t results[10];
    size_t count = index_search_term(index, "python", results, 10);
    assert_equal_int(suite, "Search 'python' returns 2 docs", 2, (int)count);
    
    count = index_search_term(index, "programming", results, 10);
    assert_equal_int(suite, "Search 'programming' returns 2 docs", 2, (int)count);
    
    count = index_search_term(index, "web", results, 10);
    assert_equal_int(suite, "Search 'web' returns 1 doc", 1, (int)count);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_term_frequency(void) {
    TestSuite* suite = test_suite_create("Index - Term Frequency");
    
    InvertedIndex* index = index_create();
    
    // Document with repeated terms
    const char* tokens[] = {"python", "python", "python", "programming"};
    index_add_document(index, 0, "Doc1", "python python python programming", 
                      tokens, 4);
    
    // Get posting list to check frequency
    size_t count;
    PostingEntry** postings = index_get_postings(index, "python", &count);
    assert_equal_int(suite, "Found postings for 'python'", 1, (int)count);
    
    if (count > 0 && postings[0]) {
        assert_equal_int(suite, "Term frequency of 'python'", 3, (int)postings[0]->frequency);
    }
    
    postings = index_get_postings(index, "programming", &count);
    if (count > 0 && postings[0]) {
        assert_equal_int(suite, "Term frequency of 'programming'", 1, (int)postings[0]->frequency);
    }
    
    postings = index_get_postings(index, "nonexistent", &count);
    assert_equal_int(suite, "Non-existent term has no postings", 0, (int)count);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_document_retrieval(void) {
    TestSuite* suite = test_suite_create("Index - Document Retrieval");
    
    InvertedIndex* index = index_create();
    
    const char* tokens[] = {"hello", "world"};
    index_add_document(index, 42, "My Title", "My Content Here", tokens, 2);
    
    Document* doc = index_get_document(index, 42);
    assert_true(suite, "Document retrieved", doc != NULL);
    
    if (doc) {
        assert_equal_str(suite, "Document title matches", "My Title", doc->title);
        assert_equal_str(suite, "Document content matches", "My Content Here", doc->content);
        assert_equal_int(suite, "Document ID matches", 42, (int)doc->docId);
    }
    
    // Non-existent document
    doc = index_get_document(index, 999);
    assert_true(suite, "Non-existent document returns NULL", doc == NULL);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_document_count(void) {
    TestSuite* suite = test_suite_create("Index - Document Count");
    
    InvertedIndex* index = index_create();
    
    assert_equal_int(suite, "Empty index has 0 documents", 0, 
                    (int)index->docStore->docCount);
    
    const char* tokens1[] = {"hello"};
    index_add_document(index, 0, "Doc1", "hello", tokens1, 1);
    assert_equal_int(suite, "Count after 1 document", 1, 
                    (int)index->docStore->docCount);
    
    const char* tokens2[] = {"world"};
    index_add_document(index, 1, "Doc2", "world", tokens2, 1);
    assert_equal_int(suite, "Count after 2 documents", 2, 
                    (int)index->docStore->docCount);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_edge_cases(void) {
    TestSuite* suite = test_suite_create("Index - Edge Cases");
    
    InvertedIndex* index = index_create();
    
    // Empty document
    const char* empty_tokens[] = {};
    index_add_document(index, 0, "Empty", "", empty_tokens, 0);
    assert_equal_int(suite, "Empty document added", 1, 
                    (int)index->docStore->docCount);
    
    // Very long document
    const char* long_tokens[100];
    for (int i = 0; i < 100; i++) {
        long_tokens[i] = "word";
    }
    index_add_document(index, 1, "Long", "Many words...", long_tokens, 100);
    
    uint32_t results[10];
    size_t count = index_search_term(index, "word", results, 10);
    assert_equal_int(suite, "Long document indexed", 1, (int)count);
    
    // Search non-existent term
    count = index_search_term(index, "nonexistent", results, 10);
    assert_equal_int(suite, "Non-existent term returns 0", 0, (int)count);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_index_docs_with_term(void) {
    TestSuite* suite = test_suite_create("Index - Docs With Term Count");
    
    InvertedIndex* index = index_create();
    
    const char* doc1_tokens[] = {"python", "java"};
    const char* doc2_tokens[] = {"python", "javascript"};
    const char* doc3_tokens[] = {"ruby", "java"};
    
    index_add_document(index, 0, "Doc1", "python java", doc1_tokens, 2);
    index_add_document(index, 1, "Doc2", "python javascript", doc2_tokens, 2);
    index_add_document(index, 2, "Doc3", "ruby java", doc3_tokens, 2);
    
    // Use index_get_postings to count
    size_t count;
    index_get_postings(index, "python", &count);
    assert_equal_int(suite, "Docs with 'python'", 2, (int)count);
    
    index_get_postings(index, "java", &count);
    assert_equal_int(suite, "Docs with 'java'", 2, (int)count);
    
    index_get_postings(index, "javascript", &count);
    assert_equal_int(suite, "Docs with 'javascript'", 1, (int)count);
    
    index_get_postings(index, "nonexistent", &count);
    assert_equal_int(suite, "Docs with non-existent term", 0, (int)count);
    
    index_destroy(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Index Module Test Suite ===\n\n");
    
    test_index_basic_operations();
    test_index_multiple_documents();
    test_index_term_frequency();
    test_index_document_retrieval();
    test_index_document_count();
    test_index_edge_cases();
    test_index_docs_with_term();
    
    return 0;
}
