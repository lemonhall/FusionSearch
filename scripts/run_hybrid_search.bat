@echo off
chcp 65001 >nul
echo ========================================
echo 混合搜索引擎 - 一键启动
echo ========================================
echo.

echo [1/3] 编译C程序...
wsl bash -c "cd /mnt/e/development/FusionSearch && make demo"
if errorlevel 1 (
    echo ❌ 编译失败
    pause
    exit /b 1
)

echo.
echo [2/3] 加载数据并导出向量...
wsl bash -c "cd /mnt/e/development/FusionSearch && ./demo_hybrid recipes_with_embeddings.jsonl vectors.bin documents.json"
if errorlevel 1 (
    echo ❌ 数据加载失败
    pause
    exit /b 1
)

echo.
echo [3/3] 启动Python搜索引擎...
python search_api.py

pause
