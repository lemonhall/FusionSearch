# 移动平台部署指南

详细说明如何在 iOS 和 Android 上使用 ICU 分词器。

## 📱 iOS 平台部署

### ✅ 系统支持

- **iOS 10.0+**: 完全支持
- **系统自带**: 无需额外下载 ICU 库
- **库名称**: `libicucore.dylib`（系统库）

### 🛠️ Xcode 项目配置

#### 方法 1: 手动添加文件

1. **添加源文件到项目**
   - 拖拽 `icu_tokenizer.c` 和 `icu_tokenizer.h` 到 Xcode 项目

2. **配置链接选项**
   - 选择 Target → Build Settings
   - 搜索 "Other Linker Flags"
   - 添加: `-licucore`

3. **包含头文件**
   ```objective-c
   #import "icu_tokenizer.h"
   ```

#### 方法 2: 使用 Bridging Header（Swift 项目）

1. **创建 Bridging Header**
   - 创建文件: `YourProject-Bridging-Header.h`
   - 添加内容:
   ```objective-c
   #import "icu_tokenizer.h"
   ```

2. **配置 Bridging Header 路径**
   - Build Settings → Objective-C Bridging Header
   - 设置为: `YourProject/YourProject-Bridging-Header.h`

3. **Swift 中使用**
   ```swift
   icu_tokenizer_init("ja")
   let tokens = icu_tokenize("日本語テスト")
   // 使用 tokens
   icu_free_tokens(tokens)
   ```

### 📝 Objective-C 示例

```objective-c
#import "icu_tokenizer.h"

- (void)tokenizeText {
    // 初始化
    icu_tokenizer_init("zh");
    
    // 分词
    ICUTokenList* tokens = icu_tokenize("这是中文测试");
    
    // 遍历结果
    for (size_t i = 0; i < tokens->count; i++) {
        NSString* token = [NSString stringWithUTF8String:tokens->tokens[i]];
        NSLog(@"Token: %@", token);
    }
    
    // 释放内存
    icu_free_tokens(tokens);
}
```

### 📝 Swift 示例

```swift
import Foundation

func tokenizeText() {
    // 初始化
    icu_tokenizer_init("zh")
    
    // 分词
    guard let tokens = icu_tokenize("这是中文测试") else {
        print("分词失败")
        return
    }
    
    // 遍历结果
    for i in 0..<tokens.pointee.count {
        if let tokenPtr = tokens.pointee.tokens[Int(i)] {
            let token = String(cString: tokenPtr)
            print("Token: \(token)")
        }
    }
    
    // 释放内存
    icu_free_tokens(tokens)
}
```

### ⚠️ iOS 注意事项

1. **内存管理**: 记得调用 `icu_free_tokens()` 释放内存
2. **线程安全**: ICU 库线程安全，但建议在主线程初始化
3. **最低版本**: 建议设置 Deployment Target 为 iOS 10.0+

---

## 🤖 Android 平台部署

### ✅ 系统支持

| Android 版本 | ICU 支持 | 说明 |
|--------------|---------|------|
| **7.0+ (API 24+)** | ✅ 完全支持 | 系统自带 ICU |
| **4.0-6.0 (API 14-23)** | ⚠️ 需要打包 | 需要打包 ICU 库 (~30MB) |

**推荐**: 最低 API Level 24

### 🛠️ NDK 配置

#### 方法 1: 使用 CMake（推荐）

**CMakeLists.txt**:
```cmake
cmake_minimum_required(VERSION 3.4.1)

# 添加 ICU 分词器库
add_library(icu_tokenizer SHARED
    src/main/cpp/icu_tokenizer.c
)

# 链接 ICU 库
target_link_libraries(icu_tokenizer
    icuuc      # ICU Unicode Common
    icui18n    # ICU Internationalization
    log        # Android 日志
)
```

**build.gradle (Module)**:
```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86', 'x86_64'
        }
        
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++11"
                arguments "-DANDROID_STL=c++_shared"
            }
        }
    }
    
    externalNativeBuild {
        cmake {
            path "src/main/cpp/CMakeLists.txt"
            version "3.18.1"
        }
    }
}
```

#### 方法 2: 使用 Android.mk

**Android.mk**:
```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := icu_tokenizer
LOCAL_SRC_FILES := icu_tokenizer.c
LOCAL_LDLIBS := -licuuc -licui18n -llog
include $(BUILD_SHARED_LIBRARY)
```

**Application.mk**:
```makefile
APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_PLATFORM := android-24
APP_STL := c++_shared
```

### 📝 Java/Kotlin 调用示例

#### Java 示例

```java
public class ICUTokenizer {
    static {
        System.loadLibrary("icu_tokenizer");
    }
    
    // JNI 方法声明
    public native void init(String locale);
    public native String[] tokenize(String text);
    public native void cleanup();
    
    // 使用示例
    public void testTokenizer() {
        ICUTokenizer tokenizer = new ICUTokenizer();
        tokenizer.init("zh");
        
        String[] tokens = tokenizer.tokenize("这是中文测试");
        for (String token : tokens) {
            Log.d("ICU", "Token: " + token);
        }
        
        tokenizer.cleanup();
    }
}
```

