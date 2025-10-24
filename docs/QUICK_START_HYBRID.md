# 混合搜索引擎 - 快速开始指南

## 🎯 核心概念

**Python调用外部API获取Query向量 → C执行BM25和向量检索 → Python融合排序**

## 📋 完整流程

### 步骤1：生成向量数据

```bash
# 设置API密钥
export SILICONFLOW_API_KEY='your-api-key'

# 生成带向量的JSONL文件
python build_vector_index.py
```

生成文件：`recipes_with_embeddings.jsonl`

### 步骤2：C程序加载数据并导出

```bash
# 编译演示程序
make demo

# 运行（加载JSONL，导出向量索引）
wsl bash -c "cd /mnt/e/development/FusionSearch && ./demo_hybrid"
```

生成文件：
- `vectors.bin` - 向量索引（二进制）
- `documents.json` - 文档元数据

### 步骤3：Python执行搜索

```bash
# 运行搜索引擎
python search_api.py
```

输入查询，系统自动：
1. 调用API获取Query向量
2. 读取vectors.bin执行向量检索
3. 返回融合排序结果

## 💻 编程接口

### Python使用示例

```python
from search_api import HybridSearchEngine

# 初始化引擎
engine = HybridSearchEngine()

# 执行搜索
results = engine.search(
    query="鱼香肉丝",
    top_k=10,
    use_vector=True,  # 使用向量检索
    use_bm25=False    # 不使用BM25（未实现）
)

# 显示结果
for i, result in enumerate(results, 1):
    print(f"{i}. {result['title']}")
    print(f"   Score: {result['final_score']:.4f}")
    print(f"   Vector: {result['vector_score']:.4f}")
```

## 🚀 一键脚本（Windows）

创建 `run_hybrid_search.bat`:

```batch
@echo off
echo ========================================
echo 混合搜索引擎 - 一键启动
echo ========================================
echo.

echo [1/3] 编译C程序...
wsl bash -c "cd /mnt/e/development/FusionSearch && make demo"

echo.
echo [2/3] 加载数据并导出向量...
wsl bash -c "cd /mnt/e/development/FusionSearch && ./demo_hybrid recipes_with_embeddings.jsonl vectors.bin documents.json"

echo.
echo [3/3] 启动Python搜索引擎...
python search_api.py

pause
```

## 📁 文件说明

| 文件 | 用途 | 生成方式 |
|------|------|----------|
| `recipes_with_embeddings.jsonl` | 原始向量数据 | Python生成 |
| `vectors.bin` | 向量索引（紧凑） | C导出 |
| `documents.json` | 文档元数据 | C导出 |
| `search_api.py` | Python搜索引擎 | 手动编写 |
| `demo_hybrid.c` | C数据准备程序 | 手动编写 |

## 🔍 技术细节

### 为什么这样设计？

1. **API调用在Python** - 移动端（iOS/Android）的HTTP限制
2. **向量检索在Python** - 简化开发，numpy加速
3. **BM25在C** - 性能考虑（未来可能集成）
4. **文件交换数据** - 最简单可靠的跨语言方案

### 移动端怎么办？

```
iOS (Swift)                        Android (Kotlin)
    │                                    │
    ├─ 调用API获取Query向量              ├─ 调用API获取Query向量
    ├─ 读取vectors.bin                  ├─ 读取vectors.bin
    ├─ 调用C向量检索（通过JNI/FFI）      ├─ 调用C向量检索（通过JNI）
    └─ 融合排序返回结果                 └─ 融合排序返回结果
```

## ⚙️ 配置选项

### 修改向量维度

`demo_hybrid.c`:
```c
VectorIndex* vectorIndex = vector_index_create(768);  // 改为768维
```

### 修改API模型

`search_api.py`:
```python
self.model = "BAAI/bge-small-zh-v1.5"  # 使用小模型（512维）
```

## 🐛 常见问题

**Q: vectors.bin文件不存在？**  
A: 运行 `./demo_hybrid` 生成

**Q: API调用失败？**  
A: 检查环境变量 `SILICONFLOW_API_KEY`

**Q: 向量维度不匹配？**  
A: 确保Python模型和C创建索引时的维度一致

**Q: 搜索结果为空？**  
A: 确认向量数据已正确加载，检查文档数量

## 📚 相关文档

- [完整技术指南](HYBRID_SEARCH_GUIDE.md)
- [向量索引详解](README_VECTOR.md)
- [API文档](generate_embeddings.py)
