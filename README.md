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
│   └── utils.h          # 工具函数
├── src/                 # 源文件（实现）
│   ├── main.c           # 主程序 + 菜单
│   ├── trie.c           # Trie 实现 (~150行)
│   ├── tokenizer.c      # 分词实现 (~100行)
│   ├── index.c          # 倒排索引实现 (~150行)
│   ├── search.c         # 搜索引擎实现 (~100行)
│   └── utils.c          # 工具函数 (~250行)
├── data/                # 数据文件（词典、文档）
├── Makefile             # 编译配置
└── README.md            # 本文件
```

---

## 🏗️ 核心模块

| 模块 | 用途 | 状态 |
|------|------|------|
| **Trie** | 字典树 + 词频存储 | ✅ 完成 |
| **Tokenizer** | 英文分词（空格/标点） | ✅ 完成 |
| **Index** | 倒排索引（词->文档） | ✅ 框架完成 |
| **Search** | 查询处理 + 排序 | ⏳ 进行中 |
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
- [x] 倒排索引基础框架
- [x] 菜单驱动的交互界面
- [x] 5 个样本文档自动加载

### 进行中 ⏳
- [ ] AND/OR/PHRASE 搜索逻辑
- [ ] TF-IDF 排序
- [ ] BM25 排序
- [ ] Snippet 生成 + 高亮

### 计划中 🔜
- [ ] 文件加载（词典、文档）
- [ ] 中文分词（Jieba/HMM）
- [ ] SQLite FTS5 集成
- [ ] 性能优化
- [ ] iOS/Android 交叉编译

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

## 🧪 样本数据

程序内置 5 个测试文档：
1. Python Programming Guide
2. JavaScript for Web Development
3. C Language Fundamentals
4. Data Structures and Algorithms
5. Database Design Principles

运行程序时自动加载。

---

## 🔧 技术细节

### 英文分词算法
```
输入: "Hello, World!"
处理: 转小写 -> "hello, world!"
分割: 按空格/标点分割
输出: ["hello", "world"]
```

### 排序算法（待实现）

**TF-IDF**
```
Score = (词频 / 文档长度) × log(总文档数 / 包含词的文档数)
```

**BM25**
```
Score = IDF × ((k1+1) × TF) / (k1×(1-b+b×(文档长) / 平均长) + TF)
参数: k1=1.5, b=0.75
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

### 优先级 1（本周）
- [ ] 实现 AND 搜索逻辑
- [ ] 实现 OR 搜索逻辑
- [ ] 测试基础搜索功能

### 优先级 2（下周）
- [ ] 实现 TF-IDF 排序
- [ ] 实现 BM25 排序
- [ ] 生成搜索结果 snippet

### 优先级 3（后续）
- [ ] 文件 I/O（加载词典、文档）
- [ ] 中文分词集成
- [ ] 性能优化

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
**当前版本**: 0.1.0 (英文骨架完成)
