@echo off
REM 向量检索交互式测试 - WSL2 启动脚本
REM 用法: 直接双击运行此文件

echo ====================================
echo 向量检索测试 (WSL2)
echo ====================================
echo.

REM 检查 numpy 是否已安装
wsl bash -c "python3 -c 'import numpy' 2>/dev/null"
if %errorlevel% neq 0 (
    echo [提示] 首次运行需要安装 numpy...
    echo.
    wsl bash -c "pip3 install numpy --user -q"
    echo.
)

REM 进入 WSL 并运行测试
echo 启动交互式查询测试...
echo 提示: 输入 'quit' 或按 Ctrl+C 退出
echo.

wsl bash -c "cd /mnt/e/development/FusionSearch && python3 query_test.py"

echo.
echo ====================================
echo 测试结束
echo ====================================
pause
