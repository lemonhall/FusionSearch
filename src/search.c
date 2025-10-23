#include "search.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include "tokenizer.h"

SearchEngine* search_engine_create(InvertedIndex* index, void* tokenizer) {
    SearchEngine* engine = (SearchEngine*)safe_malloc(sizeof(SearchEngine));
    engine->index = index;
    engine->tokenizer = tokenizer;
    return engine;
}

void search_engine_destroy(SearchEngine* engine) {
    if (engine) {
        free(engine);
    }
}

float search_calculate_tfidf(uint32_t termFreq, uint32_t docLength,
                             uint32_t docsWithTerm, uint32_t totalDocs) {
    if (totalDocs == 0 || docsWithTerm == 0) return 0.0f;
    
    // TF = term frequency / document length
    float tf = (float)termFreq / (float)(docLength + 1);
    
    // IDF = log(total docs / docs with term)
    float idf = logf((float)totalDocs / (float)docsWithTerm);
    
    return tf * idf;
}

float search_calculate_bm25(uint32_t termFreq, uint32_t docLength,
                            float avgDocLength, uint32_t docsWithTerm,
                            uint32_t totalDocs) {
    if (totalDocs == 0 || docsWithTerm == 0) return 0.0f;
    
    const float k1 = 1.5f;  // Term frequency saturation parameter
    const float b = 0.75f;  // Length normalization parameter
    
    // IDF = log((total docs - docs with term + 0.5) / (docs with term + 0.5))
    float idf = logf(((float)(totalDocs - docsWithTerm) + 0.5f) / 
                     ((float)docsWithTerm + 0.5f));
    
    // BM25 = IDF * ((k1 + 1) * TF) / (k1 * (1 - b + b * (doc_len / avg_len)) + TF)
    float numerator = (k1 + 1.0f) * (float)termFreq;
    float denominator = k1 * (1.0f - b + b * ((float)docLength / (avgDocLength + 1.0f))) 
                       + (float)termFreq;
    
    return idf * (numerator / denominator);
}

char* search_generate_snippet(const char* content, const char** terms,
                             size_t termCount, size_t snippetLength) {
    if (!content) return NULL;
    
    char* snippet = (char*)safe_malloc(snippetLength + 1);
    
    // Simple implementation: just return first part of content
    size_t copyLen = snippetLength < strlen(content) ? snippetLength : strlen(content);
    strncpy(snippet, content, copyLen);
    snippet[copyLen] = '\0';
    
    // Add ellipsis if content is longer
    if (strlen(content) > snippetLength) {
        if (copyLen >= 3) {
            snippet[copyLen - 3] = '.';
            snippet[copyLen - 2] = '.';
            snippet[copyLen - 1] = '.';
        }
    }
    
    return snippet;
}

/**
 * Compare function for qsort (sort by score descending)
 */
static int search_result_compare(const void* a, const void* b) {
    SearchResult* ra = (SearchResult*)a;
    SearchResult* rb = (SearchResult*)b;
    
    if (ra->score > rb->score) return -1;
    if (ra->score < rb->score) return 1;
    return 0;
}

/**
 * Perform AND search - all query terms must be present
 */
static size_t search_and_query(SearchEngine* engine, const char** queryTerms,
                              size_t termCount, uint32_t* results, 
                              size_t maxResults) {
    if (!engine || !queryTerms || termCount == 0) return 0;
    
    InvertedIndex* index = engine->index;
    
    // Get posting lists for all terms
    uint32_t** postingLists = (uint32_t**)safe_malloc(sizeof(uint32_t*) * termCount);
    size_t* postingCounts = (size_t*)safe_malloc(sizeof(size_t) * termCount);
    
    size_t resultCount = 0;
    
    // Get postings for first term
    postingLists[0] = (uint32_t*)safe_malloc(sizeof(uint32_t) * MAX_DOCS_PER_WORD);
    postingCounts[0] = index_search_term(index, queryTerms[0], 
                                        postingLists[0], MAX_DOCS_PER_WORD);
    
    if (postingCounts[0] == 0) {
        // First term not found, no results
        goto and_cleanup;
    }
    
    // Get postings for remaining terms
    for (size_t i = 1; i < termCount; i++) {
        postingLists[i] = (uint32_t*)safe_malloc(sizeof(uint32_t) * MAX_DOCS_PER_WORD);
        postingCounts[i] = index_search_term(index, queryTerms[i],
                                            postingLists[i], MAX_DOCS_PER_WORD);
        
        if (postingCounts[i] == 0) {
            // Any term not found, no results
            goto and_cleanup;
        }
    }
    
    // Intersect all posting lists
    // Start with first posting list
    for (size_t i = 0; i < postingCounts[0] && resultCount < maxResults; i++) {
        uint32_t docId = postingLists[0][i];
        bool inAllLists = true;
        
        // Check if docId is in all other posting lists
        for (size_t j = 1; j < termCount; j++) {
            bool found = false;
            for (size_t k = 0; k < postingCounts[j]; k++) {
                if (postingLists[j][k] == docId) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                inAllLists = false;
                break;
            }
        }
        
        if (inAllLists) {
            results[resultCount++] = docId;
        }
    }

and_cleanup:
    // Cleanup
    for (size_t i = 0; i < termCount; i++) {
        if (postingLists[i]) free(postingLists[i]);
    }
    free(postingLists);
    free(postingCounts);
    
    return resultCount;
}

