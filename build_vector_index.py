#!/usr/bin/env python3
"""
向量索引构建工具
功能：
1. 读取 CSV 文件
2. 调用 Silicon Flow API 生成 embedding
3. 生成 JSONL 文件（带向量）供 C 程序加载

使用：
    export SILICONFLOW_API_KEY='your-key'
    python build_vector_index.py
"""

import os
import json
import csv
import requests
import time
from typing import List

# API 配置
API_URL = "https://api.siliconflow.cn/v1/embeddings"
API_KEY = os.getenv("SILICONFLOW_API_KEY", "")
EMBEDDING_MODEL = "BAAI/bge-large-zh-v1.5"  # 1024维


def generate_embedding(text: str, api_key: str) -> List[float]:
    """生成文本向量"""
    headers = {
        'Authorization': f'Bearer {api_key}',
        'Content-Type': 'application/json'
    }
    
    payload = {
        'model': EMBEDDING_MODEL,
        'input': text
    }
    
    response = requests.post(API_URL, headers=headers, json=payload, timeout=30)
    response.raise_for_status()
    
    data = response.json()
    return data['data'][0]['embedding']


def build_vector_index(jsonl_file='data/recipes.jsonl', 
                       output_file='data/recipes_vector.jsonl'):
    """构建向量索引"""
    
    print("=" * 60)
    print("向量索引构建工具")
    print("=" * 60)
    print()
    
    # 检查 API 密钥
    if not API_KEY:
        print("❌ 错误：未设置 API 密钥")
        print()
        print("请设置环境变量：")
        print("  Linux/Mac: export SILICONFLOW_API_KEY='your-api-key'")
        print("  Windows:   $env:SILICONFLOW_API_KEY='your-api-key'")
        print()
        return False
    
    print(f"✓ API密钥: {API_KEY[:10]}...")
    print(f"✓ 模型: {EMBEDDING_MODEL}")
    print()
    
    # 读取 JSONL
    if not os.path.exists(jsonl_file):
        print(f"❌ 文件不存在: {jsonl_file}")
        print("   请先运行: python prepare_recipes.py")
        return False
    
    print(f"📖 读取文件: {jsonl_file}")
    
    recipes = []
    with open(jsonl_file, 'r', encoding='utf-8') as f:
        for line in f:
            recipes.append(json.loads(line))
    
    print(f"✓ 读取 {len(recipes)} 个菜谱")
    print()
    
    # 测试 API
    print("🧪 测试 API 连接...")
    try:
        test_embedding = generate_embedding("测试", API_KEY)
        print(f"✓ API 正常（维度: {len(test_embedding)}）")
    except Exception as e:
        print(f"❌ API 错误: {e}")
        return False
    
    print()
    
    # 生成向量
    print("🚀 开始生成向量...")
    print()
    
    success_count = 0
    error_count = 0
    
    with open(output_file, 'w', encoding='utf-8') as f:
        for i, recipe in enumerate(recipes, 1):
            title = recipe['title']
            content = recipe['content']
            
            # 进度显示
            print(f"[{i}/{len(recipes)}] {title[:40]:40s} ", end='', flush=True)
            
            try:
                # 生成 embedding（截断长文本，防止超限）
                start_time = time.time()
                
                # API 限制约 4096 字节（UTF-8编码），保守估计按 1500 字符截断
                # 中文字符每个约3字节，1500字符约4500字节，安全范围
                text_for_embedding = content
                if len(content) > 1500:
                    text_for_embedding = content[:1500]
                    print(f"[截断] ", end='', flush=True)
                
                # 再次检查字节数（双重保险）
                text_bytes = text_for_embedding.encode('utf-8')
                if len(text_bytes) > 4000:
                    # 按字节截断
                    text_for_embedding = text_bytes[:4000].decode('utf-8', errors='ignore')
                    print(f"[字节] ", end='', flush=True)
                
                embedding = generate_embedding(text_for_embedding, API_KEY)
                elapsed = time.time() - start_time
                
                # 写入 JSONL
                json_obj = {
                    'title': title,
                    'content': content,
                    'embedding': embedding
                }
                f.write(json.dumps(json_obj, ensure_ascii=False) + '\n')
                
                success_count += 1
                print(f"✓ {elapsed:.1f}s")
                
                # 避免 API 限流
                time.sleep(0.1)
                
            except Exception as e:
                error_count += 1
                print(f"❌ {e}")
    
    print()
    print("=" * 60)
    print(f"✅ 构建完成！")
    print(f"   成功: {success_count}")
    print(f"   失败: {error_count}")
    print(f"   输出: {output_file}")
    print("=" * 60)
    print()
    
    return success_count > 0


if __name__ == "__main__":
    success = build_vector_index()
    
    if success:
        print("下一步：")
        print("  make && ./search_engine")
        print("  或")
        print("  python query_test.py")
    else:
        exit(1)
