/**
 * @file vector_index.c
 * @brief 向量索引实现 - 暴力检索方案
 */

#include "vector_index.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// 初始容量
#define INITIAL_CAPACITY 1024

/**
 * @brief 扩容向量数组
 */
static int resize_index(VectorIndex* index) {
    uint32_t new_capacity = index->capacity * 2;
    VectorEntry* new_entries = realloc(index->entries, 
                                       new_capacity * sizeof(VectorEntry));
    if (!new_entries) {
        fprintf(stderr, "Error: Failed to resize vector index\n");
        return -1;
    }
    
    index->entries = new_entries;
    index->capacity = new_capacity;
    return 0;
}

/**
 * @brief 比较函数（用于 qsort，按相似度降序）
 */
static int compare_results_desc(const void* a, const void* b) {
    const VectorResult* ra = (const VectorResult*)a;
    const VectorResult* rb = (const VectorResult*)b;
    
    // 降序：大的在前
    if (ra->similarity > rb->similarity) return -1;
    if (ra->similarity < rb->similarity) return 1;
    return 0;
}

// ============================================================================
// 公共接口实现
// ============================================================================

VectorIndex* vector_index_create(uint32_t dimension) {
    if (dimension == 0) {
        fprintf(stderr, "Error: Invalid dimension: %u\n", dimension);
        return NULL;
    }
    
    VectorIndex* index = malloc(sizeof(VectorIndex));
    if (!index) {
        fprintf(stderr, "Error: Failed to allocate VectorIndex\n");
        return NULL;
    }
    
    index->entries = malloc(INITIAL_CAPACITY * sizeof(VectorEntry));
    if (!index->entries) {
        fprintf(stderr, "Error: Failed to allocate entries array\n");
        free(index);
        return NULL;
    }
    
    index->count = 0;
    index->capacity = INITIAL_CAPACITY;
    index->dimension = dimension;
    
    return index;
}

int vector_index_add(VectorIndex* index, uint32_t doc_id, const float* embedding) {
    if (!index || !embedding) {
        fprintf(stderr, "Error: NULL pointer in vector_index_add\n");
        return -1;
    }
    
    // 扩容检查
    if (index->count >= index->capacity) {
        if (resize_index(index) != 0) {
            return -1;
        }
    }
    
    // 分配向量内存并复制数据
    float* vec_copy = malloc(index->dimension * sizeof(float));
    if (!vec_copy) {
        fprintf(stderr, "Error: Failed to allocate embedding memory\n");
        return -1;
    }
    memcpy(vec_copy, embedding, index->dimension * sizeof(float));
    
    // 添加到索引
    index->entries[index->count].doc_id = doc_id;
    index->entries[index->count].embedding = vec_copy;
    index->count++;
    
    return 0;
}

float cosine_similarity(const float* vec1, const float* vec2, uint32_t dimension) {
    if (!vec1 || !vec2 || dimension == 0) {
        return 0.0f;
    }
    
    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    // 单次遍历计算点积和范数
    for (uint32_t i = 0; i < dimension; i++) {
        float v1 = vec1[i];
        float v2 = vec2[i];
        
        dot_product += v1 * v2;
        norm1 += v1 * v1;
        norm2 += v2 * v2;
    }
    
    // 避免除零
    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }
    
    // 余弦相似度 = dot(A, B) / (||A|| * ||B||)
    float similarity = dot_product / (sqrtf(norm1) * sqrtf(norm2));
    
    // 归一化到 [0, 1]（余弦相似度原本在 [-1, 1]）
    // 对于文本嵌入，通常都是正值，但为了安全起见做归一化
    if (similarity < 0.0f) similarity = 0.0f;
    if (similarity > 1.0f) similarity = 1.0f;
    
    return similarity;
}

VectorResult* vector_search(VectorIndex* index, 
                            const float* query_embedding,
                            uint32_t k, 
                            size_t* result_count) {
    if (!index || !query_embedding || k == 0 || !result_count) {
        fprintf(stderr, "Error: Invalid parameters in vector_search\n");
        if (result_count) *result_count = 0;
        return NULL;
    }
    
    if (index->count == 0) {
        *result_count = 0;
        return NULL;
    }
    
    // 分配临时结果数组（存储所有文档的相似度）
    VectorResult* candidates = malloc(index->count * sizeof(VectorResult));
    if (!candidates) {
        fprintf(stderr, "Error: Failed to allocate candidates array\n");
        *result_count = 0;
        return NULL;
    }
    
    // 计算所有文档的相似度
    for (uint32_t i = 0; i < index->count; i++) {
        candidates[i].doc_id = index->entries[i].doc_id;
        candidates[i].similarity = cosine_similarity(
            query_embedding, 
            index->entries[i].embedding,
            index->dimension
        );
    }
    
    // 排序（按相似度降序）
    qsort(candidates, index->count, sizeof(VectorResult), compare_results_desc);
    
    // 返回 Top-K
    uint32_t actual_k = (k < index->count) ? k : index->count;
    
    VectorResult* results = malloc(actual_k * sizeof(VectorResult));
    if (!results) {
        fprintf(stderr, "Error: Failed to allocate results array\n");
        free(candidates);
        *result_count = 0;
        return NULL;
    }
    
    memcpy(results, candidates, actual_k * sizeof(VectorResult));
    free(candidates);
    
    *result_count = actual_k;
    return results;
}

