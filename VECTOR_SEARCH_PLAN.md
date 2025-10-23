# 向量检索 + 持久化存储实施计划

## 📋 项目概述

在现有的 FusionSearch 全文搜索引擎基础上，扩展以下功能：

1. **索引持久化** - 避免每次启动重新加载文档
2. **向量检索** - 基于语义相似度的搜索（纯 C 实现）
3. **混合检索** - 融合 BM25 和向量检索
4. **RAG 支持** - 为检索增强生成提供基础

---

## 🎯 应用场景

### 典型用例：4万份菜谱搜索

**需求**：
- 全文关键词搜索（已实现）
- 语义相似度搜索（待实现）
- 快速启动（< 100ms）
- RAG 检索支持

**数据规模**：
- 文档数量：40,000 份
- 向量维度：768（典型的 Embedding 维度）
- 存储空间：~120 MB（向量数据）
- 内存占用：~170 MB（向量 + 文本索引）

---

## 🏗️ 技术架构

### 整体架构图

```
┌──────────────────────────────────────────────────┐
│          FusionSearch 混合检索引擎                │
└──────────────────────────────────────────────────┘
                    ↓
    ┌───────────────────────────────┐
    │      离线索引构建              │
    ├───────────────────────────────┤
    │  1. 文档分词（BM25 索引）      │
    │  2. 向量化（调用外部 API）     │
    │  3. 保存索引到磁盘             │
    └───────────────────────────────┘
                    ↓
    ┌───────────────────────────────┐
    │      在线检索服务              │
    ├───────────────────────────────┤
    │  1. 启动加载索引（< 100ms）    │
    │  2. 查询处理                  │
    │  3. 混合排序                  │
    └───────────────────────────────┘
                    ↓
         ┌──────────┴──────────┐
         ↓                     ↓
    ┌─────────┐          ┌─────────┐
    │ BM25    │          │ Vector  │
    │ Search  │          │ Search  │
    └─────────┘          └─────────┘
         ↓                     ↓
         └──────────┬──────────┘
                    ↓
            ┌───────────────┐
            │  混合排序      │
            │  (加权融合)    │
            └───────────────┘
```

---

## 📂 数据结构设计

### 1. 向量索引结构

```c
// 单个向量条目
typedef struct {
    uint32_t doc_id;           // 文档 ID
    float* embedding;          // 向量数据（动态分配）
    uint32_t dimension;        // 向量维度（如 768）
} VectorEntry;

// 向量索引
typedef struct {
    VectorEntry* entries;      // 向量数组
    uint32_t count;            // 向量数量
    uint32_t dimension;        // 统一维度
    uint32_t capacity;         // 数组容量
} VectorIndex;

// 检索结果
typedef struct {
    uint32_t doc_id;
    float score;               // 相似度分数 [0, 1]
} VectorSearchResult;
```

### 2. 持久化文件格式

#### 倒排索引文件格式（`index.bin`）

```
┌─────────────────────────────────────┐
│  Header (16 bytes)                  │
│  - magic: "FSIN" (4 bytes)          │
│  - version: 1 (4 bytes)             │
│  - doc_count: uint32_t (4 bytes)    │
│  - term_count: uint32_t (4 bytes)   │
├─────────────────────────────────────┤
│  Document Metadata                  │
│  - doc_id (4 bytes)                 │
│  - title_len (4 bytes)              │
│  - title (variable)                 │
│  - content_len (4 bytes)            │
│  - content (variable)               │
│  - word_count (4 bytes)             │
│  (repeat for all documents)         │
├─────────────────────────────────────┤
│  Inverted Index Data                │
│  - term_len (4 bytes)               │
│  - term (variable)                  │
│  - posting_count (4 bytes)          │
│  - posting_list (doc_id, freq)*N    │
│  (repeat for all terms)             │
└─────────────────────────────────────┘
```

#### 向量索引文件格式（`vectors.bin`）

