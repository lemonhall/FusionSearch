# 向量检索快速开始指南

## 🚀 完整工作流程

### 步骤 1：提取菜谱数据

```bash
python prepare_recipes.py
```

**功能**：
- 读取 `recipes/*.json` 文件（50个）
- 提取 `name` 和 `content` 字段
- 生成 `data/recipes.jsonl`（每行一个JSON对象，避免CSV多行问题）

**输出**：
```
✓ 成功提取 50 个菜谱
💾 写入 JSONL: data/recipes.jsonl
📊 文档数量: 50
```

---

### 步骤 2：生成向量索引

**设置 API 密钥**：
```bash
# Linux/Mac/WSL
export SILICONFLOW_API_KEY='your-api-key-here'

# Windows PowerShell
$env:SILICONFLOW_API_KEY='your-api-key-here'
```

**运行**：
```bash
python build_vector_index.py
```

**功能**：
- 读取 `data/recipes.jsonl`
- 调用 Silicon Flow API 生成 embedding（1024维）
- 自动截断超长文本（>2000字符）
- 生成 `data/recipes_vector.jsonl`（带向量）

**输出**：
```
[1/50] 普通白色清汤            [截断] ✓ 0.7s
[2/50] 澄清清汤制备法          ✓ 0.6s
...
✅ 构建完成！
   成功: 50
   失败: 0
   输出: data/recipes_vector.jsonl
```

**注意**：
- 每次API调用约0.6-0.8秒
- 50个文档约需40-50秒
- 自动处理API限流（0.1秒延迟）

---

### 步骤 3A：Python 交互式测试

```bash
python query_test.py
```

**功能**：
- 加载向量索引
- 用户输入查询
- 自动调用 API 生成查询向量
- 计算余弦相似度
- 返回 Top-5 结果

**示例**：
```
🔍 请输入查询: 如何制作清汤

🔍 生成查询向量...
✓ 向量维度: 1024

🎯 检索结果（Top-5）：
────────────────────────────────────────

#1  相似度: 0.9234
    标题: 普通白色清汤
    内容: 普通白色清汤制作4夸脱[4546毫升]用量...

#2  相似度: 0.8765
    标题: 澄清清汤制备法
    内容: 澄清清汤制备法（用于清汤）4夸脱...
```

---

### 步骤 3B：C 程序测试（未来）

编译并运行 C 程序：

```bash
wsl bash -c "cd /mnt/e/development/FusionSearch && make && ./search_engine"
```

在菜单中选择向量检索选项。

---

## 📁 文件说明

### 数据文件

```
recipes/
├── 0001.json ... 0050.json    # 原始菜谱JSON（50个）

data/
├── recipes.jsonl              # 提取的纯文本（步骤1生成）
└── recipes_vector.jsonl       # 带向量的JSONL（步骤2生成）
```

### 工具脚本

```
prepare_recipes.py             # 步骤1：提取数据
build_vector_index.py          # 步骤2：生成向量
query_test.py                  # 步骤3：交互式测试
```

---

## 🎯 数据格式

### recipes.jsonl（步骤1输出）

```jsonl
{"title": "普通白色清汤", "content": "普通白色清汤\n\n制作4夸脱..."}
{"title": "澄清清汤制备法", "content": "澄清清汤制备法..."}
```

### recipes_vector.jsonl（步骤2输出）

```jsonl
{
  "title": "普通白色清汤",
  "content": "普通白色清汤\n\n制作4夸脱...",
  "embedding": [0.0529751, 0.029211828, -0.025287248, ...]
}
```

**字段说明**：
- `title` - 菜谱名称
- `content` - 菜谱内容（完整文本，用于 BM25 索引）
- `embedding` - 1024维向量（Silicon Flow API 生成）

---

## ⚠️ 常见问题

### 1. CSV 多行问题（已修复）

**问题**：菜谱内容包含换行符，CSV 格式无法正确处理

**解决**：改用 JSONL 格式（每行一个完整的 JSON 对象）

### 2. API 文本长度限制

**问题**：某些菜谱内容超过 2000 字符，导致 `413 Request Entity Too Large`

**解决**：自动截断到 2000 字符（在 `build_vector_index.py` 中实现）

### 3. API 密钥未设置

**问题**：`❌ 错误：未设置 API 密钥`

**解决**：
```bash
export SILICONFLOW_API_KEY='sk-your-key-here'
```

### 4. numpy 依赖缺失

**问题**：`query_test.py` 需要 numpy

**解决**：
```bash
pip install numpy requests
```

---

## 📊 性能数据

### API 调用性能（Silicon Flow）

| 指标 | 值 |
|------|-----|
| 单次调用延迟 | ~0.6-0.8s |
| 向量维度 | 1024 |
| 最大输入长度 | ~2000字符 |
| 50个文档总时间 | ~40-50s |

### 本地检索性能（估算）

| 指标 | 值 |
|------|-----|
| 50文档查询 | < 1ms |
| 内存占用 | ~0.2 MB |
| 精度 | 100%（暴力检索） |

---

## 🔜 下一步

1. **C 程序集成**：
   - 修改 `main.c` 添加向量检索菜单
   - 加载 `data/recipes_vector.jsonl`
   - 实现交互式查询

2. **混合检索**：
   - BM25 + 向量加权融合
   - 线性组合或 RRF 策略

3. **性能优化**（可选）：
   - ANN 算法（HNSW/IVF）
   - 向量量化（PQ）

---

**创建时间**: 2025-10-24  
**数据规模**: 50个菜谱  
**向量维度**: 1024  
**API**: Silicon Flow (BAAI/bge-large-zh-v1.5)
