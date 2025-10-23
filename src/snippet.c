#include "snippet.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Case-insensitive substring search
 */
static const char* find_keyword_position(const char* text, const char* keyword) {
    if (!text || !keyword) return NULL;
    
    size_t text_len = strlen(text);
    size_t keyword_len = strlen(keyword);
    
    if (keyword_len > text_len) return NULL;
    
    for (size_t i = 0; i <= text_len - keyword_len; i++) {
        int match = 1;
        for (size_t j = 0; j < keyword_len; j++) {
            if (tolower(text[i + j]) != tolower(keyword[j])) {
                match = 0;
                break;
            }
        }
        if (match) {
            return &text[i];
        }
    }
    
    return NULL;
}

/**
 * Convert string to lowercase for comparison
 */
static void to_lowercase(char* str) {
    if (!str) return;
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

/**
 * Check if character is word boundary
 */
static int is_word_boundary(char c) {
    return isspace((unsigned char)c) || ispunct((unsigned char)c);
}

/**
 * Generate snippet with keyword highlighting
 */
char* snippet_generate(const char* content, const char** keywords,
                      size_t keywordCount, size_t snippetLength,
                      const char* highlightMarker) {
    if (!content || !keywords || keywordCount == 0) {
        return string_dup(content ? content : "");
    }
    
    size_t content_len = strlen(content);
    if (content_len == 0) {
        return string_dup("");
    }
    
    // Find first keyword occurrence
    const char* first_match = NULL;
    size_t first_pos = content_len;
    
    for (size_t i = 0; i < keywordCount; i++) {
        const char* match_pos = find_keyword_position(content, keywords[i]);
        if (match_pos) {
            size_t pos = match_pos - content;
            if (pos < first_pos) {
                first_pos = pos;
                first_match = match_pos;
            }
        }
    }
    
    // If no keyword found, return beginning of content
    if (!first_match) {
        size_t truncate_len = content_len < snippetLength ? content_len : snippetLength;
        char* snippet = (char*)safe_malloc(truncate_len + 10);  // +10 for ellipsis
        
        strncpy(snippet, content, truncate_len);
        snippet[truncate_len] = '\0';
        
        if (content_len > snippetLength) {
            strcat(snippet, "...");
        }
        
        return snippet;
    }
    
    // Calculate snippet boundaries around first match
    // Try to include text before and after the keyword
    size_t context_before = 50;  // Characters before keyword
    size_t context_after = 50;   // Characters after keyword
    
    size_t snippet_start = first_pos > context_before ? first_pos - context_before : 0;
    size_t snippet_end = first_pos + context_after;
    
    if (snippet_end > content_len) {
        snippet_end = content_len;
    }
    
    // Expand to snippet length if needed
    size_t current_length = snippet_end - snippet_start;
    if (current_length < snippetLength && snippet_end < content_len) {
        size_t expand = snippetLength - current_length;
        if (snippet_end + expand <= content_len) {
            snippet_end += expand;
        }
    }
    
    // Build snippet with highlighting
    size_t buffer_size = snippetLength + keywordCount * strlen(highlightMarker) * 2 + 50;
    char* snippet = (char*)safe_malloc(buffer_size);
    char* pos = snippet;
    
    // Add prefix ellipsis
    if (snippet_start > 0) {
        strcpy(pos, "...");
        pos += 3;
    }
    
    // Copy and highlight content
    for (size_t i = snippet_start; i < snippet_end && pos - snippet < (int)buffer_size - 100; i++) {
        // Check if current position matches any keyword
        int keyword_match_len = 0;
        int matched_keyword_idx = -1;
        
        for (size_t k = 0; k < keywordCount; k++) {
            size_t kw_len = strlen(keywords[k]);
            if (i + kw_len <= snippet_end) {
                // Case-insensitive comparison
                int match = 1;
                for (size_t j = 0; j < kw_len; j++) {
                    if (tolower((unsigned char)content[i + j]) != tolower((unsigned char)keywords[k][j])) {
                        match = 0;
                        break;
                    }
                }
                
                if (match) {
                    keyword_match_len = (int)kw_len;
                    matched_keyword_idx = (int)k;
                    break;
                }
            }
        }
        
        if (keyword_match_len > 0) {
            // Add highlighting marker
            strcpy(pos, highlightMarker);
            pos += strlen(highlightMarker);
            
            // Copy keyword
            for (int j = 0; j < keyword_match_len; j++) {
                *pos++ = content[i + j];
            }
            
            // Add closing marker
            strcpy(pos, highlightMarker);
            pos += strlen(highlightMarker);
            
            i += keyword_match_len - 1;  // Skip over the keyword
        } else {
            *pos++ = content[i];
        }
    }
    
    // Add suffix ellipsis
    if (snippet_end < content_len) {
        strcpy(pos, "...");
        pos += 3;
    }
    
    *pos = '\0';
    
    return snippet;
}

/**
 * Free snippet
 */
void snippet_free(char* snippet) {
    if (snippet) {
        free(snippet);
    }
}
