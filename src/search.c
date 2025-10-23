#include "search.h"
#include "utils.h"
#include "bm25.h"
#include "snippet.h"
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

/**
 * Compare function for qsort (sort by score descending)
 */
static int search_result_compare(const void* a, const void* b) {
    const SearchResult* ra = (const SearchResult*)a;
    const SearchResult* rb = (const SearchResult*)b;
    
    if (ra->score > rb->score) return -1;
    if (ra->score < rb->score) return 1;
    return 0;
}

/**
 * Calculate average document length in the index
 */
static float get_average_doc_length(InvertedIndex* index) {
    if (!index || index->docStore->docCount == 0) return 0.0f;
    
    size_t totalWords = 0;
    for (size_t i = 0; i < index->docStore->docCount; i++) {
        totalWords += index->docStore->docs[i].wordCount;
    }
    
    return (float)totalWords / (float)index->docStore->docCount;
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
    
    // Intersect all posting lists - start with first posting list
    for (size_t i = 0; i < postingCounts[0] && resultCount < maxResults; i++) {
        uint32_t docId = postingLists[0][i];
        bool inAllLists = true;
        
        // Check if docId is in all other posting lists
        for (size_t j = 1; j < termCount && inAllLists; j++) {
            bool found = false;
            for (size_t k = 0; k < postingCounts[j]; k++) {
                if (postingLists[j][k] == docId) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                inAllLists = false;
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

/**
 * Perform BM25 search - best for most queries
 */
static size_t search_bm25_query(SearchEngine* engine, const char** queryTerms,
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
        
        case SEARCH_BM25:
            docCount = search_bm25_query(engine, (const char**)queryTokens->tokens,
                                        queryTokens->count, docIds, maxResults);
            break;
        
        case SEARCH_PHRASE:
            // TODO: Implement phrase search
            docCount = 0;
            break;
        
        default:
            docCount = 0;
    }
    
    // Calculate scores using TF-IDF or BM25
    InvertedIndex* index = engine->index;
    float avgDocLength = get_average_doc_length(index);
    BM25Params* bm25Params = NULL;
    
    if (mode == SEARCH_BM25) {
        bm25Params = bm25_params_create();
    }
    
    for (size_t i = 0; i < docCount && i < MAX_SEARCH_RESULTS; i++) {
        uint32_t docId = docIds[i];
        Document* doc = index_get_document(index, docId);
        
        if (!doc) continue;
        
        // Calculate score for this document
        float totalScore = 0.0f;
        
        for (size_t j = 0; j < queryTokens->count; j++) {
            const char* term = queryTokens->tokens[j];
            size_t postingCount = 0;
            PostingEntry** postings = index_get_postings(index, term, &postingCount);
            
            if (!postings) continue;
            
            // Find term frequency in this document
            uint32_t termFreq = 0;
            for (size_t k = 0; k < postingCount; k++) {
                if (postings[k]->docId == docId) {
                    termFreq = postings[k]->frequency;
                    break;
                }
            }
            
            if (termFreq > 0) {
                float score = 0.0f;
                
                if (mode == SEARCH_BM25) {
                    // Calculate BM25 score
                    float idf = bm25_calculate_idf((uint32_t)index->totalDocs, 
                                                  (uint32_t)postingCount);
                    score = bm25_calculate_score((uint32_t)termFreq,
                                               (uint32_t)doc->wordCount,
                                               avgDocLength,
                                               idf,
                                               bm25Params);
                } else {
                    // Calculate TF-IDF score
                    score = search_calculate_tfidf(
                        termFreq, 
                        (uint32_t)doc->wordCount,
                        (uint32_t)postingCount,
                        index->totalDocs
                    );
                }
                
                totalScore += score;
            }
        }
        
        resultSet->results[resultSet->count].docId = docId;
        resultSet->results[resultSet->count].score = totalScore;
        resultSet->results[resultSet->count].title = string_dup(doc->title);
        
        // Generate snippet with keyword highlighting
        resultSet->results[resultSet->count].snippet = snippet_generate(
            doc->content,
            (const char**)queryTokens->tokens,
            queryTokens->count,
            150,      // 150 character snippet
            ">>"      // Highlight marker
        );
        
        resultSet->count++;
    }
    
    // Clean up BM25 parameters
    if (bm25Params) {
        bm25_params_destroy(bm25Params);
    }
    
    // Sort results by score (descending)
    if (resultSet->count > 1) {
        qsort(resultSet->results, resultSet->count, sizeof(SearchResult),
              search_result_compare);
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

// ============================================================================
// FFI 导出接口实现
// ============================================================================

#include <stdint.h>
#include "file_loader.h"

// 全局索引和引擎（简化版，生产环境应使用完整的生命周期管理）
static InvertedIndex* g_index = NULL;
static Tokenizer* g_tokenizer = NULL;
static SearchEngine* g_engine = NULL;
static Trie* g_dictionary = NULL;

uintptr_t ffi_index_load(const char* jsonl_file) {
    if (!jsonl_file) {
        fprintf(stderr, "Error: NULL file path\n");
        return 0;
    }
    
    // 清理旧索引
    if (g_engine) search_engine_destroy(g_engine);
    if (g_index) index_destroy(g_index);
    if (g_tokenizer) tokenizer_destroy(g_tokenizer);
    if (g_dictionary) trie_destroy(g_dictionary);
    
    // 创建新索引
    g_dictionary = trie_create();
    g_index = index_create();
    g_tokenizer = tokenizer_create(g_dictionary);
    
    // 加载JSONL文档
    int doc_count = file_loader_load_jsonl(jsonl_file, g_index, g_tokenizer, 0);
    
    if (doc_count <= 0) {
        fprintf(stderr, "Error: Failed to load documents: %s\n",
                file_loader_get_error());
        return 0;
    }
    
    // 创建搜索引擎
    g_engine = search_engine_create(g_index, g_tokenizer);
    
    printf("✓ Loaded %d documents for BM25 search\n", doc_count);
    
    return (uintptr_t)g_index;
}

size_t ffi_bm25_search(uintptr_t index_ptr,
                       const char* query,
                       uint32_t k,
                       uint32_t* out_doc_ids,
                       float* out_scores) {
    if (!index_ptr || !query || !out_doc_ids || !out_scores) {
        fprintf(stderr, "Error: NULL parameter in ffi_bm25_search\n");
        return 0;
    }
    
    if (!g_engine) {
        fprintf(stderr, "Error: Search engine not initialized\n");
        return 0;
    }
    
    // 执行BM25搜索
    SearchResultSet* results = search_engine_search(g_engine, query, SEARCH_BM25, k);
    
    if (!results) {
        return 0;
    }
    
    // 复制结果
    size_t count = results->count;
    for (size_t i = 0; i < count; i++) {
        out_doc_ids[i] = results->results[i].docId;
        out_scores[i] = results->results[i].score;
    }
    
    search_free_results(results);
    
    return count;
}

// UTF-8安全的字符串复制：确保不在多字节字符中间截断
static void utf8_safe_copy(char* dest, const char* src, size_t dest_size) {
    if (dest_size == 0) return;
    
    size_t src_len = strlen(src);
    
    // 如果源字符串完全放得下，直接复制
    if (src_len < dest_size) {
        strcpy(dest, src);
        return;
    }
    
    // 否则需要在UTF-8字符边界截断
    size_t copy_len = dest_size - 1;
    
    // 向前查找，找到最后一个完整UTF-8字符的结尾
    while (copy_len > 0) {
        unsigned char c = (unsigned char)src[copy_len];
        
        // 如果是ASCII或UTF-8字符的起始字节，这里是安全边界
        if ((c & 0x80) == 0 || (c & 0xC0) == 0xC0) {
            break;
        }
        
        // 这是一个UTF-8续字节(10xxxxxx)，继续向前找
        copy_len--;
    }
    
    // 复制并添加终止符
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
}

int ffi_get_document(uintptr_t index_ptr,
                     uint32_t doc_id,
                     char* out_title,
                     size_t title_size,
                     char* out_content,
                     size_t content_size) {
    if (!index_ptr || !out_title || !out_content) {
        fprintf(stderr, "Error: NULL parameter in ffi_get_document\n");
        return -1;
    }
    
    InvertedIndex* index = (InvertedIndex*)index_ptr;
    Document* doc = index_get_document(index, doc_id);
    
    if (!doc) {
        fprintf(stderr, "Error: Document %u not found\n", doc_id);
        return -1;
    }
    
    // 使用UTF-8安全的复制函数
    utf8_safe_copy(out_title, doc->title, title_size);
    utf8_safe_copy(out_content, doc->content, content_size);
    
    return 0;
}

void ffi_index_free(uintptr_t index_ptr) {
    (void)index_ptr;  // 忽略参数，使用全局变量
    
    if (g_engine) {
        search_engine_destroy(g_engine);
        g_engine = NULL;
    }
    if (g_index) {
        index_destroy(g_index);
        g_index = NULL;
    }
    if (g_tokenizer) {
        tokenizer_destroy(g_tokenizer);
        g_tokenizer = NULL;
    }
    if (g_dictionary) {
        trie_destroy(g_dictionary);
        g_dictionary = NULL;
    }
}
