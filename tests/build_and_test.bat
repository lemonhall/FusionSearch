@echo off
REM FusionSearch 测试套件 - Windows 批处理脚本
REM 使用 WSL 编译和运行测试

setlocal enabledelayedexpansion

set "TEST_DIR=/mnt/e/development/FusionSearch/tests"
set "ACTION=%~1"

if "%ACTION%"=="" set "ACTION=all"

echo.
echo ╔════════════════════════════════════════╗
echo ║   FusionSearch Test Suite Runner      ║
echo ╚════════════════════════════════════════╝
echo.

if "%ACTION%"=="build" goto BUILD
if "%ACTION%"=="run" goto RUN
if "%ACTION%"=="all" goto ALL
if "%ACTION%"=="clean" goto CLEAN
if "%ACTION%"=="quick" goto QUICK
if "%ACTION%"=="help" goto HELP
if "%ACTION%"=="--help" goto HELP
if "%ACTION%"=="-h" goto HELP

echo [错误] 未知选项: %ACTION%
echo 使用 'build_and_test.bat help' 查看帮助
exit /b 1

:BUILD
echo [信息] 编译测试套件...
wsl bash -c "cd %TEST_DIR% && make clean && make"
if errorlevel 1 (
    echo [错误] 编译失败
    exit /b 1
)
echo [成功] 编译完成！
goto END

:RUN
echo [信息] 运行所有测试...
wsl bash -c "cd %TEST_DIR% && make run"
goto END

:ALL
echo [信息] 清理旧文件...
wsl bash -c "cd %TEST_DIR% && make clean"
echo.
echo [信息] 编译测试套件...
wsl bash -c "cd %TEST_DIR% && make"
if errorlevel 1 (
    echo [错误] 编译失败
    exit /b 1
)
echo [成功] 编译完成！
echo.
echo [信息] 运行所有测试...
wsl bash -c "cd %TEST_DIR% && make run"
goto END

:CLEAN
echo [信息] 清理构建文件...
wsl bash -c "cd %TEST_DIR% && make clean"
echo [成功] 清理完成！
goto END

:QUICK
echo [信息] 运行快速测试...
wsl bash -c "cd %TEST_DIR% && make quick"
goto END

:HELP
echo FusionSearch 测试套件脚本 (Windows版)
echo.
echo 使用方法: build_and_test.bat [选项]
echo.
echo 选项:
echo   all       - 编译并运行所有测试 (默认)
echo   build     - 仅编译
echo   run       - 仅运行测试
echo   clean     - 清理构建文件
echo   quick     - 快速测试
echo   help      - 显示此帮助信息
echo.
echo 示例:
echo   build_and_test.bat              # 编译并运行所有测试
echo   build_and_test.bat build        # 仅编译
echo   build_and_test.bat quick        # 快速测试
echo.
echo 注意: 需要安装 WSL (Windows Subsystem for Linux)
goto END

:END
echo.
endlocal
exit /b 0
