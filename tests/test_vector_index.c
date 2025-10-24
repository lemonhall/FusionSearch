/**
 * Unit Tests for Vector Index
 * 
 * Coverage:
 * - Vector addition and storage
 * - Cosine similarity calculation
 * - Top-K search
 * - Memory management
 * - Edge cases (zero vectors, identical vectors)
 */

#include "test.h"
#include "vector_index.h"
#include <math.h>
#include <string.h>

#define EPSILON 0.0001f

void test_vector_index_creation(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Creation");
    
    VectorIndex* index = vector_index_create(128);
    assert_true(suite, "Vector index creation", index != NULL);
    assert_equal_int(suite, "Vector dimension is 128", 128, 
                    (int)vector_index_dimension(index));
    assert_equal_int(suite, "Initial count is 0", 0, 
                    (int)vector_index_count(index));
    
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_index_add(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Add Vectors");
    
    VectorIndex* index = vector_index_create(3);
    
    float vec1[] = {1.0f, 0.0f, 0.0f};
    float vec2[] = {0.0f, 1.0f, 0.0f};
    float vec3[] = {0.0f, 0.0f, 1.0f};
    
    vector_index_add(index, 1, vec1);
    assert_equal_int(suite, "Count after 1st add", 1, 
                    (int)vector_index_count(index));
    
    vector_index_add(index, 2, vec2);
    assert_equal_int(suite, "Count after 2nd add", 2, 
                    (int)vector_index_count(index));
    
    vector_index_add(index, 3, vec3);
    assert_equal_int(suite, "Count after 3rd add", 3, 
                    (int)vector_index_count(index));
    
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_cosine_similarity(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Cosine Similarity");
    
    // Identical vectors (similarity = 1.0)
    float vec1[] = {1.0f, 2.0f, 3.0f};
    float vec2[] = {1.0f, 2.0f, 3.0f};
    float sim = cosine_similarity(vec1, vec2, 3);
    assert_true(suite, "Identical vectors similarity ≈ 1.0", 
               fabs(sim - 1.0f) < EPSILON);
    
    // Orthogonal vectors (similarity = 0.0)
    float vec3[] = {1.0f, 0.0f, 0.0f};
    float vec4[] = {0.0f, 1.0f, 0.0f};
    sim = cosine_similarity(vec3, vec4, 3);
    assert_true(suite, "Orthogonal vectors similarity ≈ 0.0", 
               fabs(sim - 0.0f) < EPSILON);
    
    // Opposite vectors (similarity = -1.0)
    float vec5[] = {1.0f, 2.0f, 3.0f};
    float vec6[] = {-1.0f, -2.0f, -3.0f};
    sim = cosine_similarity(vec5, vec6, 3);
    assert_true(suite, "Opposite vectors similarity ≈ -1.0", 
               fabs(sim - (-1.0f)) < EPSILON);
    
    // Partial similarity
    float vec7[] = {1.0f, 0.0f, 0.0f};
    float vec8[] = {1.0f, 1.0f, 0.0f};
    sim = cosine_similarity(vec7, vec8, 3);
    float expected = 1.0f / sqrtf(2.0f);  // cos(45°)
    assert_true(suite, "45° angle similarity ≈ 0.707", 
               fabs(sim - expected) < EPSILON);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_search_basic(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Basic Search");
    
    VectorIndex* index = vector_index_create(3);
    
    // Add orthogonal unit vectors
    float vec1[] = {1.0f, 0.0f, 0.0f};  // X-axis
    float vec2[] = {0.0f, 1.0f, 0.0f};  // Y-axis
    float vec3[] = {0.0f, 0.0f, 1.0f};  // Z-axis
    
    vector_index_add(index, 1, vec1);
    vector_index_add(index, 2, vec2);
    vector_index_add(index, 3, vec3);
    
    // Search for X-axis vector
    float query[] = {1.0f, 0.0f, 0.0f};
    size_t count;
    VectorResult* results = vector_search(index, query, 1, &count);
    
    assert_equal_int(suite, "Search returns 1 result", 1, (int)count);
    assert_equal_int(suite, "Top result is doc_id 1", 1, (int)results[0].doc_id);
    assert_true(suite, "Top similarity ≈ 1.0", 
               fabs(results[0].similarity - 1.0f) < EPSILON);
    
    free(results);
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_search_topk(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Top-K Search");
    
    VectorIndex* index = vector_index_create(2);
    
    // Add vectors at different angles
    float vec1[] = {1.0f, 0.0f};        // 0°
    float vec2[] = {0.707f, 0.707f};    // 45°
    float vec3[] = {0.0f, 1.0f};        // 90°
    float vec4[] = {-0.707f, 0.707f};   // 135°
    float vec5[] = {-1.0f, 0.0f};       // 180°
    
    vector_index_add(index, 1, vec1);
    vector_index_add(index, 2, vec2);
    vector_index_add(index, 3, vec3);
    vector_index_add(index, 4, vec4);
    vector_index_add(index, 5, vec5);
    
    // Query vector at 0°
    float query[] = {1.0f, 0.0f};
    size_t count;
    VectorResult* results = vector_search(index, query, 3, &count);
    
    assert_equal_int(suite, "Top-3 search returns 3 results", 3, (int)count);
    assert_equal_int(suite, "1st result is doc 1", 1, (int)results[0].doc_id);
    assert_equal_int(suite, "2nd result is doc 2", 2, (int)results[1].doc_id);
    assert_equal_int(suite, "3rd result is doc 3", 3, (int)results[2].doc_id);
    
    // Verify descending order
    assert_true(suite, "Results in descending order", 
               results[0].similarity >= results[1].similarity &&
               results[1].similarity >= results[2].similarity);
    
    free(results);
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_edge_cases(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Edge Cases");
    
    VectorIndex* index = vector_index_create(3);
    
    // Zero vector
    float zero_vec[] = {0.0f, 0.0f, 0.0f};
    vector_index_add(index, 1, zero_vec);
    
    float query[] = {1.0f, 0.0f, 0.0f};
    size_t count;
    VectorResult* results = vector_search(index, query, 1, &count);
    
    // Similarity with zero vector should be 0
    assert_equal_int(suite, "Search with zero vector", 1, (int)count);
    assert_true(suite, "Zero vector similarity is 0", 
               fabs(results[0].similarity - 0.0f) < EPSILON);
    free(results);
    
    // Query with zero vector
    float zero_query[] = {0.0f, 0.0f, 0.0f};
    results = vector_search(index, zero_query, 1, &count);
    assert_equal_int(suite, "Zero query returns results", 1, (int)count);
    free(results);
    
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_large_dimension(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Large Dimension");
    
    const int dim = 1024;  // BAAI/bge-m3 dimension
    VectorIndex* index = vector_index_create(dim);
    
    // Create random-ish vectors
    float* vec1 = (float*)malloc(dim * sizeof(float));
    float* vec2 = (float*)malloc(dim * sizeof(float));
    float* query = (float*)malloc(dim * sizeof(float));
    
    for (int i = 0; i < dim; i++) {
        vec1[i] = (i % 10) / 10.0f;
        vec2[i] = ((i + 5) % 10) / 10.0f;
        query[i] = (i % 10) / 10.0f;
    }
    
    vector_index_add(index, 1, vec1);
    vector_index_add(index, 2, vec2);
    
    size_t count;
    VectorResult* results = vector_search(index, query, 2, &count);
    
    assert_equal_int(suite, "Large dimension search", 2, (int)count);
    assert_equal_int(suite, "Top result is vec1", 1, (int)results[0].doc_id);
    
    free(results);
    free(vec1);
    free(vec2);
    free(query);
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_vector_memory_stress(void) {
    TestSuite* suite = test_suite_create("VectorIndex - Memory Stress");
    
    VectorIndex* index = vector_index_create(128);
    
    // Add many vectors
    const int num_vectors = 1000;
    for (int i = 0; i < num_vectors; i++) {
        float vec[128];
        for (int j = 0; j < 128; j++) {
            vec[j] = ((i + j) % 100) / 100.0f;
        }
        vector_index_add(index, i, vec);
    }
    
    assert_equal_int(suite, "1000 vectors added", num_vectors, 
                    (int)vector_index_count(index));
    
    // Search
    float query[128];
    for (int j = 0; j < 128; j++) {
        query[j] = 0.5f;
    }
    
    size_t count;
    VectorResult* results = vector_search(index, query, 10, &count);
    assert_equal_int(suite, "Top-10 search returns 10", 10, (int)count);
    
    free(results);
    vector_index_free(index);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== Vector Index Module Test Suite ===\n\n");
    
    test_vector_index_creation();
    test_vector_index_add();
    test_cosine_similarity();
    test_vector_search_basic();
    test_vector_search_topk();
    test_vector_edge_cases();
    test_vector_large_dimension();
    test_vector_memory_stress();
    
    return 0;
}
