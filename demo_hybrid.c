/**
 * 混合搜索引擎演示程序
 * 
 * 功能：
 * 1. 加载带向量的JSONL文件（recipes_with_embeddings.jsonl）
 * 2. 构建BM25索引和向量索引
 * 3. 导出向量索引为二进制文件（vectors.bin）
 * 4. 导出文档元数据为JSON（documents.json）
 * 5. 可选：执行测试搜索
 * 
 * Python端将读取这些文件执行混合搜索
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "trie.h"
#include "tokenizer.h"
#include "cjk_tokenizer.h"
#include "index.h"
#include "search.h"
#include "file_loader.h"
#include "vector_index.h"
#include "utils.h"

/**
 * 导出文档元数据为JSON
 */
int export_documents_json(InvertedIndex* index, const char* output_file) {
    if (!index || !output_file) {
        fprintf(stderr, "Error: NULL parameter\n");
        return -1;
    }
    
    FILE* fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open %s for writing\n", output_file);
        return -1;
    }
    
    fprintf(fp, "[\n");
    
    for (size_t i = 0; i < index->docStore->docCount; i++) {
        Document* doc = &index->docStore->docs[i];
        
        // 转义JSON字符串中的特殊字符
        char* escaped_title = string_dup(doc->title);
        char* escaped_content = string_dup(doc->content);
        
        // 简化版：假设没有特殊字符（生产环境需要完整的JSON转义）
        
        fprintf(fp, "  {\n");
        fprintf(fp, "    \"id\": %u,\n", doc->docId);
        fprintf(fp, "    \"title\": \"%s\",\n", escaped_title);
        fprintf(fp, "    \"content\": \"%s\"\n", escaped_content);
        fprintf(fp, "  }");
        
        if (i < index->docStore->docCount - 1) {
            fprintf(fp, ",");
        }
        fprintf(fp, "\n");
        
        free(escaped_title);
        free(escaped_content);
    }
    
    fprintf(fp, "]\n");
    fclose(fp);
    
    printf("✓ Exported %zu documents to %s\n", index->docStore->docCount, output_file);
    return 0;
}

/**
 * 主程序
 */
