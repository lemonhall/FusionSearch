#!/bin/bash
# FusionSearch 测试套件编译和运行脚本
# 使用方法: ./build_and_test.sh [选项]
#
# 选项:
#   all       - 编译并运行所有测试（默认）
#   build     - 仅编译
#   run       - 仅运行测试
#   clean     - 清理构建文件
#   quick     - 快速测试
#   memcheck  - 内存泄漏检测

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# 检查是否在 tests 目录
if [ ! -f "Makefile" ]; then
    print_error "请在 tests/ 目录下运行此脚本"
    exit 1
fi

# 解析命令行参数
ACTION=${1:-all}

case "$ACTION" in
    build)
        print_info "开始编译测试套件..."
        make clean
        make
        print_success "编译完成！"
        ;;
    
    run)
        print_info "运行所有测试..."
        make run
        ;;
    
    all)
        print_info "编译并运行所有测试..."
        make clean
        make
        echo ""
        print_success "编译完成！"
        echo ""
        make run
        ;;
    
    clean)
        print_info "清理构建文件..."
        make clean
        print_success "清理完成！"
        ;;
    
    quick)
        print_info "运行快速测试..."
        make quick
        ;;
    
    memcheck)
        if ! command -v valgrind &> /dev/null; then
            print_warning "未安装 valgrind，跳过内存检测"
            print_info "安装方法: sudo apt-get install valgrind"
            exit 1
        fi
        print_info "运行内存泄漏检测..."
        make memcheck
        ;;
    
    help|--help|-h)
        echo "FusionSearch 测试套件脚本"
        echo ""
        echo "使用方法: $0 [选项]"
        echo ""
        echo "选项:"
        echo "  all       - 编译并运行所有测试（默认）"
        echo "  build     - 仅编译"
        echo "  run       - 仅运行测试"
        echo "  clean     - 清理构建文件"
        echo "  quick     - 快速测试"
        echo "  memcheck  - 内存泄漏检测（需要 valgrind）"
        echo "  help      - 显示此帮助信息"
        echo ""
        echo "示例:"
        echo "  $0              # 编译并运行所有测试"
        echo "  $0 build        # 仅编译"
        echo "  $0 quick        # 快速测试"
        echo "  $0 memcheck     # 内存检测"
        ;;
    
    *)
        print_error "未知选项: $ACTION"
        echo "使用 '$0 help' 查看帮助"
        exit 1
        ;;
esac

exit 0
