#!/usr/bin/env python3
"""
向量生成工具 - 调用 Silicon Flow API
用途：将文本文档转换为带 embedding 的 JSONL 格式

依赖：
    pip install requests

使用方法：
    1. 设置环境变量 SILICONFLOW_API_KEY
    2. 运行: python generate_embeddings.py
"""

import os
import json
import requests
import time
from typing import List, Dict

# API 配置
API_URL = "https://api.siliconflow.cn/v1/embeddings"
API_KEY = os.getenv("SILICONFLOW_API_KEY", "")  # 从环境变量读取

# 模型选择
EMBEDDING_MODEL = "BAAI/bge-large-zh-v1.5"  # 1024维，中文优化
# EMBEDDING_MODEL = "BAAI/bge-small-zh-v1.5"  # 512维，轻量级


def generate_embedding(text: str, api_key: str) -> List[float]:
    """
    调用 Silicon Flow API 生成文本向量
    
    Args:
        text: 输入文本
        api_key: API密钥
        
    Returns:
        向量列表（浮点数）
    """
    if not api_key:
        raise ValueError("API密钥未设置！请设置环境变量 SILICONFLOW_API_KEY")
    
    headers = {
        'Authorization': f'Bearer {api_key}',
        'Content-Type': 'application/json'
    }
    
    payload = {
        'model': EMBEDDING_MODEL,
        'input': text
    }
    
    try:
        response = requests.post(API_URL, headers=headers, json=payload, timeout=30)
        response.raise_for_status()
        
        data = response.json()
        embedding = data['data'][0]['embedding']
        
        return embedding
    
    except requests.exceptions.HTTPError as e:
        print(f"❌ HTTP错误: {e}")
        print(f"   响应内容: {e.response.text}")
        raise
    except requests.exceptions.Timeout:
        print("❌ 请求超时")
        raise
    except Exception as e:
        print(f"❌ 未知错误: {e}")
        raise


def test_api():
    """测试 API 连接和模型"""
    print("=" * 60)
    print("Silicon Flow Embedding API 测试")
    print("=" * 60)
    print()
    
    # 检查 API 密钥
    if not API_KEY:
        print("❌ 错误：未设置 API 密钥")
        print()
        print("请设置环境变量：")
        print("  Linux/Mac: export SILICONFLOW_API_KEY='your-api-key'")
        print("  Windows:   set SILICONFLOW_API_KEY=your-api-key")
        print()
        return False
    
    print(f"✓ API密钥已设置: {API_KEY[:10]}...")
    print(f"✓ 使用模型: {EMBEDDING_MODEL}")
    print()
    
    # 测试文本
    test_texts = [
        "Silicon flow embedding online: fast, affordable, and high-quality embedding services. come try it out!",
        "机器学习和深度学习很有趣",
        "番茄炒蛋是一道简单快手的家常菜"
    ]
    
    for i, text in enumerate(test_texts, 1):
        print(f"测试 {i}: {text[:50]}...")
        
        try:
            start_time = time.time()
            embedding = generate_embedding(text, API_KEY)
            elapsed = time.time() - start_time
            
            print(f"  ✓ 成功生成向量")
            print(f"  维度: {len(embedding)}")
            print(f"  前5个值: {embedding[:5]}")
            print(f"  耗时: {elapsed:.2f}s")
            print()
            
        except Exception as e:
            print(f"  ❌ 失败: {e}")
            print()
            return False
    
    print("=" * 60)
    print("✅ 所有测试通过！API 工作正常")
    print("=" * 60)
    print()
    return True


def generate_jsonl_from_csv(input_file: str, output_file: str):
    """
    从 CSV 文件生成带 embedding 的 JSONL 文件
    
    CSV 格式: title,content
    JSONL 格式: {"title": "...", "content": "...", "embedding": [...]}
    """
    print(f"📖 读取文件: {input_file}")
    
    documents = []
    
    # 读取 CSV
    with open(input_file, 'r', encoding='utf-8') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            
            # 跳过空行和注释
            if not line or line.startswith('#'):
                continue
            
            # 跳过表头
            if line_num == 1 and 'title' in line.lower():
                continue
            
            # 解析 CSV（简单分割，不处理引号）
            parts = line.split(',', 1)
            if len(parts) != 2:
                print(f"  ⚠ 跳过第 {line_num} 行（格式错误）")
                continue
            
            title, content = parts
            documents.append({
                'title': title.strip(),
                'content': content.strip()
            })
    
    print(f"✓ 读取 {len(documents)} 个文档")
    print()
    
    # 生成 embedding 并写入 JSONL
    print(f"🚀 开始生成 embedding...")
    print()
    
    with open(output_file, 'w', encoding='utf-8') as f:
        for i, doc in enumerate(documents, 1):
            print(f"[{i}/{len(documents)}] {doc['title'][:40]}...")
            
            try:
                # 生成 embedding（使用 content 字段）
                embedding = generate_embedding(doc['content'], API_KEY)
                
                # 构造 JSON 对象
                json_obj = {
                    'title': doc['title'],
                    'content': doc['content'],
                    'embedding': embedding
                }
                
                # 写入 JSONL
                f.write(json.dumps(json_obj, ensure_ascii=False) + '\n')
                
                print(f"  ✓ 完成（{len(embedding)}维）")
                
                # 避免 API 限流
                time.sleep(0.1)
                
            except Exception as e:
                print(f"  ❌ 失败: {e}")
    
    print()
    print(f"✅ 完成！输出文件: {output_file}")
    print()


if __name__ == "__main__":
    # 测试 API
    if not test_api():
        exit(1)
    
    # 示例：生成 embedding
    print()
    print("=" * 60)
    print("准备从 CSV 生成 JSONL + Embedding")
    print("=" * 60)
    print()
    
    # 用户输入
    input_file = input("请输入 CSV 文件路径（直接回车跳过）: ").strip()
    
    if input_file:
        output_file = input_file.replace('.csv', '.jsonl').replace('.txt', '.jsonl')
        if output_file == input_file:
            output_file = 'output.jsonl'
        
        print()
        generate_jsonl_from_csv(input_file, output_file)
    else:
        print("✓ 跳过文件生成，仅测试 API")
        print()
        print("使用方法：")
        print("  python generate_embeddings.py")
        print("  然后输入 CSV 文件路径")
        print()
