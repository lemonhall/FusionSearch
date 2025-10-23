#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include <stddef.h>
#include "index.h"
#include "tokenizer.h"
#include "vector_index.h"

/**
 * Load documents from a text file
 * File format: one document per line, format: "title\tcontent"
 * 
 * @param filename Path to the document file
 * @param index InvertedIndex to add documents to
 * @param tokenizer Tokenizer for processing documents
 * @param startDocId Starting document ID
 * @return Number of documents loaded, or -1 on error
 */
int file_loader_load_documents(const char* filename, InvertedIndex* index,
                               Tokenizer* tokenizer, uint32_t startDocId);

/**
 * Load a CSV file with documents
 * File format: title,content (comma-separated, with optional quotes)
 * 
 * @param filename Path to the CSV file
 * @param index InvertedIndex to add documents to
 * @param tokenizer Tokenizer for processing documents
 * @param startDocId Starting document ID
 * @return Number of documents loaded, or -1 on error
 */
int file_loader_load_csv(const char* filename, InvertedIndex* index,
                         Tokenizer* tokenizer, uint32_t startDocId);

/**
 * Load documents from a JSON Lines file (.jsonl)
 * File format: one JSON object per line
 * JSON format: {"title": "...", "content": "..."}
 * 
 * @param filename Path to the JSON Lines file
 * @param index InvertedIndex to add documents to
 * @param tokenizer Tokenizer for processing documents
 * @param startDocId Starting document ID
 * @return Number of documents loaded, or -1 on error
 */
int file_loader_load_jsonl(const char* filename, InvertedIndex* index,
                           Tokenizer* tokenizer, uint32_t startDocId);

/**
 * Load documents from a JSON Lines file with vector embeddings
 * File format: one JSON object per line
 * JSON format: {"title": "...", "content": "...", "embedding": [0.1, 0.2, ...]}
 * 
 * This function loads both text documents (for BM25) and vector embeddings (for semantic search)
 * 
 * @param filename Path to the JSON Lines file
 * @param index InvertedIndex to add documents to (for BM25)
 * @param vector_index VectorIndex to add embeddings to (can be NULL if not needed)
 * @param tokenizer Tokenizer for processing documents
 * @param startDocId Starting document ID
 * @return Number of documents loaded, or -1 on error
 */
int file_loader_load_jsonl_with_vectors(const char* filename, 
                                        InvertedIndex* index,
                                        VectorIndex* vector_index,
                                        Tokenizer* tokenizer, 
                                        uint32_t startDocId);

/**
 * Get error message for the last operation
 * 
 * @return Error message string
 */
const char* file_loader_get_error(void);

#endif // FILE_LOADER_H