```
┌─────────────────────────────────────┐
│  Header (16 bytes)                  │
│  - magic: "FSVC" (4 bytes)          │
│  - version: 1 (4 bytes)             │
│  - doc_count: uint32_t (4 bytes)    │
│  - dimension: uint32_t (4 bytes)    │
├─────────────────────────────────────┤
│  Vector Data                        │
│  - doc_id (4 bytes)                 │
│  - embedding (dimension * 4 bytes)  │
│  (repeat for all documents)         │
└─────────────────────────────────────┘
```

**存储空间估算（4万文档，768维）**：
- 向量数据：40,000 × (4 + 768×4) = 122.9 MB
- 倒排索引：~10 MB（取决于词汇量）
- **总计：~133 MB**

---

## 🚀 实施计划

### 阶段 1：索引持久化 ⭐⭐⭐⭐⭐ 优先级最高

**目标**：实现倒排索引的保存和快速加载

**新增文件**：
```
include/index_persistence.h      # 索引持久化接口
src/index_persistence.c          # 索引序列化/反序列化（~300 行）
```

**核心功能**：
```c
// 保存索引到文件
int index_save_to_file(InvertedIndex* index, const char* filepath);

// 从文件加载索引
InvertedIndex* index_load_from_file(const char* filepath);

// 检查索引文件是否存在
int index_file_exists(const char* filepath);
```

**实现要点**：
- 二进制格式（比 JSON 快 10-100 倍）
- 版本控制（支持未来升级）
- 校验和（检测文件损坏）

**预期性能**：
- 保存：~200 ms（4万文档）
- 加载：~50 ms（内存映射可优化到 ~5 ms）

---

### 阶段 2：向量检索实现 ⭐⭐⭐⭐

**目标**：纯 C 实现向量存储、检索和相似度计算

**新增文件**：
```
include/vector_index.h           # 向量索引接口
src/vector_index.c               # 向量检索实现（~250 行）
```

**核心功能**：
```c
// 创建向量索引
VectorIndex* vector_index_create(uint32_t dimension);

// 添加向量
int vector_index_add(VectorIndex* index, uint32_t doc_id, 
                     const float* embedding);

// 向量检索（Top-K）
VectorSearchResult* vector_search(VectorIndex* index, 
                                  const float* query_embedding,
                                  uint32_t k, size_t* result_count);

// 余弦相似度计算
float cosine_similarity(const float* vec1, const float* vec2, 
                       uint32_t dimension);

// 保存/加载向量索引
int vector_index_save(VectorIndex* index, const char* filepath);
VectorIndex* vector_index_load(const char* filepath);
```

**算法实现**：

#### 余弦相似度（核心算法）

```c
float cosine_similarity(const float* vec1, const float* vec2, 
                       uint32_t dimension) {
    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    // 向量点积 + 范数计算
    for (uint32_t i = 0; i < dimension; i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }
    
    // 余弦相似度 = dot(A, B) / (||A|| * ||B||)
    if (norm1 == 0.0f || norm2 == 0.0f) return 0.0f;
    return dot_product / (sqrtf(norm1) * sqrtf(norm2));
}
```

#### Top-K 检索（堆排序优化）

```c
VectorSearchResult* vector_search(VectorIndex* index, 
                                  const float* query_embedding,
                                  uint32_t k) {
    // 1. 计算所有文档的相似度
    VectorSearchResult* candidates = malloc(index->count * sizeof(...));
    
    for (uint32_t i = 0; i < index->count; i++) {
        candidates[i].doc_id = index->entries[i].doc_id;
        candidates[i].score = cosine_similarity(
            query_embedding, 
            index->entries[i].embedding,
            index->dimension
        );
    }
    
    // 2. 使用堆排序获取 Top-K
    qsort(candidates, index->count, sizeof(VectorSearchResult), 
          compare_by_score_desc);
    
    // 3. 返回前 K 个结果
    return candidates;  // 调用者负责释放
}
```

**性能分析**：
- 单次检索：O(N × D)，N=40000, D=768
- 计算时间：~5-10 ms（现代 CPU）
- 内存占用：~122 MB

---

### 阶段 3：混合检索 ⭐⭐⭐

**目标**：融合 BM25 和向量检索结果

**修改文件**：
```
include/search.h                 # 添加混合检索接口
src/search.c                     # 实现混合排序（~100 行）
```

