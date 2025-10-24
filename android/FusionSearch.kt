package com.fusionsearch

import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * FusionSearch - Android JNI 封装
 * 
 * 混合搜索引擎的 Kotlin 封装，提供 BM25 + 向量检索能力
 * 
 * 使用示例：
 * ```kotlin
 * val engine = FusionSearch()
 * engine.loadIndex("/path/to/recipes.jsonl")
 * engine.loadVectorIndex("/path/to/vectors.bin")
 * 
 * // BM25 搜索
 * val bm25Results = engine.bm25Search("芹菜", 10)
 * 
 * // 向量检索
 * val queryVector = floatArrayOf(0.1f, 0.2f, ...) // 1024维
 * val vectorResults = engine.vectorSearch(queryVector, 10)
 * 
 * // 获取文档内容
 * val doc = engine.getDocument(1)
 * println("标题: ${doc.title}, 内容: ${doc.content}")
 * ```
 */
class FusionSearch {
    
    companion object {
        init {
            // 加载本地库
            // 注意：需要将 libfusion.so 放在 app/src/main/jniLibs/arm64-v8a/ 或 armeabi-v7a/
            System.loadLibrary("fusion")
        }
    }
    
    // Native 方法声明
    private external fun nativeIndexLoad(jsonlFile: String): Long
    private external fun nativeVectorIndexLoad(vectorFile: String): Long
    private external fun nativeBM25Search(
        indexPtr: Long,
        query: String,
        k: Int,
        outDocIds: IntArray,
        outScores: FloatArray
    ): Int
    private external fun nativeVectorSearch(
        indexPtr: Long,
        queryEmbedding: FloatArray,
        dimension: Int,
        k: Int,
        outDocIds: IntArray,
        outScores: FloatArray
    ): Int
    private external fun nativeGetDocument(
        indexPtr: Long,
        docId: Int,
        outTitle: ByteArray,
        titleSize: Int,
        outContent: ByteArray,
        contentSize: Int
    ): Int
    private external fun nativeIndexFree(indexPtr: Long)
    private external fun nativeVectorIndexFree(indexPtr: Long)
    
    // 索引指针
    private var indexPtr: Long = 0
    private var vectorIndexPtr: Long = 0
    
    /**
     * 搜索结果数据类
     */
    data class SearchResult(
        val docId: Int,
        val score: Float
    )
    
    /**
     * 文档数据类
     */
    data class Document(
        val title: String,
        val content: String
    )
    
    /**
     * 加载 BM25 索引
     * 
     * @param jsonlFile JSONL 文件路径（可以是 assets 或内部存储）
     * @throws RuntimeException 如果加载失败
     */
    fun loadIndex(jsonlFile: String) {
        indexPtr = nativeIndexLoad(jsonlFile)
        if (indexPtr == 0L) {
            throw RuntimeException("Failed to load BM25 index from: $jsonlFile")
        }
    }
    
    /**
     * 加载向量索引
     * 
     * @param vectorFile vectors.bin 文件路径
     * @throws RuntimeException 如果加载失败
     */
    fun loadVectorIndex(vectorFile: String) {
        vectorIndexPtr = nativeVectorIndexLoad(vectorFile)
        if (vectorIndexPtr == 0L) {
            throw RuntimeException("Failed to load vector index from: $vectorFile")
        }
    }
    
    /**
     * BM25 全文检索
     * 
     * @param query 查询字符串
     * @param k 返回结果数量（默认 10）
     * @return 搜索结果列表（按相关性降序排列）
     */
    fun bm25Search(query: String, k: Int = 10): List<SearchResult> {
        if (indexPtr == 0L) {
            throw IllegalStateException("Index not loaded. Call loadIndex() first.")
        }
        
        val docIds = IntArray(k)
        val scores = FloatArray(k)
        
        val count = nativeBM25Search(indexPtr, query, k, docIds, scores)
        
        return (0 until count).map { SearchResult(docIds[it], scores[it]) }
    }
    
    /**
     * 向量语义检索
     * 
     * @param queryEmbedding 查询向量（1024 维）
     * @param k 返回结果数量（默认 10）
     * @return 搜索结果列表（按相似度降序排列）
     */
    fun vectorSearch(queryEmbedding: FloatArray, k: Int = 10): List<SearchResult> {
        if (vectorIndexPtr == 0L) {
            throw IllegalStateException("Vector index not loaded. Call loadVectorIndex() first.")
        }
        
        val dimension = queryEmbedding.size
        val docIds = IntArray(k)
        val scores = FloatArray(k)
        
        val count = nativeVectorSearch(vectorIndexPtr, queryEmbedding, dimension, k, docIds, scores)
        
        return (0 until count).map { SearchResult(docIds[it], scores[it]) }
    }
    
    /**
     * 获取文档内容
     * 
     * @param docId 文档 ID
     * @return 文档对象（包含标题和内容），如果不存在返回空文档
     */
    fun getDocument(docId: Int): Document {
        if (indexPtr == 0L) {
            throw IllegalStateException("Index not loaded. Call loadIndex() first.")
        }
        
        val titleBuffer = ByteArray(1024)  // 增大到 1KB
        val contentBuffer = ByteArray(4096) // 增大到 4KB
        
        val result = nativeGetDocument(indexPtr, docId, titleBuffer, 1024, contentBuffer, 4096)
        
        if (result != 0) {
            return Document("", "")
        }
        
        // UTF-8 解码
        val title = String(titleBuffer.takeWhile { it != 0.toByte() }.toByteArray(), Charsets.UTF_8)
        val content = String(contentBuffer.takeWhile { it != 0.toByte() }.toByteArray(), Charsets.UTF_8)
        
        return Document(title, content)
    }
    
