#ifndef SNIPPET_H
#define SNIPPET_H

#include <stddef.h>

/**
 * Generate a snippet with keyword highlighting
 * 
 * @param content Original document content
 * @param keywords Array of keywords to highlight
 * @param keywordCount Number of keywords
 * @param snippetLength Maximum snippet length (e.g., 150 chars)
 * @param highlightMarker Marker to wrap highlighted text (e.g., ">>")
 * @return Allocated snippet string with highlighting, must be freed
 */
char* snippet_generate(const char* content, const char** keywords,
                      size_t keywordCount, size_t snippetLength,
                      const char* highlightMarker);

/**
 * Free a snippet allocated by snippet_generate
 */
void snippet_free(char* snippet);

#endif // SNIPPET_H
