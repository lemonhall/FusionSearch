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

// ============================================================================
// C FFI 导出接口（供 Python/Swift/Kotlin 调用）
// ============================================================================

/**
 * @brief [FFI导出] 从文件加载索引（全局单例）
 * 
 * @param jsonl_file JSONL文档文件路径
 * @return 索引指针地址（intptr），失败返回 0
 */
uintptr_t ffi_index_load(const char* jsonl_file);

/**
 * @brief [FFI导出] 执行BM25搜索
 * 
 * @param index_ptr 索引指针地址（由 ffi_index_load 返回）
 * @param query 查询字符串
 * @param k Top-K数量
 * @param out_doc_ids [输出] 文档ID数组（调用者分配，至少k个元素）
 * @param out_scores [输出] BM25分数数组（调用者分配，至少k个元素）
 * @return 实际返回结果数量
 */
size_t ffi_bm25_search(uintptr_t index_ptr,
                       const char* query,
                       uint32_t k,
                       uint32_t* out_doc_ids,
                       float* out_scores);

/**
 * @brief [FFI导出] 根据文档ID获取文档内容
 * 
 * @param index_ptr 索引指针地址
 * @param doc_id 文档ID
 * @param out_title [输出] 标题缓冲区（调用者分配，建议256字节）
 * @param title_size 标题缓冲区大小
 * @param out_content [输出] 内容缓冲区（调用者分配，建议2048字节）
 * @param content_size 内容缓冲区大小
 * @return 0成功，-1失败
 */
int ffi_get_document(uintptr_t index_ptr,
                     uint32_t doc_id,
                     char* out_title,
                     size_t title_size,
                     char* out_content,
                     size_t content_size);

/**
 * @brief [FFI导出] 释放索引
 * 
 * @param index_ptr 索引指针地址
 */
void ffi_index_free(uintptr_t index_ptr);

#endif // SEARCH_H
