cmake_minimum_required(VERSION 3.18.1)

project("fusionsearch")

# 设置 C 标准
set(CMAKE_C_STANDARD 99)

# FusionSearch C 源文件路径
set(FUSION_ROOT "${CMAKE_SOURCE_DIR}/../../../")

# 添加所有 C 源文件（排除 main.c）
file(GLOB FUSION_SRC
    "${FUSION_ROOT}/src/trie.c"
    "${FUSION_ROOT}/src/tokenizer.c"
    "${FUSION_ROOT}/src/index.c"
    "${FUSION_ROOT}/src/search.c"
    "${FUSION_ROOT}/src/bm25.c"
    "${FUSION_ROOT}/src/snippet.c"
    "${FUSION_ROOT}/src/file_loader.c"
    "${FUSION_ROOT}/src/cjk_tokenizer.c"
    "${FUSION_ROOT}/src/vector_index.c"
    "${FUSION_ROOT}/src/utils.c"
)

# JNI 桥接层
set(JNI_SRC "${CMAKE_SOURCE_DIR}/../fusion_jni.c")

# 创建共享库
add_library(fusion SHARED
    ${FUSION_SRC}
    ${JNI_SRC}
)

# 包含头文件目录
target_include_directories(fusion PRIVATE
    "${FUSION_ROOT}/include"
)

# 链接库
target_link_libraries(fusion
    log      # Android 日志库
    m        # 数学库
)

# Android 7.0+ (API 24+) 支持 ICU
if(ANDROID_PLATFORM_LEVEL GREATER_EQUAL 24)
    message(STATUS "Enabling ICU support (Android 7.0+)")
    target_compile_definitions(fusion PRIVATE ENABLE_ICU)
    target_link_libraries(fusion icuuc icui18n)
else()
    message(WARNING "ICU not available (Android < 7.0), CJK tokenizer will be disabled")
endif()

# 编译选项
target_compile_options(fusion PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# 导出符号（用于 JNI）
set_target_properties(fusion PROPERTIES
    LINK_FLAGS "-Wl,--exclude-libs,ALL"
)
