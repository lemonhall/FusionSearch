#ifndef BM25_H
#define BM25_H

#include <stdint.h>
#include <stddef.h>
#include "index.h"

/**
 * BM25 (Best Matching 25) ranking algorithm
 * Industry-standard information retrieval ranking function
 */

typedef struct {
    float k1;           // Term frequency saturation parameter (default: 1.5)
    float b;            // Document length normalization parameter (default: 0.75)
    float k3;           // Query term frequency parameter (default: 8.0)
} BM25Params;

/**
 * Create BM25 parameters with default values
 * k1=1.5, b=0.75, k3=8.0
 */
BM25Params* bm25_params_create(void);

/**
 * Create BM25 parameters with custom values
 */
BM25Params* bm25_params_create_custom(float k1, float b, float k3);

/**
 * Calculate average document length in the corpus
 * 
 * @param index Inverted index
 * @return Average document length
 */
float bm25_get_average_doc_length(InvertedIndex* index);

/**
 * Calculate IDF (Inverse Document Frequency) for a term
 * 
 * @param total_docs Total number of documents
 * @param docs_with_term Number of documents containing the term
 * @return IDF score
 */
float bm25_calculate_idf(uint32_t total_docs, uint32_t docs_with_term);

/**
 * Calculate BM25 score for a term in a document
 * 
 * @param term_freq Term frequency in document
 * @param doc_length Document length
 * @param avg_doc_length Average document length in corpus
 * @param idf IDF score for the term
 * @param params BM25 parameters
 * @return BM25 score
 */
float bm25_calculate_score(uint32_t term_freq, uint32_t doc_length,
                           float avg_doc_length, float idf,
                           const BM25Params* params);

/**
 * Free BM25 parameters
 */
void bm25_params_destroy(BM25Params* params);

#endif // BM25_H
