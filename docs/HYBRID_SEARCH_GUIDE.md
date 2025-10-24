# 混合搜索引擎使用指南

## 架构设计

### 🎯 核心理念

**职责分离，各司其职**：
- **C语言负责**：文档加载、分词、BM25索引构建、向量数据管理
- **Python负责**：Query向量化（调用API）、向量检索、混合排序
- **数据交换**：通过二进制文件（`vectors.bin`）或 JSON

### 📊 数据流

```
用户Query
    │
    ├──> Python: 调用API获取Query向量
    │         │
    │         └──> Silicon Flow API: 返回1024维向量
    │
    ├──> Python: 读取C生成的向量索引文件 (vectors.bin)
    │         │
    │         └──> 暴力余弦相似度检索 → Top-K结果
    │
    ├──> Python: 调用C程序执行BM25搜索（可选，待实现）
    │         │
    │         └──> BM25排序 → Top-K结果
    │
    └──> Python: 融合排序
              │
              └──> 返回最终结果
```

## 🚀 快速开始

### 1. 环境准备

```bash
# 安装Python依赖
pip install requests numpy

# 设置API密钥
export SILICONFLOW_API_KEY='your-api-key-here'

# 编译C程序
make clean
make
```

### 2. 准备数据

#### 2.1 生成文档向量

```bash
# 使用现有的工具生成向量
python generate_embeddings.py
# 或
python build_vector_index.py
```

这会生成两个文件：
- `recipes_with_embeddings.jsonl` - 带向量的文档数据
- `documents.json` - 文档元数据（可选）

#### 2.2 C程序加载向量并导出

修改 `main.c`，添加向量导出功能：

```c
// 在加载完向量后，导出为二进制文件
if (vectorIndex) {
    vector_index_save(vectorIndex, "vectors.bin");
}
```

### 3. 运行搜索

```bash
# 独立运行Python搜索引擎
python search_api.py
```

输入搜索内容，系统会：
1. 调用API获取Query向量
2. 从 `vectors.bin` 加载向量索引
3. 执行向量检索
4. 返回结果

## 📝 技术细节

### 向量文件格式 (`vectors.bin`)

二进制格式，紧凑高效：

```
[4 bytes] count      - 向量数量 (uint32)
[4 bytes] dimension  - 向量维度 (uint32)

对每个向量：
  [4 bytes] doc_id   - 文档ID (uint32)
  [dimension * 4 bytes] embedding - 向量数据 (float[])
```

### Python加载示例

```python
from search_api import VectorIndex

# 加载向量索引
index = VectorIndex("vectors.bin")

# 执行检索
results = index.search(query_embedding, top_k=10)
# 返回: [(doc_id, similarity), ...]
```

### C导出示例

```c
#include "vector_index.h"

VectorIndex* index = vector_index_create(1024);

// 添加向量...
vector_index_add(index, doc_id, embedding);

// 导出到文件
vector_index_save(index, "vectors.bin");
```

## 🔧 集成方案

### 方案 1：独立运行（推荐）

**优点**：简单、解耦、易于调试  
**适用场景**：开发测试、小规模部署

```bash
# C程序负责索引构建和向量导出
./search_engine  # 加载文档，导出vectors.bin

# Python负责搜索服务
python search_api.py  # 读取vectors.bin，提供搜索
```

### 方案 2：C调用Python（子进程）

**优点**：统一入口  
**适用场景**：C为主程序

```c
// 在C中调用Python
system("python3 -c \"from search_api import get_query_vector; print(get_query_vector('鱼香肉丝'))\"");
```

### 方案 3：Python调用C（ctypes/FFI）

**优点**：性能更好  
**适用场景**：Python为主程序（见 `hybrid_search.py`）

```python
import ctypes

lib = ctypes.CDLL("./libfusion.so")
# 调用C函数...
```

### 方案 4：HTTP服务

**优点**：跨语言、跨平台  
**适用场景**：生产环境

```bash
# Python提供REST API
python flask_server.py

# C程序通过HTTP调用
curl http://localhost:5000/search?q=鱼香肉丝
```

## 📱 移动端集成

### iOS (Swift)

