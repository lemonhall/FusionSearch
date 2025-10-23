#ifndef SEARCH_H
#define SEARCH_H

#include <stddef.h>
#include <stdint.h>
#include "index.h"
#include "snippet.h"

#define MAX_QUERY_TERMS 100
#define MAX_SEARCH_RESULTS 100

typedef struct {
    uint32_t docId;
    float score;
    char* title;
    char* snippet;  // Highlighted snippet
} SearchResult;

typedef struct {
    SearchResult* results;
    size_t count;
    float executionTime;
} SearchResultSet;

typedef enum {
    SEARCH_AND,      // All terms must match (intersection)
    SEARCH_OR,       // Any term matches (union)
    SEARCH_BM25,     // BM25 ranking (best for most queries)
    SEARCH_PHRASE    // Exact phrase match
} SearchMode;

/**
 * Create a search engine
 * @param index: InvertedIndex
 * @param tokenizer: Tokenizer for query processing
 */
typedef struct {
    InvertedIndex* index;
    void* tokenizer;  // Tokenizer pointer
} SearchEngine;

SearchEngine* search_engine_create(InvertedIndex* index, void* tokenizer);

/**
 * Destroy the search engine
 */
void search_engine_destroy(SearchEngine* engine);

/**
 * Execute a search query
 * @param engine: SearchEngine instance
 * @param query: Search query string
 * @param mode: Search mode (AND/OR/PHRASE)
 * @param maxResults: Maximum results to return
 * @return: SearchResultSet with results sorted by relevance
 */
SearchResultSet* search_engine_search(SearchEngine* engine, const char* query,
                                     SearchMode mode, size_t maxResults);

/**
 * Free search results
 */
void search_free_results(SearchResultSet* results);

/**
 * Calculate TF-IDF score
 */
float search_calculate_tfidf(uint32_t termFreq, uint32_t docLength,
                             uint32_t docsWithTerm, uint32_t totalDocs);

/**
 * Calculate BM25 score
 * BM25 parameters: k1=1.5, b=0.75
 */
float search_calculate_bm25(uint32_t termFreq, uint32_t docLength,
                            float avgDocLength, uint32_t docsWithTerm,
                            uint32_t totalDocs);

#endif // SEARCH_H
