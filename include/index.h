#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>

#define MAX_DOCS 10000
#define MAX_WORDS 100000
#define MAX_DOCS_PER_WORD 1000

typedef struct {
    uint32_t docId;
    uint32_t frequency;  // TF (term frequency)
    float score;         // For BM25 or TF-IDF
} PostingEntry;

typedef struct {
    char* word;
    PostingEntry* postings[MAX_DOCS_PER_WORD];
    size_t postingCount;
} InvertedIndexEntry;

typedef struct {
    uint32_t docId;
    char* title;
    char* content;
    size_t wordCount;
} Document;

typedef struct {
    Document* docs;
    uint32_t docCount;
} DocumentStore;

typedef struct {
    InvertedIndexEntry** entries;
    size_t entryCount;
    uint32_t totalDocs;
    DocumentStore* docStore;  // Store for document metadata
} InvertedIndex;

/**
 * Create an inverted index
 */
InvertedIndex* index_create(void);

/**
 * Destroy the inverted index
 */
void index_destroy(InvertedIndex* index);

/**
 * Add a document and build its index
 * @param index: InvertedIndex instance
 * @param docId: Document ID
 * @param title: Document title
 * @param content: Document content
 * @param tokens: Tokenized words
 */
void index_add_document(InvertedIndex* index, uint32_t docId, 
                       const char* title, const char* content,
                       const char** tokens, size_t tokenCount);

/**
 * Search for documents containing a query term
 * @param index: InvertedIndex instance
 * @param term: Search term
 * @param results: Output array for document IDs
 * @param maxResults: Maximum results to return
 * @return: Number of results found
 */
size_t index_search_term(InvertedIndex* index, const char* term,
                        uint32_t* results, size_t maxResults);

/**
 * Get document by ID
 */
Document* index_get_document(InvertedIndex* index, uint32_t docId);

/**
 * Get posting list for a word
 */
PostingEntry** index_get_postings(InvertedIndex* index, const char* word, 
                                  size_t* count);

/**
 * Print index statistics (for debugging)
 */
void index_print_stats(InvertedIndex* index);

#endif // INDEX_H