**核心功能**：
```c
// 混合检索
SearchResultSet* hybrid_search(
    SearchEngine* engine,
    VectorIndex* vector_index,
    const char* query,              // 文本查询
    const float* query_embedding,   // 查询向量
    float bm25_weight,              // BM25 权重（如 0.7）
    float vector_weight,            // 向量权重（如 0.3）
    size_t max_results
);
```

**融合策略**：

#### 方法 1：线性加权（简单有效）

```c
// 归一化到 [0, 1]
normalized_bm25_score = bm25_score / max_bm25_score;
normalized_vector_score = vector_score;  // 余弦相似度已在 [0, 1]

// 加权融合
final_score = bm25_weight * normalized_bm25_score + 
              vector_weight * normalized_vector_score;
```

#### 方法 2：倒数排名融合（RRF）

```c
// Reciprocal Rank Fusion
final_score = 1.0 / (k + bm25_rank) + 1.0 / (k + vector_rank);
// k 通常取 60
```

**预期效果**：
- 关键词匹配：BM25 主导
- 语义相似：向量检索主导
- 综合查询：两者互补

---

### 阶段 4：Embedding API 集成 ⭐⭐

**目标**：调用外部 API 进行文本向量化

**新增文件**：
```
include/embedding_api.h          # Embedding API 接口
src/embedding_api.c              # HTTP 调用实现（~200 行）
```

**技术选型**：

#### 方案 A：使用 libcurl（推荐）

```c
// 依赖：libcurl（Ubuntu: sudo apt-get install libcurl4-openssl-dev）

float* call_embedding_api(const char* text, uint32_t* dimension) {
    CURL* curl = curl_easy_init();
    
    // 设置 API 端点
    curl_easy_setopt(curl, CURLOPT_URL, 
                    "http://localhost:11434/api/embeddings");
    
    // 构造 JSON 请求
    char json[4096];
    snprintf(json, sizeof(json), 
            "{\"model\":\"nomic-embed-text\",\"prompt\":\"%s\"}", text);
    
    // 发送请求
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_perform(curl);
    
    // 解析响应（简单 JSON 解析）
    return parse_embedding_response(response);
}
```

#### 方案 B：纯 Socket 实现（零依赖）

```c
// 不依赖任何外部库，纯标准库实现

int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, ...);

// 发送 HTTP POST 请求
const char* request = 
    "POST /api/embeddings HTTP/1.1\r\n"
    "Host: localhost:11434\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "{\"model\":\"nomic-embed-text\",\"prompt\":\"%s\"}";

send(sock, request, strlen(request), 0);
recv(sock, response, sizeof(response), 0);
```

**支持的 API**：
- Ollama（本地部署，推荐）
- OpenAI Embeddings
- HuggingFace Inference API
- 自定义 API

---

## 📊 文件组织结构

### 新增文件清单

```
FusionSearch/
├── include/
│   ├── index_persistence.h    # 索引持久化
│   ├── vector_index.h         # 向量检索
│   └── embedding_api.h        # Embedding API（可选）
├── src/
│   ├── index_persistence.c
│   ├── vector_index.c
│   └── embedding_api.c
├── data/
│   ├── index.bin              # 倒排索引文件
│   └── vectors.bin            # 向量索引文件
├── VECTOR_SEARCH_PLAN.md      # 本文件
└── README.md                  # 更新说明
```

---

## 🎯 使用流程

### 离线：构建索引

```bash
# 1. 准备文档数据
./search_engine --build-index --input recipes.jsonl

# 2. 生成向量（调用 API）
./search_engine --generate-vectors --model nomic-embed-text

# 3. 保存索引
# 自动保存到 data/index.bin 和 data/vectors.bin
```

### 在线：检索查询

```bash
# 1. 启动服务（快速加载索引）
./search_engine

# 输出：
# Loading index from data/index.bin... ✓ (48 ms)
# Loading vectors from data/vectors.bin... ✓ (52 ms)
# Ready for queries!

# 2. 全文检索
> search: 番茄炒蛋
[BM25] Found 15 results

# 3. 向量检索
> vsearch: 简单快手的家常菜
[Vector] Found 20 results

# 4. 混合检索
> hybrid: 适合新手的川菜
[Hybrid] Found 25 results (BM25: 0.7, Vector: 0.3)
```

