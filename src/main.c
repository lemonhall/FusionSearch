#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "tokenizer.h"
#include "cjk_tokenizer.h"
#include "index.h"
#include "search.h"
#include "file_loader.h"
#include "utils.h"

// Sample documents for testing
typedef struct {
    const char* title;
    const char* content;
} SampleDoc;

SampleDoc SAMPLE_DOCUMENTS[] = {
    {
        "Python Programming Guide",
        "Python is a high-level programming language known for its simplicity and readability. "
        "It supports multiple programming paradigms including object-oriented and functional programming."
    },
    {
        "JavaScript for Web Development",
        "JavaScript is the most popular programming language for web development. "
        "It runs in browsers and can also be used on servers with Node.js framework."
    },
    {
        "C Language Fundamentals",
        "C is a powerful low-level programming language used for system software development. "
        "It provides direct memory access through pointers and is highly efficient."
    },
    {
        "Data Structures and Algorithms",
        "Understanding data structures like arrays lists and trees is essential for efficient programming. "
        "Algorithms describe step-by-step procedures for solving computational problems."
    },
    {
        "Database Design Principles",
        "Databases store and manage data efficiently using SQL and other query languages. "
        "Proper database design involves normalization and indexing for optimal performance."
    },
    // 中文文档
    {
        "机器学习入门",
        "机器学习是人工智能的核心技术。通过数据训练模型，计算机可以自动学习规律和模式。"
        "深度学习是机器学习的一个重要分支，使用神经网络进行复杂模式识别。"
    },
    {
        "数据库设计与优化",
        "SQLite是一种轻量级关系型数据库，广泛应用于移动设备和嵌入式系统。"
        "全文搜索功能可以大大提升数据查询的效率，BM25算法是搜索排序的金标准。"
    },
    // 日文文档
    {
        "日本語の全文検索",
        "これは日本語のテスト文書です。全文検索エンジンはデータベース内の文章を素早く検索できます。"
        "SQLiteデータベースは軽量で高速です。機械学習と自然言語処理にも広く利用されています。"
    },
    // 混合语言文档
    {
        "Cross-language Search Engine",
        "This search engine supports 多语言搜索 including English, 中文, and 日本語. "
        "It uses ICU for 分词 and BM25 for ランキング. Perfect for international applications!"
    }
};

#define NUM_SAMPLE_DOCS (sizeof(SAMPLE_DOCUMENTS) / sizeof(SAMPLE_DOCUMENTS[0]))

/**
 * Initialize the search engine with sample data
 */
void initialize_search_engine(InvertedIndex* index, Tokenizer* tokenizer) {
    printf("Initializing search engine...\n");
    printf("Loading %zu sample documents...\n\n", NUM_SAMPLE_DOCS);
    
    for (size_t i = 0; i < NUM_SAMPLE_DOCS; i++) {
        SampleDoc* doc = &SAMPLE_DOCUMENTS[i];
        
        printf("[%zu] %s\n", i + 1, doc->title);
        
        // Tokenize content
        TokenList* tokens = tokenizer_tokenize(tokenizer, doc->content);
        
        // Add to index
        index_add_document(index, (uint32_t)i, doc->title, doc->content,
                         (const char**)tokens->tokens, tokens->count);
        
        // Free tokens
        tokenizer_free_tokens(tokens);
    }
    
    printf("\nSearch engine initialized successfully!\n\n");
}

/**
 * Display search menu
 */
void display_menu(void) {
    printf("\n=== Search Engine Menu ===\n");
    printf("1. Perform AND search (all terms must match)\n");
    printf("2. Perform OR search (any term matches)\n");
    printf("3. Perform BM25 search (relevance ranking)\n");
    printf("4. Perform PHRASE search (exact phrase match)\n");
    printf("5. Load documents from file\n");
    printf("6. View index statistics\n");
    printf("7. View dictionary contents\n");
    printf("8. Exit\n");
    printf("Enter your choice: ");
}

/**
 * Perform a search
 */
void perform_search(SearchEngine* engine, InvertedIndex* index, SearchMode mode) {
    char query[256];
    
    printf("Enter search query: ");
    fgets(query, sizeof(query), stdin);
    
    // Remove trailing newline
    size_t len = strlen(query);
    if (len > 0 && query[len - 1] == '\n') {
        query[len - 1] = '\0';
    }
    
    if (strlen(query) == 0) {
        printf("Empty query!\n");
        return;
    }
    
    const char* modeStr = mode == SEARCH_AND ? "AND" : 
                         mode == SEARCH_OR ? "OR" : "PHRASE";
    printf("\n[Searching] Query: \"%s\" | Mode: %s\n", query, modeStr);
    printf("========================================\n");
    
    // Debug: Show tokenized query
    Tokenizer* tokenizer = (Tokenizer*)engine->tokenizer;
    TokenList* queryTokens = tokenizer_tokenize(tokenizer, query);
    printf("Tokenized into %zu terms: ", queryTokens->count);
    for (size_t i = 0; i < queryTokens->count; i++) {
        printf("[%s] ", queryTokens->tokens[i]);
    }
    printf("\n");
    
    // Debug: Show if terms exist in index
    for (size_t i = 0; i < queryTokens->count; i++) {
        size_t postingCount = 0;
        index_get_postings(index, queryTokens->tokens[i], &postingCount);
        printf("  '%s' found in %zu documents\n", queryTokens->tokens[i], postingCount);
    }
    printf("\n");
    
    tokenizer_free_tokens(queryTokens);
    
    double startTime = get_time_ms();
    
    SearchResultSet* results = search_engine_search(engine, query, mode, 10);
    
    double endTime = get_time_ms();
    
    if (results->count == 0) {
        printf("No results found.\n\n");
    } else {
        printf("Found %zu results in %.2f ms:\n\n", results->count,
               endTime - startTime);
        
        for (size_t i = 0; i < results->count; i++) {
            uint32_t docId = results->results[i].docId;
            Document* doc = index_get_document(index, docId);
            
            printf("[%zu] Score: %.4f\n", i + 1, results->results[i].score);
            
            if (doc) {
                printf("    DocID: %u\n", doc->docId);
                printf("    Title: %s\n", doc->title);
                
                // Print snippet with keyword highlighting
                if (results->results[i].snippet) {
                    printf("    Snippet: %s\n", results->results[i].snippet);
                } else {
                    // Fallback: print content snippet
                    size_t snippetLen = strlen(doc->content) > 100 ? 100 : strlen(doc->content);
                    printf("    Content: ");
                    for (size_t j = 0; j < snippetLen; j++) {
                        printf("%c", doc->content[j]);
                    }
                    if (strlen(doc->content) > 100) {
                        printf("...");
                    }
                    printf("\n");
                }
            }
            printf("\n");
        }
    }
    
    search_free_results(results);
}

