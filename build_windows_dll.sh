#!/bin/bash
# WSL2 交叉编译 Windows DLL 脚本

set -e  # 遇到错误立即退出

echo "🔍 检查 MinGW-w64 交叉编译器..."

# 检查是否安装了 MinGW-w64
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "❌ MinGW-w64 未安装"
    echo ""
    echo "请运行以下命令安装："
    echo "  sudo apt update"
    echo "  sudo apt install -y mingw-w64"
    echo ""
    exit 1
fi

echo "✅ MinGW-w64 已安装"
echo ""

# 交叉编译器信息
CC=x86_64-w64-mingw32-gcc
AR=x86_64-w64-mingw32-ar
RANLIB=x86_64-w64-mingw32-ranlib

# 编译参数
CFLAGS="-Wall -Wextra -std=c99 -g -Iinclude -D_POSIX_C_SOURCE=199309L -DENABLE_ICU -fPIC"
LDFLAGS="-lm"

# 源文件
SOURCES="src/trie.c src/tokenizer.c src/index.c src/search.c src/bm25.c src/snippet.c src/file_loader.c src/cjk_tokenizer.c src/vector_index.c src/utils.c"

echo "🔨 开始交叉编译 Windows DLL..."
echo "编译器: $CC"
echo ""

# 清理旧文件
rm -f *.o fusion.dll

# 编译目标文件
echo "📦 编译目标文件..."
for src in $SOURCES; do
    obj=$(basename $src .c).o
    echo "  $src -> $obj"
    $CC $CFLAGS -c $src -o $obj
done

echo ""
echo "🔗 链接生成 DLL..."

# 链接生成 DLL
$CC -shared *.o -o fusion.dll $LDFLAGS -static-libgcc

# 清理中间文件
rm -f *.o

echo ""
if [ -f "fusion.dll" ]; then
    echo "✅ 编译成功！"
    echo ""
    echo "输出文件:"
    ls -lh fusion.dll
    echo ""
    echo "文件类型:"
    file fusion.dll
    echo ""
    echo "🎉 Windows DLL 已生成: fusion.dll"
    echo "   可以直接在 Windows 的 Python 中使用！"
else
    echo "❌ 编译失败"
    exit 1
fi
