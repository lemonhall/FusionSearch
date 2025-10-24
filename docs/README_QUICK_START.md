# 快速开始指南（Windows + WSL2）

本指南适用于在 Windows 下操作，但需要在 WSL2 中运行测试的场景。

---

## 🚀 一键测试（双击运行）

### 方案 A：Python 交互式测试（推荐）

**直接双击运行**：
```
test_query.bat
```

**功能**：
- 自动检查并安装 numpy
- 启动交互式向量检索
- 输入查询，返回 Top-5 结果

**示例**：
```
🔍 请输入查询: 如何制作清汤

🎯 检索结果（Top-5）：
#1  相似度: 0.9234
    标题: 普通白色清汤
    内容: 普通白色清汤制作4夸脱...
```

---

### 方案 B：C 程序测试

**直接双击运行**：
```
test_c_hybrid.bat
```

**功能**：
- 自动编译 C 程序（WSL2 gcc）
- 运行混合检索测试
- 展示 BM25 + 向量检索结果

---

## 📝 手动运行（PowerShell/CMD）

### Python 测试

```powershell
# 进入目录
cd e:\development\FusionSearch

# 运行测试（WSL2）
wsl bash -c "cd /mnt/e/development/FusionSearch && python3 query_test.py"
```

### C 程序测试

```powershell
# 编译
wsl bash -c "cd /mnt/e/development/FusionSearch && make clean && make"

# 运行
wsl bash -c "cd /mnt/e/development/FusionSearch && ./search_engine"
```

---

## 🔧 环境检查

### 检查 WSL2 是否可用

```powershell
wsl --version
```

预期输出：
```
WSL 版本: 2.x.x.x
```

### 检查 Python 环境（WSL2）

```powershell
wsl bash -c "python3 --version"
```

### 检查 gcc 编译器（WSL2）

```powershell
wsl bash -c "gcc --version"
```

---

## 📊 数据文件位置

### Windows 路径
```
E:\development\FusionSearch\data\recipes_vector.jsonl
```

### WSL2 路径
```
/mnt/e/development/FusionSearch/data/recipes_vector.jsonl
```

**数据统计**：
- 总文档：50 个
- 向量维度：1024
- 文件大小：~10 MB

---

## ⚡ 常见问题

### 1. "wsl 命令未找到"

**解决**：
- 确认 WSL2 已安装：`wsl --install`
- 重启计算机

### 2. "找不到 Python3"

**解决**（WSL2 内）：
```bash
wsl bash -c "sudo apt update && sudo apt install python3 python3-pip -y"
```

### 3. "找不到 gcc"

**解决**（WSL2 内）：
```bash
wsl bash -c "sudo apt update && sudo apt install build-essential -y"
```

### 4. "numpy 导入失败"

**解决**（WSL2 内）：
```bash
wsl bash -c "pip3 install numpy --user"
```

---

## 🎯 测试示例查询

试试这些查询：

1. **基础汤料**
   - "如何制作清汤"
   - "褐色高汤的做法"

2. **特定酱汁**
   - "番茄酱的配方"
   - "荷兰酱怎么做"

3. **烹饪技巧**
   - "如何澄清汤汁"
   - "面糊酱的制作方法"

---

## 📁 批处理脚本说明

### test_query.bat

**功能**：Python 交互式测试
- 自动安装依赖
- 启动查询界面
- 实时向量检索

### test_c_hybrid.bat

**功能**：C 程序测试
- 自动编译 C 代码
- 运行混合检索演示
- 展示 BM25 + 向量结果

---

## 💡 小贴士

### 快速切换到 WSL

```powershell
# 直接进入 WSL bash
wsl

# 切换到项目目录
cd /mnt/e/development/FusionSearch
```

### 在 WSL 中直接测试

```bash
# Python 测试
python3 query_test.py

# C 程序编译和运行
make clean && make && ./search_engine
```

---

## 🔗 相关文档

- [向量检索详细指南](README_VECTOR.md)
- [技术实现文档](VECTOR_USAGE.md)
- [项目主文档](README.md)

---

**创建时间**: 2025-10-24  
**适用平台**: Windows 10/11 + WSL2  
**测试环境**: WSL2 Ubuntu
