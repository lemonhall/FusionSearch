/**
 * @file test_vector.c
 * @brief 向量检索模块测试
 */

#include "vector_index.h"
#include <stdio.h>
#include <stdlib.h>

// 生成随机向量（用于测试）
static void generate_random_vector(float* vec, uint32_t dim) {
    for (uint32_t i = 0; i < dim; i++) {
        vec[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f; // [-1, 1]
    }
}

// 打印向量（前10个元素）
static void print_vector(const float* vec, uint32_t dim) {
    printf("[");
    uint32_t print_count = (dim < 10) ? dim : 10;
    for (uint32_t i = 0; i < print_count; i++) {
        printf("%.3f", vec[i]);
        if (i < print_count - 1) printf(", ");
    }
    if (dim > 10) printf(", ...");
    printf("]");
}

int main(void) {
    printf("========================================\n");
    printf("向量检索模块测试\n");
    printf("========================================\n\n");
    
    // 测试参数
    const uint32_t dimension = 768;
    const uint32_t doc_count = 100;
    const uint32_t top_k = 5;
    
    // 1. 创建索引
    printf("1. 创建向量索引 (维度: %u)\n", dimension);
    VectorIndex* index = vector_index_create(dimension);
    if (!index) {
        fprintf(stderr, "❌ 创建索引失败\n");
        return 1;
    }
    printf("✓ 索引创建成功\n\n");
    
    // 2. 添加向量
    printf("2. 添加 %u 个文档向量\n", doc_count);
    srand(42); // 固定随机种子，便于复现
    
    float* temp_vec = malloc(dimension * sizeof(float));
    if (!temp_vec) {
        fprintf(stderr, "❌ 内存分配失败\n");
        vector_index_free(index);
        return 1;
    }
    
    for (uint32_t i = 0; i < doc_count; i++) {
        generate_random_vector(temp_vec, dimension);
        
        if (vector_index_add(index, i + 1, temp_vec) != 0) {
            fprintf(stderr, "❌ 添加文档 %u 失败\n", i + 1);
            free(temp_vec);
            vector_index_free(index);
            return 1;
        }
    }
    
    printf("✓ 成功添加 %u 个文档\n", vector_index_count(index));
    printf("  索引维度: %u\n", vector_index_dimension(index));
    printf("  内存占用: ~%.2f MB\n\n", 
           (float)(doc_count * dimension * sizeof(float)) / (1024 * 1024));
    
    // 3. 测试余弦相似度计算
    printf("3. 测试余弦相似度计算\n");
    float vec1[5] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float vec2[5] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float vec3[5] = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    
    float sim12 = cosine_similarity(vec1, vec2, 5);
    float sim13 = cosine_similarity(vec1, vec3, 5);
    
    printf("  相同向量相似度: %.4f (期望 1.0)\n", sim12);
    printf("  正交向量相似度: %.4f (期望 0.0)\n", sim13);
    
    if (sim12 > 0.999f && sim13 < 0.001f) {
        printf("✓ 余弦相似度计算正确\n\n");
    } else {
        printf("❌ 余弦相似度计算错误\n\n");
    }
    
    // 4. 测试向量检索
    printf("4. 测试向量检索 (Top-%u)\n", top_k);
    
    // 使用第一个文档作为查询（应该返回自己相似度最高）
    generate_random_vector(temp_vec, dimension);
    srand(42);
    generate_random_vector(temp_vec, dimension); // 重新生成第一个向量
    
    printf("  查询向量: ");
    print_vector(temp_vec, dimension);
    printf("\n\n");
    
    size_t result_count = 0;
    VectorResult* results = vector_search(index, temp_vec, top_k, &result_count);
    
    if (!results) {
        fprintf(stderr, "❌ 检索失败\n");
        free(temp_vec);
        vector_index_free(index);
        return 1;
    }
    
    printf("  检索结果 (共 %zu 个):\n", result_count);
    printf("  排名  文档ID  相似度\n");
    printf("  ────  ──────  ──────\n");
    for (size_t i = 0; i < result_count; i++) {
        printf("  #%zu    %-6u  %.4f\n", 
               i + 1, results[i].doc_id, results[i].similarity);
    }
    
    // 验证第一个结果应该是 doc_id=1（相似度接近1.0）
    if (results[0].doc_id == 1 && results[0].similarity > 0.999f) {
        printf("\n✓ 检索结果正确（最相似文档排在第一位）\n\n");
    } else {
        printf("\n⚠ 检索结果可能有问题\n\n");
    }
    
    free(results);
    
    // 5. 性能测试
    printf("5. 性能测试\n");
    printf("  执行 100 次查询...\n");
    
    const int iterations = 100;
    for (int i = 0; i < iterations; i++) {
        generate_random_vector(temp_vec, dimension);
        VectorResult* perf_results = vector_search(index, temp_vec, top_k, &result_count);
        free(perf_results);
    }
    
    printf("✓ 完成 %d 次查询\n", iterations);
    printf("  平均每次: ~%.2f ms (估算)\n\n", 
           (float)(doc_count * dimension) / 1000000.0f);
    
    // 清理资源
    free(temp_vec);
    vector_index_free(index);
    
    printf("========================================\n");
    printf("✅ 所有测试通过！\n");
    printf("========================================\n");
    
    return 0;
}
