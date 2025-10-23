#include "tokenizer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool tokenizer_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool tokenizer_is_punctuation(char c) {
    return c == '.' || c == ',' || c == '!' || c == '?' || 
           c == ';' || c == ':' || c == '\'' || c == '"' ||
           c == '(' || c == ')' || c == '-' || c == '/';
}

void tokenizer_lowercase(char* text) {
    if (!text) return;
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = tolower((unsigned char)text[i]);
    }
}

Tokenizer* tokenizer_create(Trie* trie) {
    Tokenizer* tokenizer = (Tokenizer*)safe_malloc(sizeof(Tokenizer));
    tokenizer->dictionary = trie;
    return tokenizer;
}

void tokenizer_destroy(Tokenizer* tokenizer) {
    if (tokenizer) {
        free(tokenizer);
    }
}

TokenList* tokenizer_tokenize(Tokenizer* tokenizer, const char* text) {
    if (!tokenizer || !text) return NULL;
    
    TokenList* tokens = (TokenList*)safe_malloc(sizeof(TokenList));
    tokens->count = 0;
    
    // Create a working copy we can modify
    char* workingText = string_dup(text);
    tokenizer_lowercase(workingText);
    
    char* token = (char*)safe_malloc(MAX_TOKEN_LENGTH);
    int tokenLen = 0;
    
    for (size_t i = 0; workingText[i] != '\0'; i++) {
        char c = workingText[i];
        
        if (tokenizer_is_whitespace(c) || tokenizer_is_punctuation(c)) {
            if (tokenLen > 0) {
                token[tokenLen] = '\0';
                
                // Only add non-empty tokens
                if (tokenLen > 0) {
                    tokens->tokens[tokens->count] = string_dup(token);
                    tokens->count++;
                    
                    if (tokens->count >= MAX_TOKEN_COUNT) {
                        break;
                    }
                }
                tokenLen = 0;
            }
        } else {
            if (tokenLen < MAX_TOKEN_LENGTH - 1) {
                token[tokenLen++] = c;
            }
        }
    }
    
    // Add last token if any
    if (tokenLen > 0 && tokens->count < MAX_TOKEN_COUNT) {
        token[tokenLen] = '\0';
        tokens->tokens[tokens->count] = string_dup(token);
        tokens->count++;
    }
    
    free(token);
    free(workingText);
    
    return tokens;
}

void tokenizer_free_tokens(TokenList* tokens) {
    if (!tokens) return;
    
    for (size_t i = 0; i < tokens->count; i++) {
        if (tokens->tokens[i]) {
            free(tokens->tokens[i]);
        }
    }
    free(tokens);
}
