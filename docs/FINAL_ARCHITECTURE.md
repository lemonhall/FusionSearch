# 混合搜索引擎 - 最终架构文档

## 🎯 架构设计

### 职责分工

```
┌─────────────────────────────────────────────────────────┐
│  Python/Swift/Kotlin（高级语言调度层）                   │
├─────────────────────────────────────────────────────────┤
│  1. 调用API获取Query向量 → [float数组]                   │
│  2. 调用C函数：ffi_bm25_search(query)                    │
│      └─ 返回：[(doc_id, score), ...]                    │
│  3. 调用C函数：ffi_vector_search(query_vector)           │
│      └─ 返回：[(doc_id, similarity), ...]               │
│  4. 调用C函数：ffi_get_document(doc_id)                  │
│      └─ 返回：{title, content}                          │
│  5. 融合排序并展示结果                                   │
└─────────────────────────────────────────────────────────┘
                            ↕ FFI调用
┌─────────────────────────────────────────────────────────┐
│  C语言（核心检索引擎）                                    │
├─────────────────────────────────────────────────────────┤
│  ✓ 文档加载（JSONL格式）                                 │
│  ✓ 分词器（英文+CJK）                                    │
│  ✓ BM25索引构建和检索                                    │
│  ✓ 向量索引加载（二进制文件）                             │
│  ✓ 向量余弦相似度检索                                    │
│  ✓ 文档元数据查询                                        │
└─────────────────────────────────────────────────────────┘
```

### 关键设计理念

**C不返回向量，只返回 (doc_id, score)**  
- 节省内存
- 简化FFI调用
- 高级语言通过doc_id查询文档内容

## 📋 C FFI导出接口

### 1. 索引加载

```c
// 加载BM25索引（从JSONL文件）
uintptr_t ffi_index_load(const char* jsonl_file);

// 加载向量索引（从二进制文件）
uintptr_t ffi_vector_index_load(const char* vector_file);
```

### 2. 搜索接口

```c
// BM25搜索
size_t ffi_bm25_search(
    uintptr_t index_ptr,
    const char* query,
    uint32_t k,
    uint32_t* out_doc_ids,    // 输出：文档ID数组
    float* out_scores          // 输出：BM25分数数组
);

// 向量检索
size_t ffi_vector_search(
    uintptr_t index_ptr,
    const float* query_embedding,
    uint32_t dimension,
    uint32_t k,
    uint32_t* out_doc_ids,    // 输出：文档ID数组
    float* out_scores          // 输出：相似度数组
);
```

### 3. 文档查询

```c
// 根据doc_id获取文档内容
int ffi_get_document(
    uintptr_t index_ptr,
    uint32_t doc_id,
    char* out_title,          // 输出：标题缓冲区
    size_t title_size,
    char* out_content,        // 输出：内容缓冲区
    size_t content_size
);
```

### 4. 资源释放

```c
void ffi_index_free(uintptr_t index_ptr);
void ffi_vector_index_free(uintptr_t index_ptr);
```

## 🚀 使用流程

### 步骤1：准备数据

```bash
# 编译演示程序
make demo

# 加载JSONL，导出向量索引
wsl bash -c "./demo_hybrid recipes_with_embeddings.jsonl vectors.bin documents.json"
```

生成文件：
- `vectors.bin` - 向量索引（C导出）
- `documents.json` - 文档元数据（可选）

### 步骤2：编译共享库

```bash
# 编译FFI共享库
make lib
```

生成：`libfusion.so` (Linux) 或 `fusion.dll` (Windows)

### 步骤3：Python调用

```bash
python fusion_search.py
```

## 💻 Python示例代码

```python
from fusion_search import FusionSearchEngine

# 初始化引擎
engine = FusionSearchEngine(
    jsonl_file="recipes_with_embeddings.jsonl",
    vector_file="vectors.bin",
    lib_path="./libfusion.so"
)

# 执行混合搜索
results = engine.search(
    query="鱼香肉丝怎么做",
    top_k=10,
    use_bm25=True,       # 使用BM25
    use_vector=True,     # 使用向量检索
    bm25_weight=0.5,     # BM25权重
    vector_weight=0.5    # 向量权重
)

# 显示结果
for i, result in enumerate(results, 1):
    print(f"{i}. {result['title']}")
    print(f"   Final: {result['final_score']:.4f}")
    print(f"   BM25: {result['bm25_score']:.4f}")
    print(f"   Vector: {result['vector_score']:.4f}")
    print()
```

## 📱 iOS/Android集成

### iOS (Swift)

