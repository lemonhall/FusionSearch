/**
 * @file test_hybrid.c
 * @brief 混合检索测试 - BM25 + 向量检索
 */

#include "index.h"
#include "tokenizer.h"
#include "search.h"
#include "vector_index.h"
#include "file_loader.h"
#include <stdio.h>
#include <stdlib.h>

// 创建测试数据文件
static void create_test_data(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "❌ 无法创建测试文件\n");
        return;
    }
    
    // 写入测试文档（包含 embedding）
    fprintf(f, "{\"title\": \"番茄炒蛋\", \"content\": \"简单快手的家常菜，鸡蛋打散炒熟备用，番茄切块翻炒出汁，加入鸡蛋翻炒均匀\", \"embedding\": [0.1, 0.2, 0.3, 0.4, 0.5]}\n");
    fprintf(f, "{\"title\": \"宫保鸡丁\", \"content\": \"经典川菜，鸡肉切丁腌制，花生米炸香，鸡丁滑炒后加入宫保汁和花生翻炒\", \"embedding\": [0.2, 0.3, 0.1, 0.5, 0.4]}\n");
    fprintf(f, "{\"title\": \"红烧肉\", \"content\": \"五花肉切块焯水，冰糖炒色后加入肉块翻炒上色，加水炖煮一小时\", \"embedding\": [0.3, 0.1, 0.4, 0.2, 0.5]}\n");
    fprintf(f, "{\"title\": \"麻婆豆腐\", \"content\": \"川菜代表菜，豆腐切块焯水，肉末炒香加豆瓣酱，加入豆腐烧制入味\", \"embedding\": [0.4, 0.5, 0.2, 0.3, 0.1]}\n");
    fprintf(f, "{\"title\": \"青椒肉丝\", \"content\": \"快手家常菜，猪肉切丝腌制，青椒切丝，肉丝滑炒后加青椒快速翻炒\", \"embedding\": [0.5, 0.4, 0.5, 0.1, 0.2]}\n");
    
    fclose(f);
    printf("✓ 测试数据文件创建成功: %s\n\n", filename);
}

int main(void) {
    printf("========================================\n");
    printf("混合检索测试 (BM25 + 向量检索)\n");
    printf("========================================\n\n");
    
    const char* test_file = "test_recipes.jsonl";
    const uint32_t vector_dim = 5; // 测试用 5 维向量
    
    // 1. 创建测试数据
    printf("1. 创建测试数据\n");
    create_test_data(test_file);
    
    // 2. 初始化组件
    printf("2. 初始化搜索引擎组件\n");
    
    InvertedIndex* index = index_create();
    if (!index) {
        fprintf(stderr, "❌ 创建倒排索引失败\n");
        return 1;
    }
    
    VectorIndex* vector_index = vector_index_create(vector_dim);
    if (!vector_index) {
        fprintf(stderr, "❌ 创建向量索引失败\n");
        index_destroy(index);
        return 1;
    }
    
    Tokenizer* tokenizer = tokenizer_create(NULL);
    if (!tokenizer) {
        fprintf(stderr, "❌ 创建分词器失败\n");
        vector_index_free(vector_index);
        index_destroy(index);
        return 1;
    }
    
    printf("✓ BM25 索引已创建\n");
    printf("✓ 向量索引已创建 (维度: %u)\n", vector_dim);
    printf("✓ 分词器已创建\n\n");
    
    // 3. 加载文档（同时构建 BM25 和向量索引）
    printf("3. 加载文档（混合索引）\n");
    printf("────────────────────────────────────────\n");
    
    int doc_count = file_loader_load_jsonl_with_vectors(
        test_file, index, vector_index, tokenizer, 1
    );
    
    if (doc_count <= 0) {
        fprintf(stderr, "❌ 加载文档失败: %s\n", file_loader_get_error());
        tokenizer_destroy(tokenizer);
        vector_index_free(vector_index);
        index_destroy(index);
        return 1;
    }
    
    printf("────────────────────────────────────────\n\n");
    
    // 4. BM25 检索测试
    printf("4. BM25 全文检索测试\n");
    printf("────────────────────────────────────────\n");
    
    const char* query1 = "川菜";
    printf("查询: \"%s\"\n\n", query1);
    
    SearchEngine* engine = search_engine_create(index, tokenizer);
    if (!engine) {
        fprintf(stderr, "❌ 创建搜索引擎失败\n");
        tokenizer_destroy(tokenizer);
        vector_index_free(vector_index);
        index_destroy(index);
        return 1;
    }
    
    SearchResultSet* results = search_engine_search(engine, query1, SEARCH_OR, 10);
    if (results && results->count > 0) {
        printf("BM25 结果 (共 %zu 个):\n", results->count);
        for (size_t i = 0; i < results->count; i++) {
            Document* doc = index_get_document(index, results->results[i].docId);
            printf("  #%zu [%.4f] %s\n", 
                   i + 1, 
                   results->results[i].score,
                   doc ? doc->title : "Unknown");
        }
    } else {
        printf("❌ 无结果\n");
    }
    printf("\n");
    
    search_free_results(results);
    printf("────────────────────────────────────────\n\n");
    
    // 5. 向量检索测试
    printf("5. 向量语义检索测试\n");
    printf("────────────────────────────────────────\n");
    
    // 模拟查询向量（与"宫保鸡丁"相似）
    float query_vector[5] = {0.2, 0.3, 0.1, 0.5, 0.4};
    
    printf("查询向量: [%.1f, %.1f, %.1f, %.1f, %.1f]\n\n", 
           query_vector[0], query_vector[1], query_vector[2], 
           query_vector[3], query_vector[4]);
    
    size_t vec_result_count = 0;
    VectorResult* vec_results = vector_search(vector_index, query_vector, 3, &vec_result_count);
    
    if (vec_results && vec_result_count > 0) {
        printf("向量检索结果 (Top-3):\n");
        for (size_t i = 0; i < vec_result_count; i++) {
            // 获取文档信息
            Document* doc = index_get_document(index, vec_results[i].doc_id);
            if (doc) {
                printf("  #%zu [%.4f] %s\n", 
                       i + 1,
                       vec_results[i].similarity,
                       doc->title);
            }
        }
        free(vec_results);
    } else {
        printf("❌ 无结果\n");
    }
    printf("\n");
    printf("────────────────────────────────────────\n\n");
    
    // 6. 统计信息
    printf("6. 索引统计\n");
    printf("────────────────────────────────────────\n");
    printf("文档总数: %u\n", index->totalDocs);
    printf("BM25 词项数: %zu\n", index->entryCount);
    printf("向量索引数: %u\n", vector_index_count(vector_index));
    printf("向量维度: %u\n", vector_index_dimension(vector_index));
    printf("────────────────────────────────────────\n\n");
    
    // 清理资源
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    vector_index_free(vector_index);
    index_destroy(index);
    
    // 清理测试文件
    remove(test_file);
    
    printf("========================================\n");
    printf("✅ 混合检索测试完成！\n");
    printf("========================================\n");
    
    return 0;
}