    /**
     * 释放资源
     */
    fun close() {
        if (indexPtr != 0L) {
            nativeIndexFree(indexPtr)
            indexPtr = 0
        }
        if (vectorIndexPtr != 0L) {
            nativeVectorIndexFree(vectorIndexPtr)
            vectorIndexPtr = 0
        }
    }
    
    /**
     * 析构时自动释放资源
     */
    protected fun finalize() {
        close()
    }
}

/**
 * 混合搜索引擎（带融合排序）
 * 
 * 集成 BM25 和向量检索，提供统一的搜索接口
 */
class HybridSearchEngine(
    private val fusionSearch: FusionSearch
) {
    
    /**
     * 融合搜索结果
     */
    data class HybridResult(
        val docId: Int,
        val score: Float,
        val bm25Score: Float = 0f,
        val vectorScore: Float = 0f
    )
    
    /**
     * 混合搜索（需要外部提供 Query 向量）
     * 
     * @param query 查询字符串
     * @param queryEmbedding 查询向量（从 API 获取）
     * @param k 返回结果数量
     * @param bm25Weight BM25 权重（默认 0.5）
     * @param vectorWeight 向量权重（默认 0.5）
     * @param useBM25 是否使用 BM25（默认 true）
     * @param useVector 是否使用向量检索（默认 true）
     * @return 融合排序后的结果列表
     */
    fun hybridSearch(
        query: String,
        queryEmbedding: FloatArray? = null,
        k: Int = 10,
        bm25Weight: Float = 0.5f,
        vectorWeight: Float = 0.5f,
        useBM25: Boolean = true,
        useVector: Boolean = true
    ): List<HybridResult> {
        
        val bm25Results = if (useBM25) {
            fusionSearch.bm25Search(query, k * 2)  // 多取一些候选
        } else {
            emptyList()
        }
        
        val vectorResults = if (useVector && queryEmbedding != null) {
            fusionSearch.vectorSearch(queryEmbedding, k * 2)
        } else {
            emptyList()
        }
        
        // RRF 融合排序
        return reciprocalRankFusion(bm25Results, vectorResults, k)
    }
    
    /**
     * Reciprocal Rank Fusion (RRF) 算法
     * 
     * 工业界标准的融合排序算法，不需要归一化分数
     */
    private fun reciprocalRankFusion(
        bm25Results: List<FusionSearch.SearchResult>,
        vectorResults: List<FusionSearch.SearchResult>,
        k: Int,
        constant: Int = 60
    ): List<HybridResult> {
        
        val scores = mutableMapOf<Int, Float>()
        val bm25Scores = mutableMapOf<Int, Float>()
        val vectorScores = mutableMapOf<Int, Float>()
        
        // BM25 结果贡献
        bm25Results.forEachIndexed { rank, result ->
            val score = 1.0f / (constant + rank + 1)
            scores[result.docId] = scores.getOrDefault(result.docId, 0f) + score
            bm25Scores[result.docId] = result.score
        }
        
        // 向量结果贡献
        vectorResults.forEachIndexed { rank, result ->
            val score = 1.0f / (constant + rank + 1)
            scores[result.docId] = scores.getOrDefault(result.docId, 0f) + score
            vectorScores[result.docId] = result.score
        }
        
        // 排序并返回 Top-K
        return scores.entries
            .sortedByDescending { it.value }
            .take(k)
            .map { (docId, score) ->
                HybridResult(
                    docId = docId,
                    score = score,
                    bm25Score = bm25Scores.getOrDefault(docId, 0f),
                    vectorScore = vectorScores.getOrDefault(docId, 0f)
                )
            }
    }
    
    /**
     * 加权融合排序（简单加权平均）
     */
    private fun weightedFusion(
        bm25Results: List<FusionSearch.SearchResult>,
        vectorResults: List<FusionSearch.SearchResult>,
        k: Int,
        bm25Weight: Float,
        vectorWeight: Float
    ): List<HybridResult> {
        
        val scores = mutableMapOf<Int, Pair<Float, Float>>() // (bm25Score, vectorScore)
        
        // 归一化分数
        val bm25Max = bm25Results.maxOfOrNull { it.score } ?: 1f
        val vectorMax = vectorResults.maxOfOrNull { it.score } ?: 1f
        
        bm25Results.forEach { result ->
            val normalizedScore = result.score / bm25Max
            scores[result.docId] = Pair(normalizedScore, 0f)
        }
        
        vectorResults.forEach { result ->
            val normalizedScore = result.score / vectorMax
            val current = scores.getOrDefault(result.docId, Pair(0f, 0f))
            scores[result.docId] = Pair(current.first, normalizedScore)
        }
        
        // 加权融合
        return scores.entries
            .map { (docId, pair) ->
                val finalScore = pair.first * bm25Weight + pair.second * vectorWeight
                HybridResult(
                    docId = docId,
                    score = finalScore,
                    bm25Score = pair.first,
                    vectorScore = pair.second
                )
            }
            .sortedByDescending { it.score }
            .take(k)
    }
}
