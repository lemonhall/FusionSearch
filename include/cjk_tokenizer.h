#ifndef CJK_TOKENIZER_H
#define CJK_TOKENIZER_H

#include "tokenizer.h"
#include <stddef.h>

/**
 * CJK (中日韩) 分词器 - 基于 ICU
 * 支持中文、日文、韩文及混合语言文本分词
 */

/**
 * 初始化 CJK 分词器
 * @param locale 语言区域 ("zh", "ja", "ko", "en" 或 NULL 使用默认)
 * @return 0 成功，-1 失败
 */
int cjk_tokenizer_init(const char* locale);

/**
 * 对 CJK 文本进行分词
 * @param text UTF-8 编码的输入文本
 * @return TokenList 结构（需要调用 tokenizer_free_tokens 释放）
 */
TokenList* cjk_tokenize(const char* text);

/**
 * 清理 CJK 分词器资源
 */
void cjk_tokenizer_cleanup(void);

/**
 * 获取最后的错误信息
 * @return 错误信息字符串
 */
const char* cjk_get_error(void);

/**
 * 检查 ICU 库是否可用
 * @return 1 可用，0 不可用
 */
int cjk_is_available(void);

#endif // CJK_TOKENIZER_H
