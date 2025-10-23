#include "file_loader.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 10000
#define MAX_ERROR_LENGTH 256

static char g_error_message[MAX_ERROR_LENGTH] = "";

/**
 * Set error message
 */
static void set_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_error_message, MAX_ERROR_LENGTH - 1, fmt, args);
    va_end(args);
}

/**
 * Trim whitespace from both ends of string
 */
static void trim_string(char* str) {
    if (!str) return;
    
    // Trim leading whitespace
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    
    // Trim trailing whitespace
    int end = (int)strlen(str) - 1;
    while (end >= start && isspace((unsigned char)str[end])) {
        end--;
    }
    
    // Shift string
    if (start > 0) {
        int i = 0;
        while (start <= end) {
            str[i++] = str[start++];
        }
        str[i] = '\0';
    } else if (end < (int)strlen(str) - 1) {
        str[end + 1] = '\0';
    }
}

/**
 * Parse CSV field (handle quotes)
 */
static char* parse_csv_field(char* line, size_t* pos) {
    static char field[5000];
    size_t field_len = 0;
    
    if (!line || *pos >= strlen(line)) return NULL;
    
    // Skip leading spaces
    while (line[*pos] && isspace((unsigned char)line[*pos])) {
        (*pos)++;
    }
    
    // Check if field is quoted
    int quoted = 0;
    if (line[*pos] == '"') {
        quoted = 1;
        (*pos)++;  // Skip opening quote
    }
    
    // Read field
    while (line[*pos] && field_len < sizeof(field) - 1) {
        if (quoted) {
            if (line[*pos] == '"') {
                // Check for escaped quote
                if (line[*pos + 1] == '"') {
                    field[field_len++] = '"';
                    (*pos) += 2;
                } else {
                    // End of quoted field
                    (*pos)++;
                    break;
                }
            } else {
                field[field_len++] = line[*pos];
                (*pos)++;
            }
        } else {
            if (line[*pos] == ',') {
                break;
            }
            field[field_len++] = line[*pos];
            (*pos)++;
        }
    }
    
    field[field_len] = '\0';
    trim_string(field);
    
    // Skip comma
    while (line[*pos] && isspace((unsigned char)line[*pos])) {
        (*pos)++;
    }
    if (line[*pos] == ',') {
        (*pos)++;
    }
    
    return field;
}

/**
 * Load documents from TSV file (tab-separated)
 */
int file_loader_load_documents(const char* filename, InvertedIndex* index,
                               Tokenizer* tokenizer, uint32_t startDocId) {
    if (!filename || !index || !tokenizer) {
        set_error("Invalid parameters");
        return -1;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        set_error("Cannot open file: %s", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int doc_count = 0;
    uint32_t doc_id = startDocId;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') {
            continue;
        }
        
        // Split by tab: title\tcontent
        char* tab_pos = strchr(line, '\t');
        if (!tab_pos) {
            // If no tab, treat entire line as title with empty content
            tab_pos = line + len;
        }
        
        // Extract title
        size_t title_len = tab_pos - line;
        char* title = (char*)safe_malloc(title_len + 1);
        strncpy(title, line, title_len);
        title[title_len] = '\0';
        trim_string(title);
        
        // Extract content
        char* content = "";
        if (*tab_pos == '\t') {
            tab_pos++;
            content = tab_pos;
        }
        char* content_dup = string_dup(content);
        
        // Tokenize content
        TokenList* tokens = tokenizer_tokenize(tokenizer, content);
        
        // Add to index
        if (tokens && tokens->count > 0) {
            index_add_document(index, doc_id, title, content,
                             (const char**)tokens->tokens, tokens->count);
            doc_count++;
            doc_id++;
            
            printf("✓ [%d] %s\n", doc_count, title);
        }
        
        tokenizer_free_tokens(tokens);
        free(title);
        free(content_dup);
    }
    
    fclose(file);
    
    if (doc_count == 0) {
        set_error("No documents loaded from file");
    }
    
    return doc_count;
}

/**
 * Load documents from CSV file
 */