---

## 📈 性能目标

| 指标 | 目标值 | 实测值（待验证） |
|------|--------|-----------------|
| **索引加载** | < 100 ms | - |
| **向量检索** | < 10 ms | - |
| **BM25 检索** | < 5 ms | ✅ 已达成 |
| **混合检索** | < 15 ms | - |
| **内存占用** | < 200 MB | - |
| **索引大小** | < 150 MB | - |

---

## ⚠️ 注意事项

### 1. 依赖管理

根据用户偏好，**优先使用零依赖方案**：
- ✅ 索引持久化：纯标准库（stdio, stdlib）
- ✅ 向量检索：纯标准库 + math.h
- ⚠️ Embedding API：可选依赖 libcurl
  - 推荐：手动下载预生成的向量文件
  - 备选：纯 socket 实现（零依赖）

### 2. 跨平台兼容性

- ✅ Linux/WSL2：完全支持
- ✅ macOS：完全支持
- ✅ Windows：需要适配文件路径（`\` vs `/`）
- ✅ iOS/Android：支持（需要适配文件 I/O）

### 3. 数据安全

- 使用二进制格式（校验和保护）
- 版本控制（向后兼容）
- 损坏检测和恢复机制

---

## 🔜 未来扩展

### 性能优化

1. **mmap 内存映射**
   - 加载时间：~5 ms（零拷贝）
   - 适合大规模数据（> 100万文档）

2. **SIMD 加速**
   - AVX/SSE 指令集
   - 向量计算加速 4-8 倍

3. **量化压缩**
   - 8-bit 量化：存储减少 75%
   - 精度损失 < 2%

### 高级功能

1. **近似最近邻（ANN）**
   - HNSW 算法
   - 检索速度提升 10-100 倍

2. **多向量支持**
   - 文档分段向量化
   - 细粒度语义匹配

3. **实时更新**
   - 增量索引更新
   - 无需重建整个索引

---

## 📚 参考资源

### 技术文档
- [Faiss: 向量检索库](https://github.com/facebookresearch/faiss)
- [HNSW 算法论文](https://arxiv.org/abs/1603.09320)
- [BM25 算法详解](https://en.wikipedia.org/wiki/Okapi_BM25)

### 最佳实践
- [Hybrid Search 实现指南](https://www.pinecone.io/learn/hybrid-search/)
- [Embedding API 选择](https://huggingface.co/blog/getting-started-with-embeddings)

---

## ✅ 实施检查清单

### 阶段 1：索引持久化
- [ ] 实现 `index_persistence.h/c`
- [ ] 二进制格式设计
- [ ] 保存/加载功能
- [ ] 错误处理和恢复
- [ ] 性能测试（加载时间 < 100ms）

### 阶段 2：向量检索
- [ ] 实现 `vector_index.h/c`
- [ ] 余弦相似度计算
- [ ] Top-K 检索
- [ ] 向量文件持久化
- [ ] 性能测试（检索时间 < 10ms）

### 阶段 3：混合检索
- [ ] 扩展 `search.h/c`
- [ ] 归一化和加权融合
- [ ] 结果去重和排序
- [ ] 性能测试（混合检索 < 15ms）

### 阶段 4：Embedding API（可选）
- [ ] 实现 `embedding_api.h/c`
- [ ] HTTP 客户端（libcurl 或 socket）
- [ ] JSON 解析（简单解析器）
- [ ] 错误处理和重试
- [ ] API 抽象层（支持多种服务）

---

## 🚀 开始实施

建议顺序：
1. ✅ **阶段 1**：索引持久化（立竿见影）
2. ✅ **阶段 2**：向量检索（核心功能）
3. ✅ **阶段 3**：混合检索（增强体验）
4. 🔜 **阶段 4**：Embedding API（按需实现）

预计总开发时间：**3-5 小时**（核心功能）

准备开始了吗？🎉
