#!/usr/bin/env python3
"""
混合检索交互式测试工具
功能：
1. 加载 JSONL 向量索引
2. 用户输入查询
3. 调用 API 生成查询向量
4. 计算余弦相似度
5. 返回 Top-K 结果

使用：
    export SILICONFLOW_API_KEY='your-key'
    python query_test.py
"""

import os
import json
import requests
import numpy as np
from typing import List, Dict

# API 配置
API_URL = "https://api.siliconflow.cn/v1/embeddings"
API_KEY = os.getenv("SILICONFLOW_API_KEY", "")
EMBEDDING_MODEL = "BAAI/bge-large-zh-v1.5"


def generate_embedding(text: str) -> List[float]:
    """生成查询向量"""
    headers = {
        'Authorization': f'Bearer {API_KEY}',
        'Content-Type': 'application/json'
    }
    
    payload = {
        'model': EMBEDDING_MODEL,
        'input': text
    }
    
    response = requests.post(API_URL, headers=headers, json=payload, timeout=30)
    response.raise_for_status()
    
    return response.json()['data'][0]['embedding']


def cosine_similarity(vec1: List[float], vec2: List[float]) -> float:
    """计算余弦相似度"""
    vec1 = np.array(vec1)
    vec2 = np.array(vec2)
    
    dot_product = np.dot(vec1, vec2)
    norm1 = np.linalg.norm(vec1)
    norm2 = np.linalg.norm(vec2)
    
    if norm1 == 0 or norm2 == 0:
        return 0.0
    
    return dot_product / (norm1 * norm2)


def load_vector_index(jsonl_file='data/recipes_vector.jsonl') -> List[Dict]:
    """加载向量索引"""
    documents = []
    
    with open(jsonl_file, 'r', encoding='utf-8') as f:
        for line in f:
            doc = json.loads(line)
            documents.append(doc)
    
    return documents


def vector_search(query: str, documents: List[Dict], top_k=5) -> List[Dict]:
    """向量检索"""
    
    # 生成查询向量
    print(f"🔍 生成查询向量...")
    query_embedding = generate_embedding(query)
    print(f"✓ 向量维度: {len(query_embedding)}")
    print()
    
    # 计算相似度
    results = []
    for i, doc in enumerate(documents):
        similarity = cosine_similarity(query_embedding, doc['embedding'])
        results.append({
            'doc_id': i + 1,
            'title': doc['title'],
            'content': doc['content'],
            'similarity': similarity
        })
    
    # 排序并返回 Top-K
    results.sort(key=lambda x: x['similarity'], reverse=True)
    return results[:top_k]


def main():
    print("=" * 70)
    print("FusionSearch 混合检索测试工具")
    print("=" * 70)
    print()
    
    # 检查 API 密钥
    if not API_KEY:
        print("❌ 错误：未设置 API 密钥")
        print()
        print("请设置环境变量：")
        print("  export SILICONFLOW_API_KEY='your-api-key'")
        print()
        return
    
    # 加载向量索引
    jsonl_file = 'data/recipes_vector.jsonl'
    
    if not os.path.exists(jsonl_file):
        print(f"❌ 向量索引文件不存在: {jsonl_file}")
        print()
        print("请先运行：")
        print("  python prepare_recipes.py")
        print("  python build_vector_index.py")
        print()
        return
    
    print(f"📖 加载向量索引: {jsonl_file}")
    documents = load_vector_index(jsonl_file)
    print(f"✓ 加载 {len(documents)} 个文档")
    print()
    
    # 交互式查询
    print("=" * 70)
    print("开始查询（输入 'quit' 退出）")
    print("=" * 70)
    print()
    
    while True:
        try:
            query = input("🔍 请输入查询: ").strip()
            
            if not query:
                continue
            
            if query.lower() in ['quit', 'exit', 'q']:
                print("👋 再见！")
                break
            
            print()
            
            # 向量检索
            results = vector_search(query, documents, top_k=5)
            
            # 显示结果
            print("🎯 检索结果（Top-5）：")
            print("─" * 70)
            
            for i, result in enumerate(results, 1):
                print(f"\n#{i}  相似度: {result['similarity']:.4f}")
                print(f"    标题: {result['title']}")
                
                # 截取内容预览
                content_preview = result['content'][:100].replace('\n', ' ')
                print(f"    内容: {content_preview}...")
            
            print()
            print("─" * 70)
            print()
            
        except KeyboardInterrupt:
            print("\n\n👋 再见！")
            break
        except Exception as e:
            print(f"\n❌ 错误: {e}\n")


if __name__ == "__main__":
    try:
        import numpy as np
    except ImportError:
        print("❌ 缺少依赖: numpy")
        print()
        print("请安装：")
        print("  pip install numpy")
        print()
        exit(1)
    
    main()
