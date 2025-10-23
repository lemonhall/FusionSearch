# ICU 分词器测试项目

这是一个使用 **ICU (International Components for Unicode)** 库实现的中日韩（CJK）多语言分词器测试项目。

## 📋 项目结构

```
icu_tokenizer_test/
├── icu_tokenizer.h      # 分词器头文件
├── icu_tokenizer.c      # 分词器实现（215行）
├── test_main.c          # 测试程序
├── Makefile             # 编译脚本
└── README.md            # 本文件
```

## 🚀 快速开始

### 1. 安装 ICU 库

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libicu-dev
```

#### CentOS/RHEL
```bash
sudo dnf install libicu-devel
```

#### macOS
```bash
brew install icu4c
```

#### Windows
```powershell
# 使用 vcpkg
vcpkg install icu

# 或下载预编译版本
# https://github.com/unicode-org/icu/releases
```

### 2. 编译

```bash
cd icu_tokenizer_test
make
```

### 3. 运行测试

```bash
make run
```

## 📊 测试示例

程序会测试以下语言的分词效果：

### 日文
```
输入: これは日本語のテストです
输出: ["これ", "は", "日本語", "の", "テスト", "です"]
```

### 中文
```
输入: 这是中文测试内容
输出: ["这", "是", "中文", "测试", "内容"]
```

### 韩文
```
输入: 한국어 테스트 콘텐츠입니다
输出: ["한국어", "테스트", "콘텐츠", "입니다"]
```

### 英文
```
输入: This is an English test
输出: ["This", "is", "an", "English", "test"]
```

### 混合语言
```
输入: SQLite supports 日本語 and English
输出: ["SQLite", "supports", "日本", "語", "and", "English"]
```

## 📱 移动平台支持

### iOS

**支持情况**: ✅ **完全支持**

iOS 系统内置 ICU 库，可以直接使用：

#### 编译选项
```bash
# 使用 Xcode 编译时添加链接选项
-licucore  # 链接系统 ICU 库
```

#### Xcode 项目配置
1. 添加源文件：`icu_tokenizer.c`, `icu_tokenizer.h`
2. Build Settings → Other Linker Flags → 添加 `-licucore`
3. 包含头文件路径：系统已包含，无需额外设置

#### 注意事项
- iOS 10.0+ 完全支持
- 无需额外下载 ICU 库
- 二进制体积增加：**约 0 KB**（使用系统库）

---

### Android

**支持情况**: ✅ **支持（需要 NDK）**

Android 系统内置 ICU，但需要通过 NDK 访问：

#### NDK 配置（Android.mk）
```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := icu_tokenizer
LOCAL_SRC_FILES := icu_tokenizer.c
LOCAL_LDLIBS := -licuuc -licui18n  # 链接 ICU 库
include $(BUILD_SHARED_LIBRARY)
```

#### CMake 配置（CMakeLists.txt）
```cmake
cmake_minimum_required(VERSION 3.4.1)

add_library(icu_tokenizer SHARED
    icu_tokenizer.c
)

# 链接 ICU 库
target_link_libraries(icu_tokenizer
    icuuc
    icui18n
)
```

#### 注意事项
- Android 7.0 (API 24)+ 完全支持
- API 24 以下需要打包 ICU 库（约 30 MB）
- 建议最低 API Level 24

---

## 🔧 API 使用示例

### 基本用法

```c
#include "icu_tokenizer.h"

// 1. 初始化（设置语言）
icu_tokenizer_init("zh");  // 中文

// 2. 分词
ICUTokenList* tokens = icu_tokenize("这是测试文本");

// 3. 使用 tokens
for (size_t i = 0; i < tokens->count; i++) {
    printf("%s ", tokens->tokens[i]);
}

// 4. 释放内存
icu_free_tokens(tokens);
icu_tokenizer_cleanup();
```

### 多语言切换

```c
// 日文
icu_tokenizer_init("ja");
ICUTokenList* ja_tokens = icu_tokenize("日本語");

// 中文
icu_tokenizer_init("zh");
ICUTokenList* zh_tokens = icu_tokenize("中文");

// 韩文
icu_tokenizer_init("ko");
ICUTokenList* ko_tokens = icu_tokenize("한국어");
```

## ⚠️ 重要提醒

### 依赖库体积

ICU 库较大，会增加应用体积：

| 平台 | ICU 库大小 | 影响 |
|------|-----------|------|
| **iOS** | 0 KB | ✅ 使用系统库，无影响 |
| **Android (API 24+)** | 0 KB | ✅ 使用系统库，无影响 |
| **Android (API < 24)** | ~30 MB | ⚠️ 需要打包 ICU 库 |
| **桌面平台** | ~30 MB | ⚠️ 需要安装 ICU 库 |

### 分词精度

#### 中文分词示例
```
输入: "我喜欢学习机器学习"
ICU 输出: ["我", "喜欢", "学习", "机器", "学习"]
理想输出: ["我", "喜欢", "学习", "机器学习"]  ← 需要词典
```

ICU 是**字级别分词**（不是词级别），适合以下场景：
- ✅ 全文搜索（高召回率）
- ✅ 关键词匹配
- ⚠️ 语义分析（精度较低）

## 🆚 与轻量级方案对比

| 特性 | ICU 方案 | N-gram 方案（纯C实现） |
|------|---------|---------------------|
| **依赖库** | ❌ 需要 ICU (~30MB) | ✅ 零依赖 |
| **代码量** | ~200 行 | ~300 行 |
| **精度** | ⭐⭐⭐ 较好 | ⭐⭐ 一般 |
| **性能** | ⭐⭐⭐⭐ 快 | ⭐⭐⭐⭐⭐ 很快 |
| **跨平台** | ✅ iOS/Android 内置 | ✅ 完全跨平台 |
| **体积影响** | iOS: 0KB, Android: 0-30MB | < 50KB |

## 🎯 建议

### 适合使用 ICU 的场景：
- ✅ iOS 应用（系统自带）
- ✅ Android API 24+ 应用
- ✅ 对分词精度要求较高
- ✅ 桌面应用（可接受依赖）

### 不适合使用 ICU 的场景：
- ❌ 追求极致轻量级（< 1MB）
- ❌ Android API < 24（需要打包 30MB）
- ❌ 嵌入式设备（资源受限）
- ❌ 要求零外部依赖

## 📚 参考资源

- **ICU 官网**: https://icu.unicode.org/
- **ICU GitHub**: https://github.com/unicode-org/icu
- **ICU 文档**: https://unicode-org.github.io/icu/userguide/

## 📝 许可证

本测试项目为演示代码，可自由使用。
ICU 库遵循 Unicode License。
