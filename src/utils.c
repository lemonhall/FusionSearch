#define _POSIX_C_SOURCE 199309L
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

// ============= String utilities =============

char* string_dup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* dup = (char*)malloc(len + 1);
    if (dup) {
        strcpy(dup, str);
    }
    return dup;
}

char* string_tolower(const char* str) {
    if (!str) return NULL;
    
    char* lower = string_dup(str);
    if (lower) {
        for (int i = 0; lower[i]; i++) {
            lower[i] = tolower((unsigned char)lower[i]);
        }
    }
    return lower;
}

char* string_trim(const char* str) {
    if (!str) return NULL;
    
    // Skip leading whitespace
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return string_dup("");
    }
    
    // Find end of string
    const char* end = str + strlen(str) - 1;
    
    // Skip trailing whitespace
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    // Create trimmed string
    size_t len = end - str + 1;
    char* trimmed = (char*)malloc(len + 1);
    if (trimmed) {
        strncpy(trimmed, str, len);
        trimmed[len] = '\0';
    }
    return trimmed;
}

int string_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int string_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return 0;
    
    size_t strLen = strlen(str);
    size_t suffixLen = strlen(suffix);
    
    if (suffixLen > strLen) return 0;
    
    return strcmp(str + strLen - suffixLen, suffix) == 0;
}

char** string_split(const char* str, const char* delimiter, size_t* count) {
    if (!str || !delimiter || !count) return NULL;
    
    *count = 0;
    
    // Create a working copy
    char* workingStr = string_dup(str);
    char** parts = (char**)malloc(sizeof(char*) * 1000);  // Max 1000 parts
    
    char* token = strtok(workingStr, delimiter);
    while (token && *count < 999) {
        parts[*count] = string_dup(token);
        (*count)++;
        token = strtok(NULL, delimiter);
    }
    
    free(workingStr);
    return parts;
}

void string_split_free(char** parts, size_t count) {
    if (!parts) return;
    
    for (size_t i = 0; i < count; i++) {
        if (parts[i]) {
            free(parts[i]);
        }
    }
    free(parts);
}

// ============= File utilities =============

char* file_read(const char* filename, size_t* fileSize) {
    if (!filename || !fileSize) return NULL;
    
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    
    // Get file size
    fseek(f, 0, SEEK_END);
    *fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Read file
    char* content = (char*)malloc(*fileSize + 1);
    size_t bytesRead = fread(content, 1, *fileSize, f);
    fclose(f);
    
    if (bytesRead != *fileSize) {
        free(content);
        return NULL;
    }
    
    content[*fileSize] = '\0';
    return content;
}

int file_write(const char* filename, const char* data, size_t size) {
    if (!filename || !data) return -1;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;
    
    size_t bytesWritten = fwrite(data, 1, size, f);
    fclose(f);
    
    return bytesWritten == size ? 0 : -1;
}

int file_load_dictionary(const char* filename, char*** words,
                        uint32_t** frequencies, size_t* count) {
    // TODO: Implement dictionary loading from file
    return -1;
}

int file_load_documents(const char* filename, char*** titles,
                       char*** contents, size_t* count) {
    // TODO: Implement document loading from file
    return -1;
}

// ============= Memory utilities =============

void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Error: malloc failed for size %zu\n", size);
        exit(1);
    }
    return ptr;
}

void* safe_realloc(void* ptr, size_t size) {
    void* newPtr = realloc(ptr, size);
    if (!newPtr && size > 0) {
        fprintf(stderr, "Error: realloc failed for size %zu\n", size);
        exit(1);
    }
    return newPtr;
}

// ============= Timing utilities =============

double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}
