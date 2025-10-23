# ICU 分词器测试项目

## 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

## ICU 库配置（使用 pkg-config）
ICU_CFLAGS := $(shell pkg-config --cflags icu-uc icu-i18n 2>/dev/null)
ICU_LIBS := $(shell pkg-config --libs icu-uc icu-i18n 2>/dev/null)

## 如果 pkg-config 失败，使用默认值
ifeq ($(ICU_CFLAGS),)
    ICU_CFLAGS = -I/usr/include
endif

ifeq ($(ICU_LIBS),)
    ICU_LIBS = -licuuc -licui18n
endif

## 源文件
SOURCES = icu_tokenizer.c test_main.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = test_icu

## 默认目标
all: check_icu $(TARGET)

## 检查 ICU 库是否安装
check_icu:
	@echo "正在检查 ICU 库..."
	@pkg-config --exists icu-uc icu-i18n 2>/dev/null || \
		(echo ""; \
		 echo "❌ 错误：未找到 ICU 库！"; \
		 echo ""; \
		 echo "请安装 ICU 开发库："; \
		 echo "  Ubuntu/Debian: sudo apt-get install libicu-dev"; \
		 echo "  CentOS/RHEL:   sudo dnf install libicu-devel"; \
		 echo "  macOS:         brew install icu4c"; \
		 echo "  Windows:       使用 vcpkg install icu"; \
		 echo ""; \
		 exit 1)
	@echo "✅ ICU 库已安装"
	@pkg-config --modversion icu-uc 2>/dev/null | xargs -I {} echo "   版本: {}"

## 编译对象文件
%.o: %.c
	$(CC) $(CFLAGS) $(ICU_CFLAGS) -c $< -o $@

## 链接可执行文件
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(ICU_LIBS)
	@echo ""
	@echo "✅ 编译成功！可执行文件: $(TARGET)"
	@echo "   运行测试: make run"
	@echo ""

## 运行测试
run: $(TARGET)
	./$(TARGET)

## 清理
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✅ 清理完成"

## 显示帮助
help:
	@echo "ICU 分词器测试 - Makefile 命令"
	@echo ""
	@echo "make          - 编译项目"
	@echo "make run      - 编译并运行测试"
	@echo "make clean    - 清理编译文件"
	@echo "make check_icu - 检查 ICU 库安装情况"
	@echo "make help     - 显示此帮助信息"

.PHONY: all check_icu run clean help
