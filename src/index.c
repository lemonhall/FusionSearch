#include "index.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

InvertedIndex* index_create(void) {
    InvertedIndex* index = (InvertedIndex*)safe_malloc(sizeof(InvertedIndex));
    index->entries = (InvertedIndexEntry**)safe_malloc(sizeof(InvertedIndexEntry*) * MAX_WORDS);
    index->entryCount = 0;
    index->totalDocs = 0;
    memset(index->entries, 0, sizeof(InvertedIndexEntry*) * MAX_WORDS);
    
    // Create document store
    index->docStore = (DocumentStore*)safe_malloc(sizeof(DocumentStore));
    index->docStore->docs = (Document*)safe_malloc(sizeof(Document) * MAX_DOCS);
    index->docStore->docCount = 0;
    memset(index->docStore->docs, 0, sizeof(Document) * MAX_DOCS);
    
    return index;
}

void index_destroy(InvertedIndex* index) {
    if (!index) return;
    
    // Destroy entries
    for (size_t i = 0; i < index->entryCount; i++) {
        if (index->entries[i]) {
            if (index->entries[i]->word) {
                free(index->entries[i]->word);
            }
            for (size_t j = 0; j < index->entries[i]->postingCount; j++) {
                if (index->entries[i]->postings[j]) {
                    free(index->entries[i]->postings[j]);
                }
            }
            free(index->entries[i]);
        }
    }
    
    free(index->entries);
    
    // Destroy document store
    if (index->docStore) {
        for (size_t i = 0; i < index->docStore->docCount; i++) {
            if (index->docStore->docs[i].title) {
                free(index->docStore->docs[i].title);
            }
            if (index->docStore->docs[i].content) {
                free(index->docStore->docs[i].content);
            }
        }
        free(index->docStore->docs);
        free(index->docStore);
    }
    
    free(index);
}

/**
 * Find or create an index entry for a word
 */
static InvertedIndexEntry* index_find_or_create_entry(InvertedIndex* index, 
                                                      const char* word) {
    if (!index || !word) return NULL;
    
    // Search for existing entry
    for (size_t i = 0; i < index->entryCount; i++) {
        if (strcmp(index->entries[i]->word, word) == 0) {
            return index->entries[i];
        }
    }
    
    // Create new entry if not found
    if (index->entryCount >= MAX_WORDS) {
        return NULL;  // Index is full
    }
    
    InvertedIndexEntry* entry = 
        (InvertedIndexEntry*)safe_malloc(sizeof(InvertedIndexEntry));
    entry->word = string_dup(word);
    entry->postingCount = 0;
    memset(entry->postings, 0, sizeof(entry->postings));
    
    index->entries[index->entryCount] = entry;
    index->entryCount++;
    
    return entry;
}

/**
 * Add a posting to an entry
 */
static void index_add_posting(InvertedIndexEntry* entry, uint32_t docId, 
                             uint32_t frequency) {
    if (!entry || entry->postingCount >= MAX_DOCS_PER_WORD) return;
    
    // Check if docId already in postings
    for (size_t i = 0; i < entry->postingCount; i++) {
        if (entry->postings[i]->docId == docId) {
            entry->postings[i]->frequency += frequency;
            return;
        }
    }
    
    // Create new posting
    PostingEntry* posting = (PostingEntry*)safe_malloc(sizeof(PostingEntry));
    posting->docId = docId;
    posting->frequency = frequency;
    posting->score = 0.0f;
    
    entry->postings[entry->postingCount] = posting;
    entry->postingCount++;
}

void index_add_document(InvertedIndex* index, uint32_t docId,
                       const char* title, const char* content,
                       const char** tokens, size_t tokenCount) {
    if (!index || !tokens) return;
    
    index->totalDocs++;
    
    // Store document metadata
    if (index->docStore->docCount < MAX_DOCS) {
        Document* doc = &index->docStore->docs[index->docStore->docCount];
        doc->docId = docId;
        doc->title = string_dup(title);
        doc->content = string_dup(content);
        doc->wordCount = tokenCount;
        index->docStore->docCount++;
    }
    
    // Count term frequencies
    for (size_t i = 0; i < tokenCount; i++) {
        if (!tokens[i]) continue;
        
        InvertedIndexEntry* entry = index_find_or_create_entry(index, tokens[i]);
        if (entry) {
            index_add_posting(entry, docId, 1);
        }
    }
}

size_t index_search_term(InvertedIndex* index, const char* term,
                        uint32_t* results, size_t maxResults) {
    if (!index || !term || !results) return 0;
    
    for (size_t i = 0; i < index->entryCount; i++) {
        if (strcmp(index->entries[i]->word, term) == 0) {
            size_t count = index->entries[i]->postingCount;
            if (count > maxResults) {
                count = maxResults;
            }
            
            for (size_t j = 0; j < count; j++) {
                results[j] = index->entries[i]->postings[j]->docId;
            }
            
            return count;
        }
    }
    
    return 0;
}

Document* index_get_document(InvertedIndex* index, uint32_t docId) {
    if (!index || !index->docStore) return NULL;
    
    for (size_t i = 0; i < index->docStore->docCount; i++) {
        if (index->docStore->docs[i].docId == docId) {
            return &index->docStore->docs[i];
        }
    }
    
    return NULL;
}

PostingEntry** index_get_postings(InvertedIndex* index, const char* word,
                                 size_t* count) {
    if (!index || !word || !count) return NULL;
    
    for (size_t i = 0; i < index->entryCount; i++) {
        if (strcmp(index->entries[i]->word, word) == 0) {
            *count = index->entries[i]->postingCount;
            return index->entries[i]->postings;
        }
    }
    
    *count = 0;
    return NULL;
}

void index_print_stats(InvertedIndex* index) {
    if (!index) return;
    
    printf("\n=== Inverted Index Statistics ===\n");
    printf("Total Documents: %u\n", index->totalDocs);
    printf("Unique Terms: %zu\n", index->entryCount);
    
    size_t totalPostings = 0;
    for (size_t i = 0; i < index->entryCount; i++) {
        totalPostings += index->entries[i]->postingCount;
    }
    
    printf("Total Postings: %zu\n", totalPostings);
    printf("================================\n\n");
}
