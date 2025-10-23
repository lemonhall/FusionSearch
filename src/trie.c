#include "trie.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Create a new Trie node
 */
static TrieNode* trie_node_create(void) {
    TrieNode* node = (TrieNode*)safe_malloc(sizeof(TrieNode));
    memset(node->children, 0, sizeof(node->children));
    node->isWord = false;
    node->frequency = 0;
    return node;
}

/**
 * Recursively destroy Trie
 */
static void trie_node_destroy(TrieNode* node) {
    if (!node) return;
    
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (node->children[i]) {
            trie_node_destroy(node->children[i]);
        }
    }
    free(node);
}

Trie* trie_create(void) {
    Trie* trie = (Trie*)safe_malloc(sizeof(Trie));
    trie->root = trie_node_create();
    trie->wordCount = 0;
    return trie;
}

void trie_destroy(Trie* trie) {
    if (!trie) return;
    trie_node_destroy(trie->root);
    free(trie);
}

void trie_insert(Trie* trie, const char* word, uint32_t frequency) {
    if (!trie || !word) return;
    
    TrieNode* node = trie->root;
    
    for (int i = 0; word[i] != '\0'; i++) {
        unsigned char c = (unsigned char)word[i];
        
        if (!node->children[c]) {
            node->children[c] = trie_node_create();
        }
        node = node->children[c];
    }
    
    if (!node->isWord) {
        node->isWord = true;
        trie->wordCount++;
    }
    
    if (frequency > 0) {
        node->frequency = frequency;
    } else if (node->frequency == 0) {
        node->frequency = 1;
    }
}

bool trie_search(Trie* trie, const char* word) {
    if (!trie || !word) return false;
    
    TrieNode* node = trie->root;
    
    for (int i = 0; word[i] != '\0'; i++) {
        unsigned char c = (unsigned char)word[i];
        
        if (!node->children[c]) {
            return false;
        }
        node = node->children[c];
    }
    
    return node->isWord;
}

uint32_t trie_get_frequency(Trie* trie, const char* word) {
    if (!trie || !word) return 0;
    
    TrieNode* node = trie->root;
    
    for (int i = 0; word[i] != '\0'; i++) {
        unsigned char c = (unsigned char)word[i];
        
        if (!node->children[c]) {
            return 0;
        }
        node = node->children[c];
    }
    
    return node->isWord ? node->frequency : 0;
}

uint32_t trie_word_count(Trie* trie) {
    return trie ? trie->wordCount : 0;
}

/**
 * Recursively print all words in Trie
 */
static void trie_print_recursive(TrieNode* node, char* prefix, int depth) {
    if (!node) return;
    
    if (node->isWord) {
        printf("  %s (freq: %u)\n", prefix, node->frequency);
    }
    
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (node->children[i]) {
            prefix[depth] = (char)i;
            prefix[depth + 1] = '\0';
            trie_print_recursive(node->children[i], prefix, depth + 1);
        }
    }
}

void trie_print(Trie* trie) {
    if (!trie) return;
    
    printf("\n=== Trie Contents (Total words: %u) ===\n", trie->wordCount);
    
    char prefix[256] = {0};
    trie_print_recursive(trie->root, prefix, 0);
    
    printf("=====================================\n\n");
}
