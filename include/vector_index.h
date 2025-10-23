/**
 * @file vector_index.h
 * @brief 向量索引模块 - 基于余弦相似度的语义检索
 * 
 * 功能特性：
 * - 暴力向量检索（适用于 < 10万文档）
 * - 余弦相似度计算
 * - Top-K 结果排序
 * - 与 BM25 共享文档加载体系
 * 
 * 设计理念：
 * - 纯 C99 标准库实现，零外部依赖
 * - 向量计算由外部工具（Python）离线完成
 * - C 模块仅负责向量加载与检索
 */

#ifndef VECTOR_INDEX_H
#define VECTOR_INDEX_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 单个文档的向量条目
 */
typedef struct {
    uint32_t doc_id;        // 文档 ID（与 BM25 索引共享）
    float* embedding;       // 向量数据（动态分配）
} VectorEntry;

/**
 * @brief 向量索引结构
 */
typedef struct {
    VectorEntry* entries;   // 向量数组
    uint32_t count;         // 当前向量数量
    uint32_t capacity;      // 数组容量
    uint32_t dimension;     // 向量维度（如 768）
} VectorIndex;

/**
 * @brief 向量检索结果
 */
typedef struct {
    uint32_t doc_id;        // 文档 ID
    float similarity;       // 余弦相似度 [0, 1]
} VectorResult;

/**
 * @brief 创建向量索引
 * 
 * @param dimension 向量维度（如 768）
 * @return 新创建的向量索引，失败返回 NULL
 */
VectorIndex* vector_index_create(uint32_t dimension);

/**
 * @brief 添加向量到索引
 * 
 * @param index 向量索引
 * @param doc_id 文档 ID
 * @param embedding 向量数据（会被复制，调用者可释放原数据）
 * @return 0 成功，-1 失败
 */
int vector_index_add(VectorIndex* index, uint32_t doc_id, const float* embedding);

/**
 * @brief 向量检索 Top-K
 * 
 * 使用暴力检索：遍历所有向量，计算余弦相似度，返回最相似的 K 个结果
 * 
 * @param index 向量索引
 * @param query_embedding 查询向量
 * @param k 返回结果数量
 * @param result_count [输出] 实际返回结果数量
 * @return 检索结果数组（调用者负责释放），失败返回 NULL
 */
VectorResult* vector_search(VectorIndex* index, 
                            const float* query_embedding,
                            uint32_t k, 
                            size_t* result_count);

/**
 * @brief 计算余弦相似度
 * 
 * 公式: cosine_similarity(A, B) = dot(A, B) / (||A|| * ||B||)
 * 
 * @param vec1 向量1
 * @param vec2 向量2
 * @param dimension 向量维度
 * @return 余弦相似度 [0, 1]，0表示完全不相关，1表示完全相同
 */
float cosine_similarity(const float* vec1, const float* vec2, uint32_t dimension);

/**
 * @brief 释放向量索引资源
 * 
 * @param index 向量索引
 */
void vector_index_free(VectorIndex* index);

/**
 * @brief 获取索引中的向量数量
 * 
 * @param index 向量索引
 * @return 向量数量
 */
uint32_t vector_index_count(const VectorIndex* index);

/**
 * @brief 获取向量维度
 * 
 * @param index 向量索引
 * @return 向量维度
 */
uint32_t vector_index_dimension(const VectorIndex* index);

#endif // VECTOR_INDEX_H
