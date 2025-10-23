# FusionSearch - 跨平台搜索引擎 (C 实现)

一个轻量级的全文搜索引擎 C 实现，设计用于 iOS 和 Android 跨平台部署。

## 🎯 项目目标

构建一个支持**中英文分词**和**全文检索**的搜索引擎，目标平台：iOS/Android。

**实现路线**：
1. ✅ **英文版本** - 完成基础框架（当前）
2. 🔜 **中文版本** - 集成 Jieba 分词
3. 🔜 **性能优化** - 实现 BM25 排序
4. 🔜 **跨平台编译** - iOS/Android 集成

---

## 📁 项目结构

```
FusionSearch/
├── include/              # 头文件（接口定义）
│   ├── trie.h           # Trie 字典树
│   ├── tokenizer.h      # 分词器
│   ├── index.h          # 倒排索引
│   ├── search.h         # 搜索引擎
│   ├── bm25.h           # BM25 排序算法
│   ├── snippet.h        # Snippet 生成 + 高亮
│   ├── file_loader.h    # 文件加载（新增）
│   ├── test.h           # 单元测试框架
│   └── utils.h          # 工具函数
├── src/                 # 源文件（实现）
│   ├── main.c           # 主程序 + 菜单
│   ├── trie.c           # Trie 实现 (~150行)
│   ├── tokenizer.c      # 分词实现 (~100行)
│   ├── index.c          # 倒排索引实现 (~150行)
│   ├── search.c         # 搜索引擎实现 (~400行)
│   ├── bm25.c           # BM25 实现 (~100行)
│   ├── snippet.c        # Snippet 实现 (~150行)
│   ├── file_loader.c    # 文件加载实现 (~250行，新增）
│   ├── test.c           # 测试框架实现
│   ├── test_suite.c     # 单元测试集合
│   └── utils.c          # 工具函数 (~250行)
├── data/                # 数据文件（新增）
│   ├── documents.tsv    # TSV 格式文档 (20 个)
│   ├── documents.csv    # CSV 格式文档 (15 个)
│   └── documents.jsonl  # JSON Lines 格式文档 (10 个)
├── .gitignore           # Git 忽略配置
├── Makefile             # 编译配置
├── CHANGELOG.md         # 版本变更记录
├── README.md            # 本文件
├── FILE_LOADING.md      # 文件加载指南（新增）
├── SNIPPET_GUIDE.md     # Snippet 详细说明
└── FEATURES_SUMMARY.md  # 功能总结
```

---

## 🏗️ 核心模块

| 模块 | 用途 | 状态 |
|------|------|------|
| **Trie** | 字典树 + 词频存储 | ✅ 完成 |
| **Tokenizer** | 英文分词（空格/标点） | ✅ 完成 |
| **Index** | 倒排索引（词->文档） | ✅ 完成 |
| **Search** | AND/OR/BM25 查询处理 | ✅ 完成 |
| **BM25** | BM25 相关性排序算法 | ✅ 完成 |
| **Snippet** | Snippet 生成 + 关键词高亮 | ✅ 完成 |
| **Test** | 单元测试框架 | ✅ 完成 |
| **Utils** | 字符串/文件/内存工具 | ✅ 完成 |

---

## 🚀 快速开始

### 编译（Windows - MinGW）
```powershell
mingw32-make
```

### 编译（Linux/Mac）
```bash
make
```

### 运行
```bash
./search_engine
# 或
make run
```

### 清理
```bash
make clean
```

---

## ✨ 功能进度

### 已完成 ✅
- [x] Trie 字典树完整实现
- [x] 英文分词器（空格/标点分割）
- [x] 倒排索引框架 + 文档存储
- [x] AND/OR 搜索逻辑实现
- [x] **TF-IDF 排序算法** ⭐
- [x] **BM25 排序算法** ⭐⭐ 
- [x] 菜单驱动的交互界面
- [x] 5 个样本文档自动加载
- [x] 搜索结果展示（包含标题、内容片段）
- [x] 执行时间统计
- [x] 编译成功（仅有未使用参数警告）
- [x] **调试输出**（显示分词和索引状态）
- [x] **单元测试框架** + 测试用例
- [x] **Snippet 生成** + **关键词高亮** ⭐⭐⭐
- [x] **文件加载功能** (TSV/CSV/JSONL) ⭐⭐ （新增）

### 进行中 ⏳
- [ ] PHRASE 搜索（精确短语匹配）

### 计划中 🔜
- [ ] 中文分词（Jieba/HMM）
- [ ] SQLite FTS5 集成
- [ ] 性能优化
- [ ] iOS/Android 交叉编译

---

---

## 🧪 试用指南

### 编译（Ubuntu/WSL2）
```bash
cd /mnt/e/development/FusionSearch
make clean
make
```

### 运行
```bash
./search_engine
```

### 菜单选项
```
1. AND 搜索     - 所有关键词都必须出现
2. OR 搜索      - 任一关键词出现即可
3. BM25 搜索    - BM25 相关性排序（推荐）
4. PHRASE 搜索  - 精确短语匹配（待实现）
5. 加载文件     - 从 TSV/CSV/JSONL 文件加载文档
6. 索引统计     - 查看索引统计信息
7. 字典内容     - 查看所有分词结果
8. 退出
```

### 试用示例

**场景1：AND 搜索**
```
Query: "programming language"
Result: 返回同时包含 "programming" 和 "language" 的文档
        - Python Programming Guide
        - C Language Fundamentals
        - JavaScript for Web Development
```

**场景2：OR 搜索**
```
Query: "python javascript"
Result: 返回包含 "python" 或 "javascript" 的文档
        - Python Programming Guide
        - JavaScript for Web Development
```

