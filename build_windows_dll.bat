@echo off
REM Windows 批处理文件 - 调用 WSL2 交叉编译 DLL

echo ========================================
echo    WSL2 交叉编译 Windows DLL
echo ========================================
echo.

REM 检查 WSL2 是否可用
wsl bash --version >nul 2>&1
if errorlevel 1 (
    echo ❌ WSL2 未安装或未启动
    echo.
    echo 请先安装 WSL2：https://aka.ms/wsl2
    pause
    exit /b 1
)

echo ✅ WSL2 已就绪
echo.

REM 转换 Windows 路径为 WSL 路径
set "WSL_PATH=/mnt/e/development/FusionSearch"

echo 🔨 开始交叉编译...
echo.

REM 调用 WSL2 执行编译脚本
wsl bash -c "cd %WSL_PATH% && chmod +x build_windows_dll.sh && ./build_windows_dll.sh"

if errorlevel 1 (
    echo.
    echo ❌ 编译失败
    pause
    exit /b 1
)

echo.
echo ========================================
echo 🎉 编译完成！
echo ========================================
echo.
echo 生成的文件：fusion.dll
echo 现在可以在 Windows Python 中直接使用！
echo.
pause
