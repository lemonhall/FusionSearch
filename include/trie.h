#ifndef TRIE_H
#define TRIE_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_CHILDREN 256  // For ASCII characters

typedef struct TrieNode {
    struct TrieNode* children[MAX_CHILDREN];
    bool isWord;
    uint32_t frequency;  // Word frequency for ranking
} TrieNode;

typedef struct {
    TrieNode* root;
    uint32_t wordCount;
} Trie;

/**
 * Create a new Trie structure
 */
Trie* trie_create(void);

/**
 * Destroy the Trie and free all memory
 */
void trie_destroy(Trie* trie);

/**
 * Insert a word into the Trie
 * @param trie: Trie structure
 * @param word: Word to insert
 * @param frequency: Word frequency (default 1 if 0)
 */
void trie_insert(Trie* trie, const char* word, uint32_t frequency);

/**
 * Search for a word in the Trie
 * @param trie: Trie structure
 * @param word: Word to search
 * @return: true if found, false otherwise
 */
bool trie_search(Trie* trie, const char* word);

/**
 * Get the frequency of a word
 * @param trie: Trie structure
 * @param word: Word to query
 * @return: Word frequency (0 if not found)
 */
uint32_t trie_get_frequency(Trie* trie, const char* word);

/**
 * Get the word count in Trie
 */
uint32_t trie_word_count(Trie* trie);

/**
 * Print all words in Trie (for debugging)
 */
void trie_print(Trie* trie);

#endif // TRIE_H
