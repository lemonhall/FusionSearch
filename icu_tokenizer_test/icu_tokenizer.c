#include "icu_tokenizer.h"
#include <unicode/ubrk.h>
#include <unicode/ustring.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 全局变量
static char g_locale[32] = "ja";  // 默认日文
static char g_error[256] = "";
static UBreakIterator* g_break_iter = NULL;

/**
 * 设置错误信息
 */
static void set_error(const char* msg) {
    strncpy(g_error, msg, sizeof(g_error) - 1);
    g_error[sizeof(g_error) - 1] = '\0';
}

/**
 * 初始化 ICU 分词器
 */
int icu_tokenizer_init(const char* locale) {
    if (locale && strlen(locale) > 0) {
        strncpy(g_locale, locale, sizeof(g_locale) - 1);
        g_locale[sizeof(g_locale) - 1] = '\0';
    }
    
    set_error("");
    return 0;
}

/**
 * 创建 Token 列表
 */
static ICUTokenList* create_token_list(void) {
    ICUTokenList* list = (ICUTokenList*)malloc(sizeof(ICUTokenList));
    if (!list) {
        set_error("内存分配失败");
        return NULL;
    }
    
    list->capacity = 100;
    list->count = 0;
    list->tokens = (char**)malloc(sizeof(char*) * list->capacity);
    
    if (!list->tokens) {
        free(list);
        set_error("内存分配失败");
        return NULL;
    }
    
    return list;
}

/**
 * 添加 Token
 */
static int add_token(ICUTokenList* list, const char* token, size_t len) {
    if (list->count >= list->capacity) {
        // 扩容
        size_t new_capacity = list->capacity * 2;
        char** new_tokens = (char**)realloc(list->tokens, sizeof(char*) * new_capacity);
        if (!new_tokens) {
            set_error("内存扩容失败");
            return -1;
        }
        list->tokens = new_tokens;
        list->capacity = new_capacity;
    }
    
    // 复制 token
    list->tokens[list->count] = (char*)malloc(len + 1);
    if (!list->tokens[list->count]) {
        set_error("内存分配失败");
        return -1;
    }
    
    memcpy(list->tokens[list->count], token, len);
    list->tokens[list->count][len] = '\0';
    list->count++;
    
    return 0;
}

/**
 * 分词主函数
 */
ICUTokenList* icu_tokenize(const char* text) {
    if (!text || strlen(text) == 0) {
        set_error("输入文本为空");
        return NULL;
    }
    
    UErrorCode status = U_ZERO_ERROR;
    
    // 创建 Token 列表
    ICUTokenList* tokens = create_token_list();
    if (!tokens) {
        return NULL;
    }
    
    // UTF-8 -> UTF-16 转换
    int32_t text_len = (int32_t)strlen(text);
    int32_t utf16_capacity = text_len + 1;
    UChar* utf16_text = (UChar*)malloc(sizeof(UChar) * utf16_capacity);
    
    if (!utf16_text) {
        set_error("UTF-16 内存分配失败");
        icu_free_tokens(tokens);
        return NULL;
    }
    
    int32_t utf16_len;
    u_strFromUTF8(utf16_text, utf16_capacity, &utf16_len, text, text_len, &status);
    
    if (U_FAILURE(status)) {
        set_error("UTF-8 转 UTF-16 失败");
        free(utf16_text);
        icu_free_tokens(tokens);
        return NULL;
    }
    
    // 创建断词迭代器
    UBreakIterator* break_iter = ubrk_open(UBRK_WORD, g_locale, utf16_text, utf16_len, &status);
    
    if (U_FAILURE(status)) {
        snprintf(g_error, sizeof(g_error), "创建断词迭代器失败 (locale: %s)", g_locale);
        free(utf16_text);
        icu_free_tokens(tokens);
        return NULL;
    }
    
    // 分配 UTF-8 token 缓冲区
    char* token_buffer = (char*)malloc(1024);  // 单个 token 最大 1KB
    if (!token_buffer) {
        set_error("Token 缓冲区分配失败");
        ubrk_close(break_iter);
        free(utf16_text);
        icu_free_tokens(tokens);
        return NULL;
    }
    
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
            u_strToUTF8(token_buffer, 1024, &token_utf8_len, 
                       utf16_text + start, token_utf16_len, &status);
            
            if (!U_FAILURE(status) && token_utf8_len > 0) {
                // 添加到 token 列表
                if (add_token(tokens, token_buffer, token_utf8_len) != 0) {
                    // 添加失败，清理并返回 NULL
                    free(token_buffer);
                    ubrk_close(break_iter);
                    free(utf16_text);
                    icu_free_tokens(tokens);
                    return NULL;
                }
            }
        }
    }
    
    // 清理资源
    free(token_buffer);
    ubrk_close(break_iter);
    free(utf16_text);
    
    return tokens;
}

/**
 * 释放 Token 列表
 */
void icu_free_tokens(ICUTokenList* tokens) {
    if (!tokens) return;
    
    if (tokens->tokens) {
        for (size_t i = 0; i < tokens->count; i++) {
            if (tokens->tokens[i]) {
                free(tokens->tokens[i]);
            }
        }
        free(tokens->tokens);
    }
    
    free(tokens);
}

/**
 * 清理分词器
 */
void icu_tokenizer_cleanup(void) {
    // ICU 资源会自动释放
}

/**
 * 获取错误信息
 */
const char* icu_get_error(void) {
    return g_error;
}
