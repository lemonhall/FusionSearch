#ifndef ICU_TOKENIZER_H
#define ICU_TOKENIZER_H

#include <stddef.h>
#include <stdint.h>

/**
 * ICU 分词器接口
 * 支持中日韩（CJK）多语言分词
 */

typedef struct {
    char** tokens;      // Token 数组
    size_t count;       // Token 数量
    size_t capacity;    // 数组容量
} ICUTokenList;

/**
 * 创建 ICU 分词器
 * @param locale 语言区域（如 "ja", "zh", "ko", "en"）
 * @return 成功返回0，失败返回-1
 */
int icu_tokenizer_init(const char* locale);

/**
 * 对文本进行分词
 * @param text UTF-8 编码的输入文本
 * @return Token 列表（需要调用 icu_free_tokens 释放）
 */
ICUTokenList* icu_tokenize(const char* text);

/**
 * 释放 Token 列表
 */
void icu_free_tokens(ICUTokenList* tokens);

/**
 * 清理分词器
 */
void icu_tokenizer_cleanup(void);

/**
 * 获取错误信息
 */
const char* icu_get_error(void);

#endif // ICU_TOKENIZER_H