/**
 * Perform OR search - any query term can be present
 */
static size_t search_or_query(SearchEngine* engine, const char** queryTerms,
                             size_t termCount, uint32_t* results,
                             size_t maxResults) {
    if (!engine || !queryTerms || termCount == 0) return 0;
    
    InvertedIndex* index = engine->index;
    
    // Use a simple set to track unique document IDs
    uint32_t uniqueDocs[MAX_DOCS] = {0};
    size_t uniqueCount = 0;
    
    // For each query term, add all its documents to the result set
    for (size_t i = 0; i < termCount; i++) {
        uint32_t* postings = (uint32_t*)safe_malloc(sizeof(uint32_t) * MAX_DOCS_PER_WORD);
        size_t postingCount = index_search_term(index, queryTerms[i],
                                               postings, MAX_DOCS_PER_WORD);
        
        // Add each document to unique set (avoid duplicates)
        for (size_t j = 0; j < postingCount; j++) {
            uint32_t docId = postings[j];
            bool alreadyExists = false;
            
            for (size_t k = 0; k < uniqueCount; k++) {
                if (uniqueDocs[k] == docId) {
                    alreadyExists = true;
                    break;
                }
            }
            
            if (!alreadyExists && uniqueCount < MAX_DOCS) {
                uniqueDocs[uniqueCount++] = docId;
            }
        }
        
        free(postings);
    }
    
    // Copy results (limit to maxResults)
    size_t resultCount = uniqueCount < maxResults ? uniqueCount : maxResults;
    for (size_t i = 0; i < resultCount; i++) {
        results[i] = uniqueDocs[i];
    }
    
    return resultCount;
}

SearchResultSet* search_engine_search(SearchEngine* engine, const char* query,
                                     SearchMode mode, size_t maxResults) {
    if (!engine || !query) return NULL;
    
    double startTime = get_time_ms();
    
    SearchResultSet* resultSet = 
        (SearchResultSet*)safe_malloc(sizeof(SearchResultSet));
    resultSet->results = (SearchResult*)safe_malloc(
        sizeof(SearchResult) * MAX_SEARCH_RESULTS);
    resultSet->count = 0;
    
    // Tokenize the query
    Tokenizer* tokenizer = (Tokenizer*)engine->tokenizer;
    TokenList* queryTokens = tokenizer_tokenize(tokenizer, query);
    
    if (!queryTokens || queryTokens->count == 0) {
        tokenizer_free_tokens(queryTokens);
        resultSet->executionTime = get_time_ms() - startTime;
        return resultSet;
    }
    
    // Perform search based on mode
    uint32_t docIds[MAX_SEARCH_RESULTS];
    size_t docCount = 0;
    
    switch (mode) {
        case SEARCH_AND:
            docCount = search_and_query(engine, (const char**)queryTokens->tokens,
                                       queryTokens->count, docIds, maxResults);
            break;
        
        case SEARCH_OR:
            docCount = search_or_query(engine, (const char**)queryTokens->tokens,
                                      queryTokens->count, docIds, maxResults);
            break;
        
        case SEARCH_PHRASE:
            // TODO: Implement phrase search
            docCount = 0;
            break;
        
        default:
            docCount = 0;
    }
    
    // Convert document IDs to results
    // For now, just store the doc IDs with default scores
    for (size_t i = 0; i < docCount && i < MAX_SEARCH_RESULTS; i++) {
        resultSet->results[i].docId = docIds[i];
        resultSet->results[i].score = 1.0f / (i + 1);  // Simple scoring: first result gets 1.0
        resultSet->results[i].title = NULL;  // TODO: Get from document store
        resultSet->results[i].snippet = NULL;
        resultSet->count++;
    }
    
    // Cleanup
    tokenizer_free_tokens(queryTokens);
    
    resultSet->executionTime = get_time_ms() - startTime;
    
    return resultSet;
}

void search_free_results(SearchResultSet* results) {
    if (!results) return;
    
    for (size_t i = 0; i < results->count; i++) {
        if (results->results[i].title) {
            free(results->results[i].title);
        }
        if (results->results[i].snippet) {
            free(results->results[i].snippet);
        }
    }
    
    free(results->results);
    free(results);
}
