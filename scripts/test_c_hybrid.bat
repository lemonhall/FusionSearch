@echo off
REM C 程序编译和测试 - WSL2 脚本
REM 用法: 直接双击运行此文件

echo ====================================
echo 混合检索测试 (C 程序 - WSL2)
echo ====================================
echo.

echo [1/3] 清理旧文件...
wsl bash -c "cd /mnt/e/development/FusionSearch && make clean 2>/dev/null"

echo [2/3] 编译 C 程序...
wsl bash -c "cd /mnt/e/development/FusionSearch && gcc -Wall -Wextra -std=c99 -g -D_POSIX_C_SOURCE=199309L -Iinclude test_hybrid.c src/vector_index.c src/file_loader.c src/index.c src/tokenizer.c src/search.c src/bm25.c src/snippet.c src/utils.c src/cjk_tokenizer.c -o test_hybrid -lm"

if %errorlevel% neq 0 (
    echo.
    echo [错误] 编译失败！
    pause
    exit /b 1
)

echo [3/3] 运行测试...
echo.

wsl bash -c "cd /mnt/e/development/FusionSearch && ./test_hybrid"

echo.
echo ====================================
echo 测试完成
echo ====================================
pause
