# 数据文件加载指南

## 📁 数据文件格式

FusionSearch 支持三种常见的数据文件格式：

### 1. TSV 格式 (Tab-Separated Values)

**文件**: `data/documents.tsv`

**格式**: 一行一个文档，用制表符分隔标题和内容
```
title	content
Python Basics	Python is a high-level programming language...
Web Development with Python	Python has excellent frameworks...
```

**特点**:
- ✅ 简单易用
- ✅ 易于手工编辑
- ✅ 支持制表符分隔
- ✅ 注释行以 # 开头

**示例行**:
```
Python Programming Guide	Python is a high-level programming language created by Guido van Rossum in 1991.
```

### 2. CSV 格式 (Comma-Separated Values)

**文件**: `data/documents.csv`

**格式**: 标准 CSV 格式，带可选的引号和转义
```
title,content
"Python Basics","Python is a high-level programming language..."
"Web Development","Python frameworks like Django and Flask..."
```

**特点**:
- ✅ 业界标准格式
- ✅ 支持引号包围的字段
- ✅ 支持转义序列
- ✅ Excel/Google Sheets 兼容

**示例行**:
```
"Reactive Programming","Reactive programming is a programming paradigm focused on data flows..."
```

### 3. JSON Lines 格式 (.jsonl)

**文件**: `data/documents.jsonl`

**格式**: 每行一个 JSON 对象
```json
{"title": "Python Basics", "content": "Python is a high-level programming language..."}
{"title": "Web Development", "content": "Python has excellent frameworks..."}
```

**特点**:
- ✅ 结构化格式
- ✅ 支持嵌套数据
- ✅ 易于 Python/JavaScript 处理
- ✅ 支持转义序列

**示例行**:
```json
{"title": "Blockchain Technology", "content": "Blockchain is a distributed ledger technology that records transactions..."}
```

## 🚀 使用方法

### 方法 1: 运行程序时加载

```bash
./search_engine
```

在菜单中选择选项 5:
```
=== Search Engine Menu ===
1. Perform AND search (all terms must match)
2. Perform OR search (any term matches)
3. Perform BM25 search (relevance ranking)
4. Perform PHRASE search (exact phrase match)
5. Load documents from file
6. View index statistics
7. View dictionary contents
8. Exit
Enter your choice: 5
```

### 方法 2: 选择文件格式

程序会提示选择文件格式:
```
=== Load Documents from File ===
File format options:
1. TSV (Tab-separated: title\tcontent)
2. CSV (Comma-separated: title,content)
3. JSONL (JSON Lines: {"title": "...", "content": "..."})

Enter format (1-3): 1
```

### 方法 3: 输入文件路径

```
Enter file path (or press Enter for default): data/documents.tsv
```

默认路径:
- TSV: `data/documents.tsv`
- CSV: `data/documents.csv`
- JSONL: `data/documents.jsonl`

## 📊 加载结果

加载完成后，程序会显示:
```
✅ Successfully loaded 20 documents

Index Statistics:
  Total documents: 20
  Total unique words: 245
  Average words per document: 18
  Largest document: 25 words
```

## 📝 创建自己的数据文件

### TSV 格式模板

```
# My Documents Database
# Format: title\tcontent

Document 1	This is the content of document 1 with keywords...
Document 2	This is the content of document 2 with keywords...
```

### CSV 格式模板

```
title,content
"Document 1","This is the content of document 1 with keywords..."
"Document 2","This is the content of document 2 with keywords..."
```

### JSON Lines 格式模板

```json
{"title": "Document 1", "content": "This is the content of document 1 with keywords..."}
{"title": "Document 2", "content": "This is the content of document 2 with keywords..."}
```

## ⚙️ 文件加载配置

### 最大限制

| 限制 | 值 |
|------|-----|
| 单行最大长度 | 10,000 字符 |
| 最大文档数 | 10,000 |
| 最大单词数 | 100,000 |
| 单个字段最大长度 | 5,000 字符 |

### 性能指标

| 操作 | 时间 |
|------|------|
| 加载 20 个文档 | < 50 ms |
| 加载 100 个文档 | < 200 ms |
| 加载 1000 个文档 | < 2 s |

## 🔧 错误处理

### 常见错误

**错误**: "Cannot open file"
```
解决: 检查文件路径是否正确，文件是否存在
```

**错误**: "No documents loaded from file"
```
解决: 检查文件格式是否正确，是否有有效的文档行
```

**错误**: 非 UTF-8 编码
```
解决: 转换文件为 UTF-8 编码再重新加载
```

## 📚 内置示例文件

### 1. documents.tsv (20 个文档)

包含以下主题:
- Python 基础和高级
- Web 开发
- JavaScript
- 数据科学
- C 语言
- 数据库
- 移动开发
- 云计算
- 机器学习
- DevOps

### 2. documents.csv (15 个文档)

包含以下主题:
- 响应式编程
- 函数式编程
- 面向对象设计
- 微服务架构
- 容器技术
- Kubernetes
- GraphQL
- REST API
- 事件驱动架构
- NoSQL 数据库

### 3. documents.jsonl (10 个文档)

包含以下主题:
- 区块链技术
- 智能合约
- 自然语言处理
- 计算机视觉
- 深度学习
- 迁移学习
- 模型部署
- 大数据处理
- 流处理
- 数据仓库

## 💡 使用场景

### 场景 1: 快速测试

使用内置示例文件快速测试搜索功能:
```
./search_engine
选择菜单 5 -> 使用默认文件 -> 搜索
```

### 场景 2: 加载大数据集

从 CSV 导出文件加载大量文档:
```
1. 从数据库导出为 CSV
2. 运行 search_engine
3. 选择菜单 5 -> 选择 CSV 格式
4. 输入 CSV 文件路径
```

### 场景 3: 实时索引更新

使用 JSON Lines 格式支持流式加载:
```
1. 准备 .jsonl 文件
2. 运行 search_engine
3. 选择菜单 5 -> 选择 JSONL 格式
4. 加载文件并搜索
```

## 🎯 最佳实践

1. **文件编码** - 使用 UTF-8 编码
2. **转义序列** - CSV 中的引号使用 `""` 转义
3. **特殊字符** - 避免在标题中使用制表符/逗号
4. **文件大小** - 单个文件 < 100 MB 推荐
5. **备份** - 加载前备份原始数据文件

## 📖 技术细节

### 文件加载流程

```
输入: 文件路径 + 文件格式
  |
  v
1. 打开文件并读取
  |
  v
2. 按格式解析每一行
  |
  v
3. 提取标题和内容
  |
  v
4. 对内容进行分词
  |
  v
5. 添加到倒排索引
  |
  v
输出: 加载的文档数量
```

### 性能优化

- 使用流式读取减少内存占用
- 批量索引构建
- 增量加载支持
- 缓存分词结果

---

**最后更新**: 2025-10-24  
**版本**: v0.6.0 (文件加载功能)
