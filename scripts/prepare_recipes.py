#!/usr/bin/env python3
"""
菜谱数据准备工具
功能：
1. 读取 recipes/*.json 文件
2. 提取 name 和 content 字段
3. 生成 CSV 文件供后续处理

使用：
    python prepare_recipes.py
"""

import json
import os
import glob
import csv

def extract_recipes(recipes_dir='recipes', output_file='data/recipes.jsonl'):
    """
    提取所有菜谱到 JSONL 文件（修正：直接用 JSONL 避免 CSV 多行问题）
    
    Args:
        recipes_dir: JSON 文件目录
        output_file: 输出 JSONL 文件路径
    """
    print("=" * 60)
    print("菜谱数据提取工具")
    print("=" * 60)
    print()
    
    # 确保输出目录存在
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    
    # 查找所有 JSON 文件
    json_files = sorted(glob.glob(os.path.join(recipes_dir, '*.json')))
    
    if not json_files:
        print(f"❌ 未找到 JSON 文件: {recipes_dir}/")
        return False
    
    print(f"📁 找到 {len(json_files)} 个 JSON 文件")
    print()
    
    recipes = []
    errors = []
    
    # 读取每个 JSON 文件
    for json_file in json_files:
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # 提取字段（修正路径）
            recipe_name = data.get('original_recipe', {}).get('recipe_location', {}).get('name', '')
            content = data.get('original_recipe', {}).get('content', '')
            
            if not recipe_name or not content:
                errors.append(f"{os.path.basename(json_file)}: 缺少必要字段")
                continue
            
            recipes.append({
                'title': recipe_name,
                'content': content
            })
            
            print(f"✓ {os.path.basename(json_file):12s} {recipe_name[:40]}")
            
        except Exception as e:
            errors.append(f"{os.path.basename(json_file)}: {e}")
    
    print()
    print(f"✓ 成功提取 {len(recipes)} 个菜谱")
    
    if errors:
        print(f"⚠ {len(errors)} 个文件出错：")
        for error in errors[:5]:  # 只显示前5个错误
            print(f"  - {error}")
        if len(errors) > 5:
            print(f"  ... 还有 {len(errors) - 5} 个错误")
    
    print()
    
    # 写入 JSONL（修正：直接 JSONL 格式，避免多行文本问题）
    print(f"💾 写入 JSONL: {output_file}")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        for recipe in recipes:
            # 每行一个 JSON 对象
            f.write(json.dumps(recipe, ensure_ascii=False) + '\n')
    
    print(f"✓ 完成！")
    print()
    
    # 统计信息
    total_chars = sum(len(r['content']) for r in recipes)
    avg_chars = total_chars // len(recipes) if recipes else 0
    
    print("📊 统计信息：")
    print(f"  文档数量: {len(recipes)}")
    print(f"  总字符数: {total_chars:,}")
    print(f"  平均长度: {avg_chars:,} 字符")
    print()
    
    return True


if __name__ == "__main__":
    success = extract_recipes()
    
    if success:
        print("=" * 60)
        print("✅ 数据准备完成！")
        print()
        print("下一步：")
        print("  python build_vector_index.py")
        print("=" * 60)
    else:
        print("❌ 数据准备失败")
        exit(1)
