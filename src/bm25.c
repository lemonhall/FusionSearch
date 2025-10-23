#include "bm25.h"
#include <stdlib.h>
#include <math.h>

/**
 * Create BM25 parameters with default values
 */
BM25Params* bm25_params_create(void) {
    return bm25_params_create_custom(1.5f, 0.75f, 8.0f);
}

/**
 * Create BM25 parameters with custom values
 */
BM25Params* bm25_params_create_custom(float k1, float b, float k3) {
    BM25Params* params = (BM25Params*)malloc(sizeof(BM25Params));
    if (params) {
        params->k1 = k1;
        params->b = b;
        params->k3 = k3;
    }
    return params;
}

/**
 * Calculate average document length in the corpus
 */
float bm25_get_average_doc_length(InvertedIndex* index) {
    if (!index || !index->docStore || index->docStore->docCount == 0) {
        return 0.0f;
    }

    uint32_t total_length = 0;
    for (uint32_t i = 0; i < index->docStore->docCount; i++) {
        if (index->docStore->docs[i].wordCount > 0) {
            total_length += (uint32_t)index->docStore->docs[i].wordCount;
        }
    }

    return (float)total_length / (float)index->docStore->docCount;
}

/**
 * Calculate IDF (Inverse Document Frequency) for a term
 * IDF = log((N - n + 0.5) / (n + 0.5))
 * where N = total documents, n = documents containing term
 */
float bm25_calculate_idf(uint32_t total_docs, uint32_t docs_with_term) {
    if (total_docs == 0 || docs_with_term > total_docs) {
        return 0.0f;
    }

    // Prevent division by zero
    if (docs_with_term == 0) {
        docs_with_term = 1;
    }

    float numerator = (float)(total_docs - docs_with_term + 0.5f);
    float denominator = (float)(docs_with_term + 0.5f);

    return logf(numerator / denominator);
}

/**
 * Calculate BM25 score for a term in a document
 * 
 * BM25(D,Q) = Σ IDF(q_i) * ((k1 + 1) * f(q_i, D)) / (f(q_i, D) + k1 * (1 - b + b * |D| / avgdl))
 * 
 * where:
 *   D = document
 *   Q = query terms
 *   q_i = individual query term
 *   f(q_i, D) = frequency of q_i in D
 *   |D| = document length
 *   avgdl = average document length
 *   k1, b = parameters
 */
float bm25_calculate_score(uint32_t term_freq, uint32_t doc_length,
                           float avg_doc_length, float idf,
                           const BM25Params* params) {
    if (!params || term_freq == 0) {
        return 0.0f;
    }

    // Avoid division by zero
    if (avg_doc_length == 0.0f) {
        avg_doc_length = 1.0f;
    }

    // Calculate numerator: (k1 + 1) * tf
    float numerator = (params->k1 + 1.0f) * (float)term_freq;

    // Calculate denominator: tf + k1 * (1 - b + b * |D| / avgdl)
    float length_norm = 1.0f - params->b + params->b * ((float)doc_length / avg_doc_length);
    float denominator = (float)term_freq + params->k1 * length_norm;

    // Avoid division by zero
    if (denominator == 0.0f) {
        denominator = 1.0f;
    }

    return idf * (numerator / denominator);
}

/**
 * Free BM25 parameters
 */
void bm25_params_destroy(BM25Params* params) {
    if (params) {
        free(params);
    }
}
