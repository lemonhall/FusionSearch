/**
 * Unit Tests for BM25 Scoring Algorithm
 * 
 * Coverage:
 * - IDF calculation
 * - BM25 score calculation
 * - Parameter effects (k1, b)
 * - Score saturation
 * - Document length normalization
 */

#include "test.h"
#include "bm25.h"
#include <math.h>

#define EPSILON 0.0001f

void test_bm25_params(void) {
    TestSuite* suite = test_suite_create("BM25 - Parameters");
    
    BM25Params* params = bm25_params_create();
    assert_true(suite, "Params creation", params != NULL);
    
    // Default values
    assert_true(suite, "Default k1 is 1.5", fabs(params->k1 - 1.5f) < EPSILON);
    assert_true(suite, "Default b is 0.75", fabs(params->b - 0.75f) < EPSILON);
    
    // Custom values
    BM25Params* custom = bm25_params_create_custom(2.0f, 0.5f, 8.0f);
    assert_true(suite, "Custom k1 is 2.0", fabs(custom->k1 - 2.0f) < EPSILON);
    assert_true(suite, "Custom b is 0.5", fabs(custom->b - 0.5f) < EPSILON);
    
    bm25_params_destroy(params);
    bm25_params_destroy(custom);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_idf_calculation(void) {
    TestSuite* suite = test_suite_create("BM25 - IDF Calculation");
    
    // Rare term (appears in 1 out of 100 docs)
    float idf_rare = bm25_calculate_idf(100, 1);
    assert_true(suite, "Rare term IDF > 0", idf_rare > 0.0f);
    
    // Common term (appears in 50 out of 100 docs)
    float idf_common = bm25_calculate_idf(100, 50);
    assert_true(suite, "Common term IDF > 0", idf_common > 0.0f);
    
    // Rare term should have higher IDF
    assert_true(suite, "Rare term IDF > Common term IDF", idf_rare > idf_common);
    
    // Very common term (appears in 99 out of 100 docs)
    float idf_very_common = bm25_calculate_idf(100, 99);
    assert_true(suite, "Very common term has low IDF", idf_very_common < idf_common);
    
    // Edge case: term in all docs
    float idf_all = bm25_calculate_idf(100, 100);
    assert_true(suite, "Ubiquitous term IDF is very low", idf_all < 0.1f);
    
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_score_basic(void) {
    TestSuite* suite = test_suite_create("BM25 - Basic Score Calculation");
    
    BM25Params* params = bm25_params_create();
    float idf = 2.0f;
    float avgLen = 100.0f;
    
    // Basic case: TF=1, doc length = avg length
    float score = bm25_calculate_score(1, 100, avgLen, idf, params);
    assert_true(suite, "Basic score > 0", score > 0.0f);
    
    // Higher term frequency gives higher score
    float score_tf1 = bm25_calculate_score(1, 100, avgLen, idf, params);
    float score_tf5 = bm25_calculate_score(5, 100, avgLen, idf, params);
    assert_true(suite, "Higher TF = higher score", score_tf5 > score_tf1);
    
    // Higher IDF gives higher score
    float score_idf1 = bm25_calculate_score(1, 100, avgLen, 1.0f, params);
    float score_idf3 = bm25_calculate_score(1, 100, avgLen, 3.0f, params);
    assert_true(suite, "Higher IDF = higher score", score_idf3 > score_idf1);
    
    bm25_params_destroy(params);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_score_saturation(void) {
    TestSuite* suite = test_suite_create("BM25 - Score Saturation");
    
    BM25Params* params = bm25_params_create();
    float idf = 2.0f;
    float avgLen = 100.0f;
    
    // Score growth should slow down with high TF
    float score_tf1 = bm25_calculate_score(1, 100, avgLen, idf, params);
    float score_tf10 = bm25_calculate_score(10, 100, avgLen, idf, params);
    float score_tf100 = bm25_calculate_score(100, 100, avgLen, idf, params);
    
    float growth1 = score_tf10 - score_tf1;
    float growth2 = score_tf100 - score_tf10;
    
    assert_true(suite, "Score growth saturates", growth2 < growth1);
    
    // Very high TF should not increase score much
    float score_tf1000 = bm25_calculate_score(1000, 100, avgLen, idf, params);
    float score_tf10000 = bm25_calculate_score(10000, 100, avgLen, idf, params);
    
    float diff = score_tf10000 - score_tf1000;
    assert_true(suite, "Very high TF has minimal effect", diff < 1.0f);
    
    bm25_params_destroy(params);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_length_normalization(void) {
    TestSuite* suite = test_suite_create("BM25 - Length Normalization");
    
    BM25Params* params = bm25_params_create();
    float idf = 2.0f;
    float avgLen = 100.0f;
    
    // Same TF, different doc lengths
    float score_short = bm25_calculate_score(1, 50, avgLen, idf, params);   // Half avg
    float score_avg = bm25_calculate_score(1, 100, avgLen, idf, params);    // Average
    float score_long = bm25_calculate_score(1, 200, avgLen, idf, params);   // Double avg
    
    // Shorter docs should get slight boost
    assert_true(suite, "Shorter doc scores higher", score_short > score_avg);
    assert_true(suite, "Average doc scores higher than long doc", score_avg > score_long);
    
    // But the effect shouldn't be too dramatic (controlled by b parameter)
    assert_true(suite, "Length effect is moderate", score_short < score_avg * 1.5f);
    
    bm25_params_destroy(params);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_parameter_effects(void) {
    TestSuite* suite = test_suite_create("BM25 - Parameter Effects");
    
    float idf = 2.0f;
    float avgLen = 100.0f;
    
    // Test k1 parameter (term frequency saturation)
    BM25Params* params_k1_low = bm25_params_create_custom(0.5f, 0.75f, 8.0f);
    BM25Params* params_k1_high = bm25_params_create_custom(3.0f, 0.75f, 8.0f);
    
    float score_k1_low = bm25_calculate_score(10, 100, avgLen, idf, params_k1_low);
    float score_k1_high = bm25_calculate_score(10, 100, avgLen, idf, params_k1_high);
    
    // Higher k1 means less saturation (higher score for high TF)
    assert_true(suite, "Higher k1 = higher score for high TF", score_k1_high > score_k1_low);
    
    // Test b parameter (length normalization)
    BM25Params* params_b0 = bm25_params_create_custom(1.5f, 0.0f, 8.0f);
    BM25Params* params_b1 = bm25_params_create_custom(1.5f, 1.0f, 8.0f);
    
    float score_b0_short = bm25_calculate_score(1, 50, avgLen, idf, params_b0);
    float score_b0_long = bm25_calculate_score(1, 200, avgLen, idf, params_b0);
    float score_b1_short = bm25_calculate_score(1, 50, avgLen, idf, params_b1);
    float score_b1_long = bm25_calculate_score(1, 200, avgLen, idf, params_b1);
    
    // b=0 means no length normalization (scores should be same)
    float diff_b0 = fabs(score_b0_short - score_b0_long);
    // b=1 means full length normalization (scores should differ)
    float diff_b1 = fabs(score_b1_short - score_b1_long);
    
    assert_true(suite, "b=0 has minimal length effect", diff_b0 < diff_b1);
    
    bm25_params_destroy(params_k1_low);
    bm25_params_destroy(params_k1_high);
    bm25_params_destroy(params_b0);
    bm25_params_destroy(params_b1);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

void test_bm25_edge_cases(void) {
    TestSuite* suite = test_suite_create("BM25 - Edge Cases");
    
    BM25Params* params = bm25_params_create();
    
    // Zero term frequency
    float score = bm25_calculate_score(0, 100, 100.0f, 2.0f, params);
    assert_true(suite, "Zero TF gives zero score", fabs(score - 0.0f) < EPSILON);
    
    // Zero document length (edge case, shouldn't crash)
    score = bm25_calculate_score(1, 0, 100.0f, 2.0f, params);
    assert_true(suite, "Zero doc length doesn't crash", 1);
    
    // Zero IDF (ubiquitous term)
    score = bm25_calculate_score(1, 100, 100.0f, 0.0f, params);
    assert_true(suite, "Zero IDF gives zero score", fabs(score - 0.0f) < EPSILON);
    
    // Negative IDF (shouldn't happen in practice, but test robustness)
    score = bm25_calculate_score(1, 100, 100.0f, -1.0f, params);
    assert_true(suite, "Negative IDF doesn't crash", 1);
    
    bm25_params_destroy(params);
    test_suite_print_results(suite);
    test_suite_destroy(suite);
}

int main(void) {
    printf("\n=== BM25 Algorithm Test Suite ===\n\n");
    
    test_bm25_params();
    test_bm25_idf_calculation();
    test_bm25_score_basic();
    test_bm25_score_saturation();
    test_bm25_length_normalization();
    test_bm25_parameter_effects();
    test_bm25_edge_cases();
    
    return 0;
}