#### Kotlin 示例

```kotlin
class ICUTokenizer {
    companion object {
        init {
            System.loadLibrary("icu_tokenizer")
        }
    }
    
    // JNI 方法
    external fun init(locale: String)
    external fun tokenize(text: String): Array<String>
    external fun cleanup()
    
    // 使用示例
    fun testTokenizer() {
        init("zh")
        
        val tokens = tokenize("这是中文测试")
        tokens.forEach { token ->
            Log.d("ICU", "Token: $token")
        }
        
        cleanup()
    }
}
```

### 🔧 JNI 桥接代码

需要创建 JNI 包装器来连接 Java 和 C 代码：

**icu_tokenizer_jni.c**:
```c
#include <jni.h>
#include "icu_tokenizer.h"
#include <android/log.h>

#define TAG "ICUTokenizer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

JNIEXPORT void JNICALL
Java_com_example_app_ICUTokenizer_init(JNIEnv *env, jobject thiz, jstring locale) {
    const char *locale_str = (*env)->GetStringUTFChars(env, locale, 0);
    icu_tokenizer_init(locale_str);
    (*env)->ReleaseStringUTFChars(env, locale, locale_str);
}

JNIEXPORT jobjectArray JNICALL
Java_com_example_app_ICUTokenizer_tokenize(JNIEnv *env, jobject thiz, jstring text) {
    const char *text_str = (*env)->GetStringUTFChars(env, text, 0);
    
    ICUTokenList* tokens = icu_tokenize(text_str);
    (*env)->ReleaseStringUTFChars(env, text, text_str);
    
    if (!tokens) {
        return NULL;
    }
    
    // 创建 Java String 数组
    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    jobjectArray result = (*env)->NewObjectArray(env, tokens->count, stringClass, NULL);
    
    for (size_t i = 0; i < tokens->count; i++) {
        jstring jtoken = (*env)->NewStringUTF(env, tokens->tokens[i]);
        (*env)->SetObjectArrayElement(env, result, i, jtoken);
        (*env)->DeleteLocalRef(env, jtoken);
    }
    
    icu_free_tokens(tokens);
    return result;
}

JNIEXPORT void JNICALL
Java_com_example_app_ICUTokenizer_cleanup(JNIEnv *env, jobject thiz) {
    icu_tokenizer_cleanup();
}
```

### ⚠️ Android 注意事项

1. **最低 API Level**: 建议设置为 24（Android 7.0）
2. **多架构支持**: 需要编译多个 ABI 版本（armeabi-v7a, arm64-v8a 等）
3. **APK 体积**: 
   - API 24+: 几乎无影响（使用系统库）
   - API < 24: 需要打包 ICU 库，增加约 30MB
4. **权限**: 不需要特殊权限

---

## 🎯 推荐配置

### iOS 项目

```
✅ 最低版本: iOS 10.0
✅ 链接选项: -licucore
✅ 体积影响: 0 KB（系统库）
✅ 推荐度: ⭐⭐⭐⭐⭐
```

### Android 项目

```
✅ 最低 API: 24 (Android 7.0)
✅ NDK 版本: r21+
✅ 体积影响: 0 KB（系统库）
✅ 推荐度: ⭐⭐⭐⭐⭐
```

### 如果需要支持旧版 Android (API < 24)

```
⚠️ 需要打包 ICU: ~30 MB
⚠️ 推荐度: ⭐⭐⭐ (考虑使用 N-gram 替代)
```

---

## 📊 性能对比

| 平台 | ICU 方案 | N-gram 方案 |
|------|---------|------------|
| **iOS** | ⭐⭐⭐⭐⭐ 推荐 | ⭐⭐⭐ 可选 |
| **Android 7.0+** | ⭐⭐⭐⭐⭐ 推荐 | ⭐⭐⭐ 可选 |
| **Android < 7.0** | ⭐⭐ 体积大 | ⭐⭐⭐⭐⭐ 推荐 |

---

## 🆘 常见问题

### Q1: iOS 编译时找不到 ICU 头文件？
**A**: iOS 系统已经包含 ICU，使用 `#include <unicode/ubrk.h>` 即可，无需额外配置。

### Q2: Android 链接时报错 "undefined reference to ICU functions"？
**A**: 确保在 CMakeLists.txt 或 Android.mk 中添加了 `-licuuc -licui18n`。

### Q3: Android API 23 能用吗？
**A**: 可以，但需要打包 ICU 库（约 30MB），建议使用 N-gram 方案替代。

### Q4: 如何减小 APK 体积？
**A**: 
1. 最低 API Level 设置为 24
2. 使用 APK 分包（针对不同架构）
3. 考虑使用纯 C 实现的 N-gram 方案

---

## 📚 更多资源

- **ICU Android 文档**: https://developer.android.com/guide/topics/resources/internationalization
- **ICU iOS 文档**: https://developer.apple.com/documentation/foundation/nslinguistictagger
- **NDK 文档**: https://developer.android.com/ndk/guides
