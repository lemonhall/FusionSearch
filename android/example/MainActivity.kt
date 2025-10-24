package com.fusionsearch.example

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.fusionsearch.FusionSearch
import com.fusionsearch.HybridSearchEngine
import com.fusionsearch.example.databinding.ActivityMainBinding
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream

/**
 * FusionSearch 示例应用
 * 
 * 演示如何在 Android 中使用混合搜索引擎
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var binding: ActivityMainBinding
    private lateinit var fusionSearch: FusionSearch
    private lateinit var hybridEngine: HybridSearchEngine
    private lateinit var apiClient: ApiClient
    private lateinit var adapter: SearchResultAdapter
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        // 初始化 API 客户端
        apiClient = ApiClient(getString(R.string.api_key))
        
        // 设置 RecyclerView
        setupRecyclerView()
        
        // 初始化搜索引擎
        initSearchEngine()
        
        // 设置搜索按钮
        binding.btnSearch.setOnClickListener {
            val query = binding.etQuery.text.toString().trim()
            if (query.isNotEmpty()) {
                performSearch(query)
            }
        }
        
        // 设置搜索模式切换
        binding.rgSearchMode.setOnCheckedChangeListener { _, checkedId ->
            // 可以根据不同模式切换搜索策略
        }
    }
    
    private fun setupRecyclerView() {
        adapter = SearchResultAdapter()
        binding.rvResults.apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = this@MainActivity.adapter
        }
    }
    
    private fun initSearchEngine() {
        binding.tvStatus.text = "正在初始化搜索引擎..."
        binding.progressBar.visibility = View.VISIBLE
        
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                // 1. 从 assets 复制数据文件
                val jsonlFile = copyAssetToFile("recipes.jsonl")
                val vectorFile = copyAssetToFile("vectors.bin")
                
                // 2. 初始化 FusionSearch
                fusionSearch = FusionSearch()
                fusionSearch.loadIndex(jsonlFile.absolutePath)
                fusionSearch.loadVectorIndex(vectorFile.absolutePath)
                
                // 3. 创建混合搜索引擎
                hybridEngine = HybridSearchEngine(fusionSearch)
                
                withContext(Dispatchers.Main) {
                    binding.tvStatus.text = "初始化完成！请输入搜索内容"
                    binding.progressBar.visibility = View.GONE
                    binding.btnSearch.isEnabled = true
                }
                
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    binding.tvStatus.text = "初始化失败: ${e.message}"
                    binding.progressBar.visibility = View.GONE
                }
            }
        }
    }
    
    private fun copyAssetToFile(assetName: String): File {
        val outFile = File(filesDir, assetName)
        
        if (!outFile.exists()) {
            assets.open(assetName).use { input ->
                FileOutputStream(outFile).use { output ->
                    input.copyTo(output)
                }
            }
        }
        
        return outFile
    }
    
    private fun performSearch(query: String) {
        binding.tvStatus.text = "搜索中..."
        binding.progressBar.visibility = View.VISIBLE
        binding.btnSearch.isEnabled = false
        
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val results = when (binding.rgSearchMode.checkedRadioButtonId) {
                    R.id.rbBM25 -> performBM25Search(query)
                    R.id.rbVector -> performVectorSearch(query)
                    R.id.rbHybrid -> performHybridSearch(query)
                    else -> emptyList()
                }
                
                withContext(Dispatchers.Main) {
                    displayResults(results)
                    binding.tvStatus.text = "找到 ${results.size} 个结果"
                    binding.progressBar.visibility = View.GONE
                    binding.btnSearch.isEnabled = true
                }
                
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    binding.tvStatus.text = "搜索失败: ${e.message}"
                    binding.progressBar.visibility = View.GONE
                    binding.btnSearch.isEnabled = true
                }
            }
        }
    }
    
    private suspend fun performBM25Search(query: String): List<SearchResultItem> {
        val results = fusionSearch.bm25Search(query, 10)
        
        return results.map { result ->
            val doc = fusionSearch.getDocument(result.docId)
            SearchResultItem(
                docId = result.docId,
                title = doc.title,
                content = doc.content,
                score = result.score,
                scoreType = "BM25"
            )
        }
    }
    
    private suspend fun performVectorSearch(query: String): List<SearchResultItem> {
        // 1. 获取 Query 向量
        val queryEmbedding = apiClient.getEmbedding(query)
        
        // 2. 向量检索
        val results = fusionSearch.vectorSearch(queryEmbedding, 10)
        
        return results.map { result ->
            val doc = fusionSearch.getDocument(result.docId)
            SearchResultItem(
                docId = result.docId,
                title = doc.title,
                content = doc.content,
                score = result.score,
                scoreType = "Vector"
            )
        }
    }
    
    private suspend fun performHybridSearch(query: String): List<SearchResultItem> {
        // 1. 获取 Query 向量
        val queryEmbedding = apiClient.getEmbedding(query)
        
        // 2. 混合搜索
        val results = hybridEngine.hybridSearch(
            query = query,
            queryEmbedding = queryEmbedding,
            k = 10,
            useBM25 = true,
            useVector = true
        )
        
        return results.map { result ->
            val doc = fusionSearch.getDocument(result.docId)
            SearchResultItem(
                docId = result.docId,
                title = doc.title,
                content = doc.content,
                score = result.score,
                scoreType = "Hybrid (BM25: ${String.format("%.3f", result.bm25Score)}, Vector: ${String.format("%.3f", result.vectorScore)})"
            )
        }
    }
    
    private fun displayResults(results: List<SearchResultItem>) {
        adapter.submitList(results)
    }
    
    override fun onDestroy() {
        super.onDestroy()
        if (::fusionSearch.isInitialized) {
            fusionSearch.close()
        }
    }
}

// ============================================================================
// 数据类
// ============================================================================

data class SearchResultItem(
    val docId: Int,
    val title: String,
    val content: String,
    val score: Float,
    val scoreType: String
)

// ============================================================================
// RecyclerView Adapter
// ============================================================================

class SearchResultAdapter : RecyclerView.Adapter<SearchResultAdapter.ViewHolder>() {
    
    private var results: List<SearchResultItem> = emptyList()
    
    fun submitList(newResults: List<SearchResultItem>) {
        results = newResults
        notifyDataSetChanged()
    }
    
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_search_result, parent, false)
        return ViewHolder(view)
    }
    
    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.bind(results[position])
    }
    
    override fun getItemCount() = results.size
    
    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        private val tvTitle: TextView = view.findViewById(R.id.tvTitle)
        private val tvContent: TextView = view.findViewById(R.id.tvContent)
        private val tvScore: TextView = view.findViewById(R.id.tvScore)
        private val tvDocId: TextView = view.findViewById(R.id.tvDocId)
        
        fun bind(item: SearchResultItem) {
            tvTitle.text = item.title
            tvContent.text = item.content
            tvScore.text = "${item.scoreType}: ${String.format("%.4f", item.score)}"
            tvDocId.text = "Doc ID: ${item.docId}"
        }
    }
}

// ============================================================================
// API 客户端
// ============================================================================

class ApiClient(private val apiKey: String) {
    
    private val service: SiliconFlowApi by lazy {
        val okHttpClient = okhttp3.OkHttpClient.Builder()
            .addInterceptor { chain ->
                val request = chain.request().newBuilder()
                    .addHeader("Authorization", "Bearer $apiKey")
                    .build()
                chain.proceed(request)
            }
            .build()
        
        val retrofit = retrofit2.Retrofit.Builder()
            .baseUrl("https://api.siliconflow.cn/")
            .client(okHttpClient)
            .addConverterFactory(retrofit2.converter.gson.GsonConverterFactory.create())
            .build()
        
        retrofit.create(SiliconFlowApi::class.java)
    }
    
    suspend fun getEmbedding(text: String): FloatArray {
        return withContext(Dispatchers.IO) {
            val response = service.getEmbedding(
                EmbeddingRequest(input = text)
            )
            response.data.first().embedding.toFloatArray()
        }
    }
}

// API 接口
interface SiliconFlowApi {
    @retrofit2.http.POST("v1/embeddings")
    @retrofit2.http.Headers("Content-Type: application/json")
    suspend fun getEmbedding(
        @retrofit2.http.Body request: EmbeddingRequest
    ): EmbeddingResponse
}

data class EmbeddingRequest(
    val model: String = "BAAI/bge-m3",
    val input: String
)

data class EmbeddingResponse(
    val data: List<EmbeddingData>
)

data class EmbeddingData(
    val embedding: List<Float>
)
