#include "icu_tokenizer.h"
#include <stdio.h>
#include <string.h>

/**
 * 测试函数：打印分词结果
 */
void test_tokenize(const char* locale, const char* text) {
    printf("\n========================================\n");
    printf("Locale: %s\n", locale);
    printf("Input:  %s\n", text);
    printf("----------------------------------------\n");
    
    // 设置语言
    if (icu_tokenizer_init(locale) != 0) {
        printf("❌ 初始化失败: %s\n", icu_get_error());
        return;
    }
    
    // 分词
    ICUTokenList* tokens = icu_tokenize(text);
    if (!tokens) {
        printf("❌ 分词失败: %s\n", icu_get_error());
        return;
    }
    
    // 打印结果
    printf("Tokens: %zu\n", tokens->count);
    printf("Output: [");
    for (size_t i = 0; i < tokens->count; i++) {
        printf("\"%s\"", tokens->tokens[i]);
        if (i < tokens->count - 1) {
            printf(", ");
        }
    }
    printf("]\n");
    
    // 释放内存
    icu_free_tokens(tokens);
}

int main(void) {
    printf("===========================================\n");
    printf("  ICU 分词器测试程序\n");
    printf("  支持中日韩（CJK）多语言分词\n");
    printf("===========================================\n");
    
    // 测试日文
    test_tokenize("ja", "これは日本語のテストです");
    test_tokenize("ja", "SQLiteデータベースで全文検索ができます");
    
    // 测试中文
    test_tokenize("zh", "这是中文测试内容");
    test_tokenize("zh", "我喜欢使用SQLite数据库");
    test_tokenize("zh", "机器学习和深度学习很有趣");
    
    // 测试韩文
    test_tokenize("ko", "한국어 테스트 콘텐츠입니다");
    test_tokenize("ko", "데이터베이스를 사용합니다");
    
    // 测试英文
    test_tokenize("en", "This is an English test");
    test_tokenize("en", "SQLite full-text search is powerful");
    
    // 测试混合语言
    test_tokenize("ja", "SQLite supports 日本語 and English");
    test_tokenize("zh", "使用Python和机器学习技术");
    
    printf("\n========================================\n");
    printf("✅ 所有测试完成！\n");
    printf("========================================\n");
    
    icu_tokenizer_cleanup();
    return 0;
}