int file_loader_load_csv(const char* filename, InvertedIndex* index,
                         Tokenizer* tokenizer, uint32_t startDocId) {
    if (!filename || !index || !tokenizer) {
        set_error("Invalid parameters");
        return -1;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        set_error("Cannot open file: %s", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int doc_count = 0;
    uint32_t doc_id = startDocId;
    int first_line = 1;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') {
            continue;
        }
        
        // Skip header line if it exists
        if (first_line && (strstr(line, "title") || strstr(line, "Title"))) {
            first_line = 0;
            continue;
        }
        first_line = 0;
        
        // Parse CSV fields
        size_t pos = 0;
        char* title = parse_csv_field(line, &pos);
        char* content = parse_csv_field(line, &pos);
        
        if (!title || !content) {
            continue;
        }
        
        char* title_dup = string_dup(title);
        char* content_dup = string_dup(content);
        
        // Tokenize content
        TokenList* tokens = tokenizer_tokenize(tokenizer, content_dup);
        
        // Add to index
        if (tokens && tokens->count > 0) {
            index_add_document(index, doc_id, title_dup, content_dup,
                             (const char**)tokens->tokens, tokens->count);
            doc_count++;
            doc_id++;
            
            printf("✓ [%d] %s\n", doc_count, title_dup);
        }
        
        tokenizer_free_tokens(tokens);
        free(title_dup);
        free(content_dup);
    }
    
    fclose(file);
    
    if (doc_count == 0) {
        set_error("No documents loaded from file");
    }
    
    return doc_count;
}

/**
 * Simple JSON field parser
 */
static char* parse_json_field(const char* line, const char* field_name) {
    static char value[5000];
    size_t value_len = 0;
    
    // Find field: "field_name": "value"
    char pattern[100];
    snprintf(pattern, sizeof(pattern), "\"%s\": \"", field_name);
    
    const char* start = strstr(line, pattern);
    if (!start) {
        return NULL;
    }
    
    start += strlen(pattern);
    
    // Extract value until closing quote
    while (*start && *start != '"' && value_len < sizeof(value) - 1) {
        if (*start == '\\' && *(start + 1)) {
            // Handle escape sequences
            start++;
            if (*start == 'n') {
                value[value_len++] = '\n';
            } else if (*start == 't') {
                value[value_len++] = '\t';
            } else if (*start == '"') {
                value[value_len++] = '"';
            } else {
                value[value_len++] = *start;
            }
        } else {
            value[value_len++] = *start;
        }
        start++;
    }
    
    value[value_len] = '\0';
    return value;
}

/**
 * Load documents from JSON Lines file
 */
int file_loader_load_jsonl(const char* filename, InvertedIndex* index,
                           Tokenizer* tokenizer, uint32_t startDocId) {
    if (!filename || !index || !tokenizer) {
        set_error("Invalid parameters");
        return -1;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        set_error("Cannot open file: %s", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int doc_count = 0;
    uint32_t doc_id = startDocId;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] != '{') {
            continue;
        }
        
        // Parse JSON fields
        char* title = parse_json_field(line, "title");
        char* content = parse_json_field(line, "content");
        
        if (!title || !content) {
            continue;
        }
        
        char* title_dup = string_dup(title);
        char* content_dup = string_dup(content);
        
        // Tokenize content
        TokenList* tokens = tokenizer_tokenize(tokenizer, content_dup);
        
        // Add to index
        if (tokens && tokens->count > 0) {
            index_add_document(index, doc_id, title_dup, content_dup,
                             (const char**)tokens->tokens, tokens->count);
            doc_count++;
            doc_id++;
            
            printf("✓ [%d] %s\n", doc_count, title_dup);
        }
        
        tokenizer_free_tokens(tokens);
        free(title_dup);
        free(content_dup);
    }
    
    fclose(file);
    
    if (doc_count == 0) {
        set_error("No documents loaded from file");
    }
    
    return doc_count;
}

/**
 * Get error message
 */
const char* file_loader_get_error(void) {
    return g_error_message;
}
