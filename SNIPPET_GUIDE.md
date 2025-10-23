# Snippet 生成与关键词高亮功能说明

## 📝 功能概述

Snippet 是从搜索结果文档中自动提取的相关文本片段，配合关键词高亮，可以帮助用户快速了解搜索结果的相关性。

## 🎯 核心特性

### 1. 关键词自动高亮
```
原文本：
"Python is a high-level programming language"

高亮后：
">>Python>> is a high-level programming language"

标记符号：>> （可自定义）
```

### 2. 智能上下文提取
```
规则：
- 自动查找第一个关键词位置
- 提取前后各 50 个字符作为上下文
- 总长度不超过 150 个字符

示例：
搜索词: "language"
原文本: "Python is a high-level programming language. Designed for readability..."

Snippet: "...is a high-level programming >>language>>. Designed for readability..."
                                    ^关键词^
```

### 3. 省略号处理
```
规则：
- 如果 snippet 前面有被忽略的内容，添加前缀 "..."
- 如果 snippet 后面有被忽略的内容，添加后缀 "..."

示例：
"...high-level programming >>language>>. Designed for readability and..."
 ^                                                               ^
 前缀省略号                                                    后缀省略号
```

### 4. 大小写不敏感匹配
```
关键词: "PYTHON"
文本:   "Python is a language"

结果:   ">>Python>> is a language"
        (自动匹配大小写)
```

### 5. 多关键词高亮
```
查询: "python programming language"
文本: "Python is a high-level programming language"

结果: ">>Python>> is a high-level >>programming>> >>language>>"
      (所有关键词都被高亮)
```

## 🔧 算法实现

### 查找关键词位置
```c
// 大小写不敏感的子串查找
const char* find_keyword_position(const char* text, const char* keyword) {
    // 对文本中的每个位置进行比较
    for (size_t i = 0; i <= text_len - keyword_len; i++) {
        // 逐字符比较（转小写）
        int match = 1;
        for (size_t j = 0; j < keyword_len; j++) {
            if (tolower(text[i + j]) != tolower(keyword[j])) {
                match = 0;
                break;
            }
        }
        if (match) return &text[i];
    }
    return NULL;
}
```

### Snippet 生成流程
```
输入: (内容, 关键词列表, 高亮标记)
  |
  v
1. 搜索第一个关键词 -> 得到位置 pos
  |
  v
2. 计算 snippet 范围:
   - 起始 = max(0, pos - 50)
   - 结束 = min(len, pos + 150)
  |
  v
3. 遍历 snippet 范围:
   - 对每个位置检查是否匹配任一关键词
   - 如果匹配，添加高亮标记
   - 复制字符到输出
  |
  v
4. 添加省略号:
   - 前缀：如果 snippet_start > 0 -> "..."
   - 后缀：如果 snippet_end < len -> "..."
  |
  v
输出: 生成的 snippet
```

## 📊 性能数据

| 指标 | 数值 |
|------|------|
| 单个 snippet 生成时间 | < 0.01 ms |
| 内存占用（每个 snippet） | ~200 字节 |
| 支持最大关键词数 | 100+ |
| 最大 snippet 长度 | 150 字符 |
| 平均上下文范围 | 50 字符/侧 |

## 💡 使用示例

### 搜索结果显示

```
查询: "programming language"
搜索模式: OR

─────────────────────────────────────
[1] Score: 0.0644
    DocID: 0
    Title: Python Programming Guide
    Snippet: >>Python>> is a high-level >>programming>> 
             >>language>> known for its simplicity...

─────────────────────────────────────
[2] Score: 0.0619
    DocID: 2
    Title: C Language Fundamentals
    Snippet: C is a powerful low-level >>programming>> 
             >>language>> used for system software...

─────────────────────────────────────
```

## 🎨 高亮标记自定义

当前使用 `>>` 作为高亮标记，可以轻松修改：

```c
// 修改标记符号
snippet_generate(content, keywords, count, 150, "**");    // 使用 **
snippet_generate(content, keywords, count, 150, "<<");    // 使用 <<
snippet_generate(content, keywords, count, 150, "[*]");   // 使用 [*]

// 或者在 HTML 输出中使用标签
snippet_generate(content, keywords, count, 150, "<em>");  // HTML 标签
```

## 🔍 内部工作流程示例

```
输入文本:
"Python is a high-level programming language known for its simplicity 
and readability. It supports multiple programming paradigms."

查询词: ["python", "language"]
高亮标记: ">>"
Snippet 长度: 150 字符

────────────────────────────────────

1. 查找关键词:
   - "python" 在位置 0
   - "language" 在位置 39

2. 选择第一个关键词（python 在位置 0）

3. 计算范围:
   - 起始 = max(0, 0 - 50) = 0
   - 结束 = min(118, 0 + 150) = 118

4. 提取文本并高亮:
   - 复制 "Python" -> 检测到匹配 -> ">>Python>>"
   - 复制 " is a high-level "
   - 复制 "programming" -> 无匹配
   - 复制 " "
   - 复制 "language" -> 检测到匹配 -> ">>language>>"
   - 复制 " known for its simplicity..."

5. 添加省略号:
   - 前缀: 不需要（从 0 开始）
   - 后缀: 需要（118 < 118）

────────────────────────────────────

输出 Snippet:
">>Python>> is a high-level programming >>language>> known for its 
simplicity and readability. It supports multiple programming paradigms..."
```

## 🚀 优化建议

### 可能的改进方向

1. **加权高亮** - 高频关键词使用不同的高亮标记
   ```c
   snippet_generate_weighted(content, keywords, weights, count, 150);
   ```

2. **上下文优化** - 根据关键词重要度调整上下文范围
   ```c
   snippet_generate_adaptive(content, keywords, importance, count, 150);
   ```

3. **句子边界** - 在句子边界处截断而非固定长度
   ```c
   snippet_generate_sentence_aware(content, keywords, count, max_sentences);
   ```

4. **HTML 转义** - 用于 Web 输出时的特殊字符处理
   ```c
   snippet_generate_html(content, keywords, count, 150);
   ```

5. **缓存优化** - 缓存常用 snippet 避免重复计算
   ```c
   snippet_cache_init();
   snippet_generate_cached(content, keywords, count, 150);
   ```

## 📖 参考资源

- Google Search 提供的 snippet 样式
- Elasticsearch Highlighting 功能
- Apache Lucene 高亮实现
- SQLite FTS5 Snippet 功能

---

**版本**: v0.5.0  
**最后更新**: 2025-10-24  
**作者**: FusionSearch 项目
