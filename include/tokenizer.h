#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include "trie.h"

#define MAX_TOKEN_COUNT 10000
#define MAX_TOKEN_LENGTH 256

typedef struct {
    char* tokens[MAX_TOKEN_COUNT];
    size_t count;
} TokenList;

typedef struct {
    Trie* dictionary;
} Tokenizer;

/**
 * Create a tokenizer with a dictionary
 * @param trie: Trie containing dictionary
 */
Tokenizer* tokenizer_create(Trie* trie);

/**
 * Destroy the tokenizer
 */
void tokenizer_destroy(Tokenizer* tokenizer);

/**
 * Tokenize text into words
 * For English, this is simple whitespace/punctuation splitting
 * @param tokenizer: Tokenizer instance
 * @param text: Input text
 * @return: TokenList containing all tokens
 */
TokenList* tokenizer_tokenize(Tokenizer* tokenizer, const char* text);

/**
 * Free token list memory
 */
void tokenizer_free_tokens(TokenList* tokens);

/**
 * Convert text to lowercase (for case-insensitive search)
 */
void tokenizer_lowercase(char* text);

/**
 * Check if character is whitespace
 */
bool tokenizer_is_whitespace(char c);

/**
 * Check if character is punctuation
 */
bool tokenizer_is_punctuation(char c);

#endif // TOKENIZER_H