/**
 * Load documents from file
 */
void load_documents(InvertedIndex* index, Tokenizer* tokenizer, uint32_t startDocId) {
    char filepath[256];
    char buffer[10];
    char choice;
    
    printf("\n=== Load Documents from File ===\n");
    printf("File format options:\n");
    printf("1. TSV (Tab-separated: title\\tcontent)\n");
    printf("2. CSV (Comma-separated: title,content)\n");
    printf("3. JSONL (JSON Lines: {\"title\": \"...\", \"content\": \"...\"})\n");
    printf("\nEnter format (1-3): ");
    
    fgets(buffer, sizeof(buffer), stdin);
    choice = buffer[0];
    while (getchar() != '\n');  // Clear input buffer
    
    printf("Enter file path (or press Enter for default): ");
    if (fgets(filepath, sizeof(filepath), stdin)) {
        // Remove trailing newline
        size_t len = strlen(filepath);
        if (len > 0 && filepath[len - 1] == '\n') {
            filepath[len - 1] = '\0';
            len--;
        }
    }
    
    // Use default files if empty
    if (strlen(filepath) == 0) {
        switch (choice) {
            case '1':
                strcpy(filepath, "data/documents.tsv");
                break;
            case '2':
                strcpy(filepath, "data/documents.csv");
                break;
            case '3':
                strcpy(filepath, "data/documents.jsonl");
                break;
            default:
                printf("Invalid choice!\n");
                return;
        }
    }
    
    printf("\nLoading documents from: %s\n", filepath);
    printf("========================================\n");
    
    int result = -1;
    
    switch (choice) {
        case '1':
            result = file_loader_load_documents(filepath, index, tokenizer, startDocId);
            break;
        case '2':
            result = file_loader_load_csv(filepath, index, tokenizer, startDocId);
            break;
        case '3':
            result = file_loader_load_jsonl(filepath, index, tokenizer, startDocId);
            break;
        default:
            printf("Invalid choice!\n");
            return;
    }
    
    printf("========================================\n");
    
    if (result > 0) {
        printf("\n✅ Successfully loaded %d documents\n", result);
        index_print_stats(index);
    } else {
        printf("\n❌ Failed to load documents\n");
        printf("Error: %s\n", file_loader_get_error());
    }
}

/**
 * Main function
 */
int main(void) {
    printf("========================================\n");
    printf("  Cross-platform Search Engine (C)\n");
    printf("  Multilingual Support: English + CJK\n");
    printf("========================================\n\n");
    
    // 初始化 CJK 分词器（如果可用）
    if (cjk_is_available()) {
        cjk_tokenizer_init("zh");  // 默认中文，会自动检测其他语言
        printf("✓ CJK tokenizer enabled (ICU)\n");
    } else {
        printf("⚠ CJK tokenizer disabled (ICU not available)\n");
        printf("  Only English tokenization will work\n");
    }
    printf("\n");
    
    // Create data structures
    Trie* dictionary = trie_create();
    InvertedIndex* index = index_create();
    Tokenizer* tokenizer = tokenizer_create(dictionary);
    SearchEngine* engine = search_engine_create(index, tokenizer);
    
    // Initialize with sample data
    initialize_search_engine(index, tokenizer);
    
    // Main loop
    int choice = 0;
    while (1) {
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            // Clear input buffer
            while (getchar() != '\n');
            printf("Invalid input!\n");
            continue;
        }
        
        // Clear input buffer
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                perform_search(engine, index, SEARCH_AND);
                break;
            
            case 2:
                perform_search(engine, index, SEARCH_OR);
                break;
            
            case 3:
                perform_search(engine, index, SEARCH_BM25);
                break;
            
            case 4:
                perform_search(engine, index, SEARCH_PHRASE);
                break;
            
            case 5:
                load_documents(index, tokenizer, (uint32_t)index->totalDocs);
                break;
            
            case 6:
                index_print_stats(index);
                break;
            
            case 7:
                trie_print(dictionary);
                break;
            
            case 8:
                printf("Exiting...\n");
                goto cleanup;
            
            default:
                printf("Invalid choice!\n");
        }
    }
    
cleanup:
    // Cleanup
    cjk_tokenizer_cleanup();  // 清理 CJK 分词器资源
    search_engine_destroy(engine);
    index_destroy(index);
    tokenizer_destroy(tokenizer);
    trie_destroy(dictionary);
    
    printf("Goodbye!\n");
    return 0;
}
