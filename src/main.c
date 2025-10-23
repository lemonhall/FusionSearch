#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trie.h"
#include "tokenizer.h"
#include "index.h"
#include "search.h"
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
    printf("3. Perform PHRASE search (exact phrase match)\n");
    printf("4. View index statistics\n");
    printf("5. View dictionary contents\n");
    printf("6. Exit\n");
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
    printf("========================================\n\n");
    
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
                
                // Print snippet (first 100 characters)
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
            printf("\n");
        }
    }
    
    search_free_results(results);
}

/**
 * Main function
 */
int main(void) {
    printf("========================================\n");
    printf("  Cross-platform Search Engine (C)\n");
    printf("  English Version\n");
    printf("========================================\n\n");
    
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
                perform_search(engine, index, SEARCH_PHRASE);
                break;
            
            case 4:
                index_print_stats(index);
                break;
            
            case 5:
                trie_print(dictionary);
                break;
            
            case 6:
                printf("Exiting...\n");
                goto cleanup;
            
            default:
                printf("Invalid choice!\n");
        }
    }
    
cleanup:
    // Cleanup
    search_engine_destroy(engine);
    index_destroy(index);
    tokenizer_destroy(tokenizer);
    trie_destroy(dictionary);
    
    printf("Goodbye!\n");
    return 0;
}