int main(int argc, char* argv[]) {
    printf("========================================\n");
    printf("  混合搜索引擎 - 数据准备程序\n");
    printf("========================================\n\n");
    
    // 默认文件路径
    const char* input_file = "recipes_with_embeddings.jsonl";
    const char* vector_output = "vectors.bin";
    const char* doc_output = "documents.json";
    
    // 解析命令行参数
    if (argc >= 2) {
        input_file = argv[1];
    }
    if (argc >= 3) {
        vector_output = argv[2];
    }
    if (argc >= 4) {
        doc_output = argv[3];
    }
    
    printf("配置:\n");
    printf("  输入文件: %s\n", input_file);
    printf("  向量输出: %s\n", vector_output);
    printf("  文档输出: %s\n", doc_output);
    printf("\n");
    
    // 初始化 CJK 分词器
    if (cjk_is_available()) {
        cjk_tokenizer_init("zh");
        printf("✓ CJK tokenizer enabled\n");
    } else {
        printf("⚠ CJK tokenizer disabled\n");
    }
    
    // 创建数据结构
    Trie* dictionary = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dictionary);
    
    // 自动检测向量维度
    VectorIndex* vectorIndex = NULL;
    FILE* test_file = fopen(input_file, "r");
    if (test_file) {
        char test_line[65536];
        if (fgets(test_line, sizeof(test_line), test_file)) {
            // 查找embedding数组并计数
            const char* embed_start = strstr(test_line, "\"embedding\": [");
            if (embed_start) {
                embed_start += strlen("\"embedding\": [");
                uint32_t dim = 0;
                const char* ptr = embed_start;
                
                while (*ptr && *ptr != ']') {
                    // 跳过空白和逗号
                    while (*ptr && (isspace(*ptr) || *ptr == ',')) ptr++;
                    if (*ptr == ']') break;
                    
                    // 找到一个数字
                    char* end;
                    strtof(ptr, &end);
                    if (ptr != end) {
                        dim++;
                        ptr = end;
                    } else {
                        break;
                    }
                }
                
                if (dim > 0) {
                    printf("✓ 检测到向量维度: %u\n", dim);
                    vectorIndex = vector_index_create(dim);
                }
            }
        }
        fclose(test_file);
    }
    
    if (!vectorIndex) {
        printf("⚠ 未检测到向量数据，将仅加载BM25索引\n");
    }
    printf("\n");
    
    printf("\n========================================\n");
    printf("加载文档和向量...\n");
    printf("========================================\n");
    
    // 加载文档（包含向量）
    int doc_count = file_loader_load_jsonl_with_vectors(
        input_file,
        index,
        vectorIndex,
        tokenizer,
        0  // startDocId
    );
    
    if (doc_count <= 0) {
        fprintf(stderr, "❌ Failed to load documents: %s\n", 
                file_loader_get_error());
        goto cleanup;
    }
    
    printf("\n✅ 成功加载 %d 个文档\n\n", doc_count);
    
    // 打印统计信息
    index_print_stats(index);
    
    printf("\n向量索引统计:\n");
    printf("  文档数量: %u\n", vector_index_count(vectorIndex));
    printf("  向量维度: %u\n", vector_index_dimension(vectorIndex));
    printf("\n");
    
    // 导出向量索引
    printf("========================================\n");
    printf("导出向量索引...\n");
    printf("========================================\n");
    
    if (vector_index_save(vectorIndex, vector_output) != 0) {
        fprintf(stderr, "❌ Failed to export vectors\n");
        goto cleanup;
    }
    
    // 导出文档元数据
    printf("\n========================================\n");
    printf("导出文档元数据...\n");
    printf("========================================\n");
    
    if (export_documents_json(index, doc_output) != 0) {
        fprintf(stderr, "❌ Failed to export documents\n");
        goto cleanup;
    }
    
    printf("\n========================================\n");
    printf("✅ 数据准备完成！\n");
    printf("========================================\n\n");
    
    printf("下一步：\n");
    printf("1. 运行 Python 搜索引擎:\n");
    printf("   python search_api.py\n\n");
    printf("2. 或在代码中导入:\n");
    printf("   from search_api import HybridSearchEngine\n");
    printf("   engine = HybridSearchEngine()\n");
    printf("   results = engine.search(\"你的查询\")\n\n");
    
    // 可选：测试BM25搜索
    printf("是否执行测试搜索？(y/n): ");
    char choice;
    if (scanf(" %c", &choice) == 1 && (choice == 'y' || choice == 'Y')) {
        SearchEngine* engine = search_engine_create(index, tokenizer);
        
        printf("\n输入测试查询: ");
        char query[256];
        scanf(" %[^\n]", query);
        
        printf("\n执行 BM25 搜索: \"%s\"\n", query);
        printf("========================================\n");
        
        SearchResultSet* results = search_engine_search(
            engine, query, SEARCH_BM25, 10
        );
        
        if (results->count == 0) {
            printf("未找到结果\n");
        } else {
            printf("找到 %zu 个结果:\n\n", results->count);
            
            for (size_t i = 0; i < results->count; i++) {
                printf("[%zu] Score: %.4f\n", i + 1, results->results[i].score);
                printf("    Title: %s\n", results->results[i].title);
                if (results->results[i].snippet) {
                    printf("    Snippet: %s\n", results->results[i].snippet);
                }
                printf("\n");
            }
        }
        
        search_free_results(results);
        search_engine_destroy(engine);
    }
    
cleanup:
    // 清理资源
    if (vectorIndex) vector_index_free(vectorIndex);
    if (tokenizer) tokenizer_destroy(tokenizer);
    if (index) index_destroy(index);
    if (dictionary) trie_destroy(dictionary);
    cjk_tokenizer_cleanup();
    
    return 0;
}