```swift
// Swift负责API调用
func getQueryEmbedding(query: String) async -> [Float] {
    // 调用Silicon Flow API
    let embedding = await callAPI(query)
    
    // 传递给C进行向量检索
    let results = vector_search(vectorIndex, embedding, 10)
    return results
}
```

### Android (Kotlin/Java)

```kotlin
// Kotlin负责API调用
suspend fun getQueryEmbedding(query: String): FloatArray {
    val embedding = apiClient.getEmbedding(query)
    
    // 调用JNI/NDK中的C函数
    return nativeVectorSearch(embedding, 10)
}
```

## 🎨 混合排序策略

### 加权求和

```python
final_score = bm25_weight * norm(bm25_score) + vector_weight * vector_score
```

### 参数建议

- **纯向量检索**：`bm25_weight=0, vector_weight=1`
- **纯BM25**：`bm25_weight=1, vector_weight=0`
- **混合平衡**：`bm25_weight=0.5, vector_weight=0.5`
- **侧重语义**：`bm25_weight=0.3, vector_weight=0.7`
- **侧重关键词**：`bm25_weight=0.7, vector_weight=0.3`

## 🐛 调试技巧

### 检查向量文件

```bash
# 查看文件大小
ls -lh vectors.bin

# 计算向量数量
# size = 8 + count * (4 + dimension * 4)
# 例如：50个文档，1024维
# size = 8 + 50 * (4 + 1024 * 4) = 204808 bytes ≈ 200KB
```

### 验证向量加载

```python
from search_api import VectorIndex

index = VectorIndex("vectors.bin")
print(f"Loaded {len(index.vectors)} vectors")
print(f"Dimension: {index.dimension}")
```

### 测试API连接

```python
from search_api import EmbeddingAPI

api = EmbeddingAPI()
vec = api.get_embedding("测试文本")
print(f"Vector dimension: {len(vec)}")
```

## 📦 文件清单

### C代码
- `include/vector_index.h` - 向量索引接口
- `src/vector_index.c` - 向量索引实现（含导出/加载）
- `src/main.c` - 主程序（需添加导出调用）

### Python代码
- `search_api.py` - **独立搜索引擎**（推荐使用）
- `hybrid_search.py` - FFI封装（高级用法）
- `generate_embeddings.py` - 向量生成工具

### 数据文件
- `vectors.bin` - 向量索引（C导出，Python读取）
- `documents.json` - 文档元数据（可选）
- `recipes_with_embeddings.jsonl` - 原始向量数据

## ⚡ 性能优化

### C端
- ✅ 使用二进制格式（比JSON小10倍）
- ✅ 单次文件写入（避免频繁I/O）
- ⏳ 可考虑压缩（gzip）

### Python端
- ✅ 使用numpy加速向量计算
- ✅ 批量检索（一次加载索引）
- ⏳ 可考虑Cython加速关键路径

## 🔜 待办事项

- [ ] C程序添加命令行参数支持JSON输出（BM25结果）
- [ ] Python添加缓存机制（避免重复API调用）
- [ ] 添加Flask/FastAPI封装，提供REST API
- [ ] 添加批量搜索支持
- [ ] 添加搜索日志和分析

## 💡 常见问题

**Q: 为什么不在C中调用HTTP API？**  
A: C调用HTTP需要额外库（libcurl），且在iOS/Android上受限。Python/Swift/Kotlin调用API更简单。

**Q: vectors.bin文件很大怎么办？**  
A: 可以使用gzip压缩，或者考虑量化（float32→float16）。

**Q: 能否不使用文件交换数据？**  
A: 可以，使用共享内存或Socket，但会增加复杂度。文件方式最简单可靠。

**Q: 移动端如何处理API调用？**  
A: 移动端使用原生HTTP客户端（URLSession/OkHttp），C只负责向量检索。

## 📚 参考资料

- [ctypes官方文档](https://docs.python.org/3/library/ctypes.html)
- [Silicon Flow API文档](https://docs.siliconflow.cn/)
- [BM25算法详解](https://en.wikipedia.org/wiki/Okapi_BM25)
- [余弦相似度](https://en.wikipedia.org/wiki/Cosine_similarity)
