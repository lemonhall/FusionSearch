#include "cjk_tokenizer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 检查是否编译时包含了 ICU 支持
#ifdef ENABLE_ICU

#include <unicode/ubrk.h>
#include <unicode/ustring.h>

// 全局变量
static char g_locale[32] = "ja";  // 默认日文
static char g_error[256] = "";

/**
 * 设置错误信息
 */
static void set_error(const char* msg) {
    strncpy(g_error, msg, sizeof(g_error) - 1);
    g_error[sizeof(g_error) - 1] = '\0';
}

/**
 * 初始化 CJK 分词器
 */
int cjk_tokenizer_init(const char* locale) {
    if (locale && strlen(locale) > 0) {
        strncpy(g_locale, locale, sizeof(g_locale) - 1);
        g_locale[sizeof(g_locale) - 1] = '\0';
    }
    
    set_error("");
    return 0;
}

/**
 * CJK 分词主函数
 */
TokenList* cjk_tokenize(const char* text) {
    if (!text || strlen(text) == 0) {
        set_error("输入文本为空");
        return NULL;
    }
    
    UErrorCode status = U_ZERO_ERROR;
    
    // 创建 Token 列表
    TokenList* tokens = (TokenList*)safe_malloc(sizeof(TokenList));
    tokens->count = 0;
    
    // UTF-8 -> UTF-16 转换
    int32_t text_len = (int32_t)strlen(text);
    int32_t utf16_capacity = text_len + 1;
    UChar* utf16_text = (UChar*)safe_malloc(sizeof(UChar) * utf16_capacity);
    
    int32_t utf16_len;
    u_strFromUTF8(utf16_text, utf16_capacity, &utf16_len, text, text_len, &status);
    
    if (U_FAILURE(status)) {
        set_error("UTF-8 转 UTF-16 失败");
        free(utf16_text);
        free(tokens);
        return NULL;
    }
    
    // 创建断词迭代器
    UBreakIterator* break_iter = ubrk_open(UBRK_WORD, g_locale, utf16_text, utf16_len, &status);
    
    if (U_FAILURE(status)) {
        snprintf(g_error, sizeof(g_error), "创建断词迭代器失败 (locale: %s)", g_locale);
        free(utf16_text);
        free(tokens);
        return NULL;
    }
    
    // 分配 UTF-8 token 缓冲区
    char token_buffer[1024];  // 单个 token 最大 1KB
    
    // 遍历所有断词边界
    int32_t start = ubrk_first(break_iter);
    for (int32_t end = ubrk_next(break_iter); end != UBRK_DONE; 
         start = end, end = ubrk_next(break_iter)) {
        
        // 检查是否是有效单词（排除空格、标点）
        int32_t rule_status = ubrk_getRuleStatus(break_iter);
        if (rule_status != UBRK_WORD_NONE && rule_status != UBRK_WORD_NONE_LIMIT) {
            
            int32_t token_utf16_len = end - start;
            int32_t token_utf8_len;
            
            // UTF-16 -> UTF-8 转换
            status = U_ZERO_ERROR;
            u_strToUTF8(token_buffer, sizeof(token_buffer), &token_utf8_len, 
                       utf16_text + start, token_utf16_len, &status);
            
            if (!U_FAILURE(status) && token_utf8_len > 0 && tokens->count < MAX_TOKEN_COUNT) {
                // 添加到 token 列表
                tokens->tokens[tokens->count] = string_dup(token_buffer);
                tokens->count++;
            }
        }
    }
    
    // 清理资源
    ubrk_close(break_iter);
    free(utf16_text);
    
    return tokens;
}

/**
 * 清理 CJK 分词器
 */
void cjk_tokenizer_cleanup(void) {
    // ICU 资源会自动释放
}

/**
 * 获取错误信息
 */
const char* cjk_get_error(void) {
    return g_error;
}

/**
 * 检查 ICU 是否可用
 */
int cjk_is_available(void) {
    return 1;  // 编译时启用了 ICU
}

#else  // 未启用 ICU

// ICU 未启用时的桩函数
int cjk_tokenizer_init(const char* locale) {
    (void)locale;
    return -1;
}

TokenList* cjk_tokenize(const char* text) {
    (void)text;
    return NULL;
}

void cjk_tokenizer_cleanup(void) {
    // 空实现
}

const char* cjk_get_error(void) {
    return "ICU 支持未启用（编译时需要 -DENABLE_ICU）";
}

int cjk_is_available(void) {
    return 0;  // ICU 不可用
}

#endif  // ENABLE_ICU