```swift
// 1. 调用API获取Query向量
func getQueryVector(query: String) async -> [Float] {
    let embedding = await callSiliconFlowAPI(query)
    return embedding
}

// 2. 调用C函数（通过桥接）
let docIds = UnsafeMutablePointer<UInt32>.allocate(capacity: 10)
let scores = UnsafeMutablePointer<Float>.allocate(capacity: 10)

// BM25搜索
let bm25Count = ffi_bm25_search(indexPtr, query, 10, docIds, scores)

// 向量检索
let queryVector = await getQueryVector(query: "鱼香肉丝")
let vectorCount = ffi_vector_search(
    vectorIndexPtr,
    queryVector,
    1024,
    10,
    docIds,
    scores
)

// 3. 查询文档内容
let titleBuffer = UnsafeMutablePointer<CChar>.allocate(capacity: 256)
let contentBuffer = UnsafeMutablePointer<CChar>.allocate(capacity: 2048)

ffi_get_document(indexPtr, docId, titleBuffer, 256, contentBuffer, 2048)

let title = String(cString: titleBuffer)
let content = String(cString: contentBuffer)

// 4. 融合排序（Swift代码）
let fusedResults = fuseResults(bm25Results, vectorResults)
```

### Android (Kotlin)

```kotlin
// 1. 调用API获取Query向量
suspend fun getQueryVector(query: String): FloatArray {
    val embedding = callSiliconFlowAPI(query)
    return embedding
}

// 2. 调用C函数（通过JNI）
external fun ffiBm25Search(
    indexPtr: Long,
    query: String,
    k: Int,
    outDocIds: IntArray,
    outScores: FloatArray
): Int

external fun ffiVectorSearch(
    indexPtr: Long,
    queryEmbedding: FloatArray,
    dimension: Int,
    k: Int,
    outDocIds: IntArray,
    outScores: FloatArray
): Int

external fun ffiGetDocument(
    indexPtr: Long,
    docId: Int
): Pair<String, String>  // (title, content)

// 3. 使用示例
val queryVector = getQueryVector("鱼香肉丝")

val bm25DocIds = IntArray(10)
val bm25Scores = FloatArray(10)
val bm25Count = ffiBm25Search(indexPtr, query, 10, bm25DocIds, bm25Scores)

val vectorDocIds = IntArray(10)
val vectorScores = FloatArray(10)
val vectorCount = ffiVectorSearch(
    vectorIndexPtr,
    queryVector,
    1024,
    10,
    vectorDocIds,
    vectorScores
)

// 4. 融合排序
val fusedResults = fuseResults(bm25Results, vectorResults)
```

## 🔧 编译配置

### Makefile目标

```makefile
# 编译共享库
make lib

# 编译演示程序
make demo

# 运行演示
wsl bash -c "./demo_hybrid"
```

### 交叉编译（移动端）

```bash
# iOS
xcrun clang -arch arm64 -isysroot $(xcrun --sdk iphoneos --show-sdk-path) \
    -shared -fPIC src/*.c -o libfusion.a

# Android
$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang \
    -shared -fPIC src/*.c -o libfusion.so
```

## 📊 性能优化

### C端优化

- ✅ 二进制格式存储向量（比JSON小10倍）
- ✅ 暴力检索足够快（<10万文档，<100ms）
- ✅ 零拷贝FFI接口（直接传递指针）

### Python端优化

- ✅ 使用ctypes（Python内置，无额外依赖）
- ✅ 批量查询文档内容
- ✅ 缓存API结果（避免重复调用）

## 📁 文件清单

| 文件 | 用途 |
|------|------|
| `include/vector_index.h` | 向量检索头文件（含FFI接口） |
| `include/search.h` | BM25搜索头文件（含FFI接口） |
| `src/vector_index.c` | 向量检索实现（含FFI实现） |
| `src/search.c` | BM25搜索实现（含FFI实现） |
| `fusion_search.py` | **Python调度层**（推荐使用） |
| `demo_hybrid.c` | C数据准备程序 |
| `Makefile` | 编译脚本（含lib目标） |

## 🎯 关键优势

1. **职责清晰**：C做检索，Python/Swift/Kotlin做调度
2. **零拷贝**：只返回doc_id和score，不传递大数据
3. **轻量级**：C无外部依赖，Python只用ctypes+requests
4. **移动端友好**：高级语言调用API，C只做本地检索
5. **易于扩展**：FFI接口简单，任何语言都能调用

## 🔜 下一步

1. **测试**：运行 `python fusion_search.py` 验证功能
2. **移动端**：按照示例集成到iOS/Android
3. **优化**：根据实际场景调整权重和top_k
4. **部署**：打包C库和索引文件到生产环境

完美！🎉