**场景3：单词搜索**
```
Query: "database"
Result: 返回包含 "database" 的文档
        - Database Design Principles
```

---

## 📊 内置样本文档

程序内置了5个英文技术文档供测试：

1. **Python Programming Guide** - Python 高级特性、范型介绍
2. **JavaScript for Web Development** - Web 前端开发框架
3. **C Language Fundamentals** - C 语言低层特性、指针
4. **Data Structures and Algorithms** - 数据结构、算法
5. **Database Design Principles** - 数据库设计、索引优化

---

## 📈 性能表现

- **编译时间**：< 1 秒
- **初始化时间**：< 10 ms（5个文档）
- **搜索时间**：< 0.1 ms（OR 搜索）
- **可执行文件大小**：~100 KB
- **内存占用**：< 10 MB

### 编译信息
```
✓ 编译成功（零错误）
⚠️ 仅有未使用参数警告（这是正常的，待实现功能的参数）
✓ WSL2 Ubuntu 编译通过
```

### 搜索性能示例
```
Query: "python javascript" (2 terms, OR mode)
Results: 2 documents found
Time: 0.05 ms
Sorting: By TF-IDF score
```

---

## 📊 数据结构

### Trie 树
```
root
├── h -> [e] -> [l] -> [l] -> [o*] (频率: 2)
└── p -> [r] -> [o] -> [g] -> [r] -> [a] -> [m] -> [*] (频率: 1)
```

### 倒排索引
```
"hello" -> [PostingEntry{docId: 1, freq: 2},
            PostingEntry{docId: 3, freq: 1}]

"world" -> [PostingEntry{docId: 1, freq: 1},
            PostingEntry{docId: 2, freq: 3}]
```

### 搜索模式
- **AND** - 所有查询词都必须出现
- **OR** - 任一查询词出现即可
- **PHRASE** - 精确短语匹配（待实现）

---

## 🔧 技术细节

### 英文分词算法
```
输入: "Hello, World!"
处理: 转小写 -> "hello, world!"
分割: 按空格/标点分割
输出: ["hello", "world"]
```

### 排序算法

**TF-IDF** (Term Frequency-Inverse Document Frequency)
```
Score = (词频 / 文档长度) × log(总文档数 / 包含词的文档数)
特点: 简单快速，适合通用搜索
```

**BM25** (Best Matching 25) ⭐ **推荐**
```
Score = IDF × ((k1+1) × TF) / (k1×(1-b+b×(|D|/avgdl)) + TF)

其中:
  IDF = log((N - n + 0.5) / (n + 0.5))
  N = 总文档数
  n = 包含词的文档数
  |D| = 文档长度
  avgdl = 平均文档长度
  k1 = 1.5（词频饱和参数，默认）
  b = 0.75（文档长度归一化参数，默认）

特点: 
  • 更好的相关性排序
  • 饱和函数（避免词频过高导致过度加权）
  • 文档长度归一化
  • 业界标准排序算法
```

---

## 📈 性能分析

| 操作 | 时间复杂度 | 空间复杂度 |
|------|-----------|-----------|
| 插入词 | O(L) | O(1) |
| 查询词 | O(L) | O(1) |
| 建索引 | O(N×L) | O(M) |
| 搜索 | O(K×P) | O(R) |

说明：
- L = 词长度
- N = 总词数
- M = 倒排表大小
- K = 查询词数
- P = 平均 posting 列表长度
- R = 返回结果数

---

## 📝 下一步任务

### 优先级 1（本周完成）✅ 已完成
- [x] 实现 AND 搜索逻辑
- [x] 实现 OR 搜索逻辑
- [x] 实现 TF-IDF 排序
- [x] 实现 BM25 排序 ⭐
- [x] 创建单元测试框架
- [x] Snippet 生成 + 关键词高亮 ⭐
- [x] 基础搜索功能测试通过

### 优先级 2（下周）
- [ ] 完善单元测试覆盖率（目标 >90%）
- [ ] 性能对比（TF-IDF vs BM25）
- [ ] 优化关键词匹配算法
- [ ] 删除调试输出，优化界面

### 优先级 3（后续）
- [ ] 文件 I/O（加载词典、文档）
- [ ] 中文分词集成（Jieba）
- [ ] 性能优化 + 内存管理
- [ ] PHRASE 搜索实现

---

## 💡 代码示例

### 基础使用
```c
// 创建搜索引擎
Trie* dict = trie_create();
InvertedIndex* idx = index_create();
Tokenizer* tok = tokenizer_create(dict);
SearchEngine* engine = search_engine_create(idx, tok);

// 索引文档
const char* tokens[] = {"hello", "world"};
index_add_document(idx, 1, "Title", "Content", tokens, 2);

// 查询
SearchResultSet* results = search_engine_search(
    engine, "hello", SEARCH_AND, 10);

// 清理
search_free_results(results);
search_engine_destroy(engine);
```

---

## 📚 参考资源

- [Full-Text Search](https://en.wikipedia.org/wiki/Full-text_search)
- [Inverted Index](https://en.wikipedia.org/wiki/Inverted_index)
- [BM25 Algorithm](https://en.wikipedia.org/wiki/Okapi_BM25)
- [Trie Data Structure](https://en.wikipedia.org/wiki/Trie)
- [SQLite FTS5](https://www.sqlite.org/fts5.html)
- [Jieba 中文分词](https://github.com/fxsjy/jieba)

---

## 📄 许可证

MIT License

## 👤 作者

FusionSearch 跨平台搜索引擎项目

**创建时间**: 2025-10-24  
**当前版本**: 0.6.0 (文件加载功能完成)  
**编译环境**: WSL2 Ubuntu + GCC  
**最后更新**: 2025-10-24
