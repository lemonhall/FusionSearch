/**
 * FusionSearch JNI 桥接层
 * 
 * 连接 Kotlin/Java 和 C 核心引擎
 */

#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include "../include/search.h"
#include "../include/vector_index.h"

// ============================================================================
// JNI 辅助宏
// ============================================================================

#define JNI_METHOD(returnType, methodName) \
    JNIEXPORT returnType JNICALL Java_com_fusionsearch_FusionSearch_##methodName

// ============================================================================
// 索引加载
// ============================================================================

/**
 * 加载 BM25 索引
 * 
 * @param env JNI 环境
 * @param obj 对象实例
 * @param jsonl_file JSONL 文件路径
 * @return 索引指针（long）
 */
JNI_METHOD(jlong, nativeIndexLoad)(JNIEnv* env, jobject obj, jstring jsonl_file) {
    const char* file_path = (*env)->GetStringUTFChars(env, jsonl_file, NULL);
    if (file_path == NULL) {
        return 0;
    }
    
    uintptr_t index_ptr = ffi_index_load(file_path);
    
    (*env)->ReleaseStringUTFChars(env, jsonl_file, file_path);
    
    return (jlong)index_ptr;
}

/**
 * 加载向量索引
 */
JNI_METHOD(jlong, nativeVectorIndexLoad)(JNIEnv* env, jobject obj, jstring vector_file) {
    const char* file_path = (*env)->GetStringUTFChars(env, vector_file, NULL);
    if (file_path == NULL) {
        return 0;
    }
    
    uintptr_t index_ptr = ffi_vector_index_load(file_path);
    
    (*env)->ReleaseStringUTFChars(env, vector_file, file_path);
    
    return (jlong)index_ptr;
}

// ============================================================================
// BM25 搜索
// ============================================================================

/**
 * BM25 搜索
 * 
 * @param env JNI 环境
 * @param obj 对象实例
 * @param index_ptr 索引指针
 * @param query 查询字符串
 * @param k Top-K 数量
 * @param out_doc_ids 输出：文档 ID 数组
 * @param out_scores 输出：分数数组
 * @return 返回结果数量
 */
JNI_METHOD(jint, nativeBM25Search)(
    JNIEnv* env,
    jobject obj,
    jlong index_ptr,
    jstring query,
    jint k,
    jintArray out_doc_ids,
    jfloatArray out_scores
) {
    // 获取查询字符串
    const char* query_str = (*env)->GetStringUTFChars(env, query, NULL);
    if (query_str == NULL) {
        return 0;
    }
    
    // 分配临时缓冲区
    uint32_t* doc_ids = (uint32_t*)malloc(k * sizeof(uint32_t));
    float* scores = (float*)malloc(k * sizeof(float));
    
    if (doc_ids == NULL || scores == NULL) {
        free(doc_ids);
        free(scores);
        (*env)->ReleaseStringUTFChars(env, query, query_str);
        return 0;
    }
    
    // 调用 C 搜索函数
    size_t count = ffi_bm25_search(
        (uintptr_t)index_ptr,
        query_str,
        (uint32_t)k,
        doc_ids,
        scores
    );
    
    // 释放查询字符串
    (*env)->ReleaseStringUTFChars(env, query, query_str);
    
    // 复制结果到 Java 数组
    if (count > 0) {
        // 将 uint32_t 转为 jint
        jint* java_doc_ids = (jint*)malloc(count * sizeof(jint));
        for (size_t i = 0; i < count; i++) {
            java_doc_ids[i] = (jint)doc_ids[i];
        }
        
        (*env)->SetIntArrayRegion(env, out_doc_ids, 0, count, java_doc_ids);
        (*env)->SetFloatArrayRegion(env, out_scores, 0, count, scores);
        
        free(java_doc_ids);
    }
    
    free(doc_ids);
    free(scores);
    
    return (jint)count;
}

// ============================================================================
// 向量检索
// ============================================================================

/**
 * 向量检索
 */
JNI_METHOD(jint, nativeVectorSearch)(
    JNIEnv* env,
    jobject obj,
    jlong index_ptr,
    jfloatArray query_embedding,
    jint dimension,
    jint k,
    jintArray out_doc_ids,
    jfloatArray out_scores
) {
    // 获取查询向量
    jfloat* embedding = (*env)->GetFloatArrayElements(env, query_embedding, NULL);
    if (embedding == NULL) {
        return 0;
    }
    
    // 分配临时缓冲区
    uint32_t* doc_ids = (uint32_t*)malloc(k * sizeof(uint32_t));
    float* scores = (float*)malloc(k * sizeof(float));
    
    if (doc_ids == NULL || scores == NULL) {
        free(doc_ids);
        free(scores);
        (*env)->ReleaseFloatArrayElements(env, query_embedding, embedding, JNI_ABORT);
        return 0;
    }
    
    // 调用 C 搜索函数
    size_t count = ffi_vector_search(
        (uintptr_t)index_ptr,
        embedding,
        (uint32_t)dimension,
        (uint32_t)k,
        doc_ids,
        scores
    );
    
    // 释放查询向量
    (*env)->ReleaseFloatArrayElements(env, query_embedding, embedding, JNI_ABORT);
    
    // 复制结果到 Java 数组
    if (count > 0) {
        jint* java_doc_ids = (jint*)malloc(count * sizeof(jint));
        for (size_t i = 0; i < count; i++) {
            java_doc_ids[i] = (jint)doc_ids[i];
        }
        
        (*env)->SetIntArrayRegion(env, out_doc_ids, 0, count, java_doc_ids);
        (*env)->SetFloatArrayRegion(env, out_scores, 0, count, scores);
        
        free(java_doc_ids);
    }
    
    free(doc_ids);
    free(scores);
    
    return (jint)count;
}

// ============================================================================
// 文档查询
// ============================================================================

/**
 * 获取文档内容
 */
JNI_METHOD(jint, nativeGetDocument)(
    JNIEnv* env,
    jobject obj,
    jlong index_ptr,
    jint doc_id,
    jbyteArray out_title,
    jint title_size,
    jbyteArray out_content,
    jint content_size
) {
    // 分配 C 缓冲区
    char* title_buffer = (char*)malloc(title_size);
    char* content_buffer = (char*)malloc(content_size);
    
    if (title_buffer == NULL || content_buffer == NULL) {
        free(title_buffer);
        free(content_buffer);
        return -1;
    }
    
    // 调用 C 查询函数
    int result = ffi_get_document(
        (uintptr_t)index_ptr,
        (uint32_t)doc_id,
        title_buffer,
        (size_t)title_size,
        content_buffer,
        (size_t)content_size
    );
    
    if (result == 0) {
        // 成功，复制到 Java 数组
        jsize title_len = (jsize)strlen(title_buffer);
        jsize content_len = (jsize)strlen(content_buffer);
        
        (*env)->SetByteArrayRegion(env, out_title, 0, title_len, (jbyte*)title_buffer);
        (*env)->SetByteArrayRegion(env, out_content, 0, content_len, (jbyte*)content_buffer);
    }
    
    free(title_buffer);
    free(content_buffer);
    
    return result;
}

// ============================================================================
// 资源释放
// ============================================================================

/**
 * 释放 BM25 索引
 */
JNI_METHOD(void, nativeIndexFree)(JNIEnv* env, jobject obj, jlong index_ptr) {
    ffi_index_free((uintptr_t)index_ptr);
}

/**
 * 释放向量索引
 */
JNI_METHOD(void, nativeVectorIndexFree)(JNIEnv* env, jobject obj, jlong index_ptr) {
    ffi_vector_index_free((uintptr_t)index_ptr);
}
