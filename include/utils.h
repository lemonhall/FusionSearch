#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/**
 * String utilities
 */

/**
 * Duplicate a string
 */
char* string_dup(const char* str);

/**
 * Convert string to lowercase
 */
char* string_tolower(const char* str);

/**
 * Trim whitespace from start and end
 */
char* string_trim(const char* str);

/**
 * Check if string starts with prefix
 */
int string_starts_with(const char* str, const char* prefix);

/**
 * Check if string ends with suffix
 */
int string_ends_with(const char* str, const char* suffix);

/**
 * Split string by delimiter
 */
char** string_split(const char* str, const char* delimiter, size_t* count);

/**
 * Free split result
 */
void string_split_free(char** parts, size_t count);

/**
 * File utilities
 */

/**
 * Read entire file into memory
 */
char* file_read(const char* filename, size_t* fileSize);

/**
 * Write data to file
 */
int file_write(const char* filename, const char* data, size_t size);

/**
 * Load dictionary from file (word frequency pairs)
 */
int file_load_dictionary(const char* filename, char*** words, 
                        uint32_t** frequencies, size_t* count);

/**
 * Load documents from file
 */
int file_load_documents(const char* filename, char*** titles, 
                       char*** contents, size_t* count);

/**
 * Memory utilities
 */

/**
 * Safe malloc with error checking
 */
void* safe_malloc(size_t size);

/**
 * Safe realloc with error checking
 */
void* safe_realloc(void* ptr, size_t size);

/**
 * Timing utilities
 */

/**
 * Get current time in milliseconds
 */
double get_time_ms(void);

#endif // UTILS_H
