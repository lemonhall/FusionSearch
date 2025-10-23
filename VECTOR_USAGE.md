# 向量检索使用指南

本文档介绍如何使用 FusionSearch 的向量检索功能。

---

## 📋 前提条件

1. **API 密钥**：申请 Silicon Flow API 密钥
   - 网址：https://siliconflow.cn
   - 免费额度：足够测试使用

2. **Python 环境**：Python 3.6+
   ```bash
   pip install requests
   ```

3. **C 编译环境**：GCC + Make（WSL/Linux）

---

## 🚀 快速开始

### 步骤 1：设置 API 密钥

**Linux/Mac/WSL:**
```bash
export SILICONFLOW_API_KEY='your-api-key-here'
```

**Windows PowerShell:**
```powershell
$env:SILICONFLOW_API_KEY='your-api-key-here'
```

**Windows CMD:**
```cmd
set SILICONFLOW_API_KEY=your-api-key-here
```

### 步骤 2：测试 API 连接

```bash
python generate_embeddings.py
```

预期输出：
```
============================================================
Silicon Flow Embedding API 测试
============================================================

✓ API密钥已设置: sk-xxxxxx...
✓ 使用模型: BAAI/bge-large-zh-v1.5

测试 1: Silicon flow embedding online...
  ✓ 成功生成向量
  维度: 1024
  前5个值: [0.0529751, 0.029211828, ...]
  耗时: 1.07s

✅ 所有测试通过！API 工作正常
============================================================
```

### 步骤 3：生成向量数据

使用示例数据：

```bash
python generate_embeddings.py
# 输入: example_recipes.csv
```

输出文件：`example_recipes.jsonl`

格式：
```jsonl
{"title": "番茄炒蛋", "content": "简单快手的家常菜...", "embedding": [0.010521737, -0.021317074, ...]}
{"title": "宫保鸡丁", "content": "经典川菜...", "embedding": [0.035585437, -0.02555703, ...]}
```

### 步骤 4：C 程序加载和检索

编译测试程序：

```bash
wsl bash -c "cd /mnt/e/development/FusionSearch && \
  gcc -std=c99 -g -D_POSIX_C_SOURCE=199309L -Iinclude \
  test_hybrid.c src/vector_index.c src/file_loader.c \
  src/index.c src/tokenizer.c src/search.c src/bm25.c \
  src/snippet.c src/utils.c src/cjk_tokenizer.c \
  -o test_hybrid -lm && ./test_hybrid"
```

---

## 📝 数据格式说明

### CSV 输入格式

```csv
title,content
番茄炒蛋,简单快手的家常菜，鸡蛋打散炒熟备用...
宫保鸡丁,经典川菜，鸡肉切丁腌制...
```

### JSONL 输出格式

```jsonl
{
  "title": "番茄炒蛋",
  "content": "简单快手的家常菜，鸡蛋打散炒熟备用，番茄切块翻炒出汁，加入鸡蛋翻炒均匀，加盐调味即可",
  "embedding": [0.010521737, -0.021317074, 0.052388612, ..., 0.010194605]
}
```

**字段说明**：
- `title` - 文档标题（用于显示）
- `content` - 文档内容（用于 BM25 分词索引）
- `embedding` - 向量数据（1024维浮点数数组）

---

## 🔧 API 配置

### 模型选择

在 `generate_embeddings.py` 中修改：

```python
# 1024维，中文优化（推荐）
EMBEDDING_MODEL = "BAAI/bge-large-zh-v1.5"

# 512维，轻量级
# EMBEDDING_MODEL = "BAAI/bge-small-zh-v1.5"

# 1536维，多语言
# EMBEDDING_MODEL = "text-embedding-3-small"
```

### API 调用示例

```python
import requests

def generate_embedding(text, api_key):
    response = requests.post(
        'https://api.siliconflow.cn/v1/embeddings',
        headers={
            'Authorization': f'Bearer {api_key}',
            'Content-Type': 'application/json'
        },
        json={
            'model': 'BAAI/bge-large-zh-v1.5',
            'input': text
        }
    )
    return response.json()['data'][0]['embedding']
```

---

## 💻 C 代码集成

### 完整示例