void vector_index_free(VectorIndex* index) {
    if (!index) return;
    
    // 释放所有向量数据
    if (index->entries) {
        for (uint32_t i = 0; i < index->count; i++) {
            if (index->entries[i].embedding) {
                free(index->entries[i].embedding);
            }
        }
        free(index->entries);
    }
    
    free(index);
}

uint32_t vector_index_count(const VectorIndex* index) {
    return index ? index->count : 0;
}

uint32_t vector_index_dimension(const VectorIndex* index) {
    return index ? index->dimension : 0;
}

int vector_index_save(const VectorIndex* index, const char* file_path) {
    if (!index || !file_path) {
        fprintf(stderr, "Error: NULL parameter in vector_index_save\n");
        return -1;
    }
    
    FILE* fp = fopen(file_path, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open file for writing: %s\n", file_path);
        return -1;
    }
    
    // 写入头部：count 和 dimension
    if (fwrite(&index->count, sizeof(uint32_t), 1, fp) != 1 ||
        fwrite(&index->dimension, sizeof(uint32_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to write header\n");
        fclose(fp);
        return -1;
    }
    
    // 写入每个向量
    for (uint32_t i = 0; i < index->count; i++) {
        // 写入 doc_id
        if (fwrite(&index->entries[i].doc_id, sizeof(uint32_t), 1, fp) != 1) {
            fprintf(stderr, "Error: Failed to write doc_id\n");
            fclose(fp);
            return -1;
        }
        
        // 写入 embedding
        if (fwrite(index->entries[i].embedding, sizeof(float), index->dimension, fp) != index->dimension) {
            fprintf(stderr, "Error: Failed to write embedding\n");
            fclose(fp);
            return -1;
        }
    }
    
    fclose(fp);
    printf("✓ Saved %u vectors to %s\n", index->count, file_path);
    return 0;
}

VectorIndex* vector_index_load(const char* file_path) {
    if (!file_path) {
        fprintf(stderr, "Error: NULL file_path\n");
        return NULL;
    }
    
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open file for reading: %s\n", file_path);
        return NULL;
    }
    
    // 读取头部
    uint32_t count, dimension;
    if (fread(&count, sizeof(uint32_t), 1, fp) != 1 ||
        fread(&dimension, sizeof(uint32_t), 1, fp) != 1) {
        fprintf(stderr, "Error: Failed to read header\n");
        fclose(fp);
        return NULL;
    }
    
    // 创建向量索引
    VectorIndex* index = vector_index_create(dimension);
    if (!index) {
        fclose(fp);
        return NULL;
    }
    
    // 读取每个向量
    for (uint32_t i = 0; i < count; i++) {
        uint32_t doc_id;
        if (fread(&doc_id, sizeof(uint32_t), 1, fp) != 1) {
            fprintf(stderr, "Error: Failed to read doc_id\n");
            vector_index_free(index);
            fclose(fp);
            return NULL;
        }
        
        // 读取 embedding
        float* embedding = malloc(dimension * sizeof(float));
        if (!embedding) {
            fprintf(stderr, "Error: Failed to allocate embedding\n");
            vector_index_free(index);
            fclose(fp);
            return NULL;
        }
        
        if (fread(embedding, sizeof(float), dimension, fp) != dimension) {
            fprintf(stderr, "Error: Failed to read embedding\n");
            free(embedding);
            vector_index_free(index);
            fclose(fp);
            return NULL;
        }
        
        // 添加到索引
        if (vector_index_add(index, doc_id, embedding) != 0) {
            fprintf(stderr, "Error: Failed to add vector to index\n");
            free(embedding);
            vector_index_free(index);
            fclose(fp);
            return NULL;
        }
        
        free(embedding);  // vector_index_add 会复制数据
    }
    
    fclose(fp);
    printf("✓ Loaded %u vectors from %s\n", index->count, file_path);
    return index;
}

void vector_free_results(VectorResult* results) {
    if (results) {
        free(results);
    }
}

// ============================================================================
// FFI 导出接口实现
// ============================================================================

#include <stdint.h>

uintptr_t ffi_vector_index_load(const char* file_path) {
    VectorIndex* index = vector_index_load(file_path);
    return (uintptr_t)index;
}

size_t ffi_vector_search(uintptr_t index_ptr,
                         const float* query_embedding,
                         uint32_t dimension,
                         uint32_t k,
                         uint32_t* out_doc_ids,
                         float* out_scores) {
    if (!index_ptr || !query_embedding || !out_doc_ids || !out_scores) {
        fprintf(stderr, "Error: NULL parameter in ffi_vector_search\n");
        return 0;
    }
    
    VectorIndex* index = (VectorIndex*)index_ptr;
    
    // 验证维度
    if (index->dimension != dimension) {
        fprintf(stderr, "Error: Dimension mismatch: expected %u, got %u\n",
                index->dimension, dimension);
        return 0;
    }
    
    // 执行检索
    size_t result_count = 0;
    VectorResult* results = vector_search(index, query_embedding, k, &result_count);
    
    if (!results) {
        return 0;
    }
    
    // 复制结果到输出数组
    for (size_t i = 0; i < result_count; i++) {
        out_doc_ids[i] = results[i].doc_id;
        out_scores[i] = results[i].similarity;
    }
    
    free(results);
    
    return result_count;
}

void ffi_vector_index_free(uintptr_t index_ptr) {
    if (index_ptr) {
        vector_index_free((VectorIndex*)index_ptr);
    }
}