```c
#include "vector_index.h"
#include "file_loader.h"
#include "index.h"
#include "tokenizer.h"

int main() {
    // 1. 创建索引
    InvertedIndex* bm25_index = index_create();
    VectorIndex* vector_index = vector_index_create(1024);  // 1024维
    Tokenizer* tokenizer = tokenizer_create(NULL);
    
    // 2. 加载 JSONL 文件（自动构建双索引）
    int doc_count = file_loader_load_jsonl_with_vectors(
        "example_recipes.jsonl",
        bm25_index,
        vector_index,
        tokenizer,
        1  // 起始 doc_id
    );
    
    printf("✓ 加载 %d 个文档\n", doc_count);
    
    // 3. BM25 全文检索
    SearchEngine* engine = search_engine_create(bm25_index, tokenizer);
    SearchResultSet* text_results = search_engine_search(
        engine, "川菜", SEARCH_OR, 10
    );
    
    // 4. 向量语义检索
    float query_vector[1024] = {...};  // 从 API 获取查询向量
    size_t count;
    VectorResult* vec_results = vector_search(
        vector_index, query_vector, 10, &count
    );
    
    // 5. 清理
    free(vec_results);
    search_free_results(text_results);
    search_engine_destroy(engine);
    tokenizer_destroy(tokenizer);
    vector_index_free(vector_index);
    index_destroy(bm25_index);
    
    return 0;
}
```

---

## 📊 性能参数

### 4万文档规模（1024维）

| 指标 | 值 |
|------|-----|
| 内存占用 | ~156 MB |
| 查询时间 | ~10-15 ms |
| 索引构建 | ~500 ms |
| 精度 | 100% (暴力检索) |

### 适用场景

- ✅ **< 10万文档** - 暴力检索完全够用
- ✅ **移动端应用** - 内存和性能可接受
- ✅ **离线检索** - 无网络依赖
- ⚠️ **> 50万文档** - 需要考虑 ANN 算法（HNSW/IVF）

---

## 🔍 检索策略

### 纯 BM25 检索（关键词）

```c
SearchResultSet* results = search_engine_search(
    engine, "川菜 麻辣", SEARCH_AND, 10
);
```

适用场景：
- 精确关键词匹配
- 布尔搜索（AND/OR）
- 快速响应（< 1ms）

### 纯向量检索（语义）

```c
VectorResult* results = vector_search(
    vector_index, query_embedding, 10, &count
);
```

适用场景：
- 语义相似搜索
- 跨语言检索
- 模糊匹配

### 混合检索（未来支持）

```c
HybridResult* results = hybrid_search(
    engine, vector_index,
    "川菜",           // 文本查询
    query_embedding,  // 查询向量
    0.7,              // BM25 权重
    0.3,              // 向量权重
    10
);
```

---

## ⚠️ 常见问题

### 1. API 调用失败

**问题**：`HTTP错误: 401`

**解决**：
- 检查 API 密钥是否正确
- 确认环境变量已设置：`echo $SILICONFLOW_API_KEY`

### 2. 向量维度不匹配

**问题**：`Embedding dimension mismatch`

**解决**：
- 确保 C 代码中的维度与模型一致
- `bge-large-zh-v1.5` = 1024维
- `bge-small-zh-v1.5` = 512维

### 3. 内存不足

**问题**：加载大量文档时内存溢出

**解决**：
- 使用较小的模型（512维 vs 1024维）
- 分批处理文档
- 考虑使用量化（未来支持）

### 4. 查询速度慢

**问题**：单次查询 > 100ms

**解决**：
- 暴力检索适用于 < 10万文档
- 超过此规模需要 ANN 算法（HNSW/IVF）
- 参考 `VECTOR_SEARCH_PLAN.md` 优化方案

---

## 📚 参考资源

- **Silicon Flow API 文档**：https://docs.siliconflow.cn
- **BGE 模型介绍**：https://huggingface.co/BAAI/bge-large-zh-v1.5
- **向量检索详细计划**：`VECTOR_SEARCH_PLAN.md`

---

## 🎯 下一步

1. ✅ 测试 API 连接
2. ✅ 生成示例数据
3. 🔜 集成到主程序
4. 🔜 实现混合检索
5. 🔜 性能优化（ANN）

---

**最后更新**: 2025-10-24  
**版本**: 1.0.0
