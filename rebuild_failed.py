#!/usr/bin/env python3
"""
重新生成失败的向量
只处理缺失 embedding 的文档
"""

import os
import json
import requests
import time
from typing import List

# API 配置
API_URL = "https://api.siliconflow.cn/v1/embeddings"
API_KEY = os.getenv("SILICONFLOW_API_KEY", "")
EMBEDDING_MODEL = "BAAI/bge-large-zh-v1.5"


def generate_embedding(text: str) -> List[float]:
    """生成文本向量"""
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


def rebuild_failed():
    """重新生成失败的向量"""
    
    input_file = 'data/recipes.jsonl'
    output_file = 'data/recipes_vector.jsonl'
    
    print("=" * 60)
    print("重新生成失败的向量")
    print("=" * 60)
    print()
    
    # 检查 API 密钥
    if not API_KEY:
        print("❌ 错误：未设置 API 密钥")
        return False
    
    print(f"✓ API密钥: {API_KEY[:10]}...")
    print()
    
    # 读取原始文档
    with open(input_file, 'r', encoding='utf-8') as f:
        all_docs = [json.loads(line) for line in f]
    
    # 读取已有的向量文档
    existing_docs = {}
    if os.path.exists(output_file):
        with open(output_file, 'r', encoding='utf-8') as f:
            for line in f:
                doc = json.loads(line)
                if 'embedding' in doc and doc['embedding']:
                    existing_docs[doc['title']] = doc
    
    print(f"📖 总文档数: {len(all_docs)}")
    print(f"✓ 已有向量: {len(existing_docs)}")
    print(f"⚠ 需要生成: {len(all_docs) - len(existing_docs)}")
    print()
    
    # 生成缺失的向量
    success_count = 0
    error_count = 0
    
    all_results = []
    
    for i, doc in enumerate(all_docs, 1):
        title = doc['title']
        content = doc['content']
        
        # 如果已有向量，直接使用
        if title in existing_docs:
            all_results.append(existing_docs[title])
            print(f"[{i}/{len(all_docs)}] {title:40s} [已存在]")
            continue
        
        # 生成新向量
        print(f"[{i}/{len(all_docs)}] {title:40s} ", end='', flush=True)
        
        try:
            # 截断文本（保守策略）
            text_for_embedding = content
            if len(content) > 1500:
                text_for_embedding = content[:1500]
                print(f"[截断] ", end='', flush=True)
            
            # 再次检查字节数
            text_bytes = text_for_embedding.encode('utf-8')
            if len(text_bytes) > 4000:
                text_for_embedding = text_bytes[:4000].decode('utf-8', errors='ignore')
                print(f"[字节] ", end='', flush=True)
            
            start_time = time.time()
            embedding = generate_embedding(text_for_embedding)
            elapsed = time.time() - start_time
            
            # 添加到结果
            result_doc = {
                'title': title,
                'content': content,
                'embedding': embedding
            }
            all_results.append(result_doc)
            
            success_count += 1
            print(f"✓ {elapsed:.1f}s")
            
            time.sleep(0.1)
            
        except Exception as e:
            error_count += 1
            print(f"❌ {e}")
            # 失败的也要占位，保持顺序
            all_results.append({
                'title': title,
                'content': content,
                'embedding': []  # 空向量标记失败
            })
    
    # 写入完整结果
    print()
    print("💾 写入完整结果...")
    with open(output_file, 'w', encoding='utf-8') as f:
        for doc in all_results:
            f.write(json.dumps(doc, ensure_ascii=False) + '\n')
    
    print()
    print("=" * 60)
    print(f"✅ 完成！")
    print(f"   成功: {success_count}")
    print(f"   失败: {error_count}")
    print(f"   总计: {len(all_results)} / {len(all_docs)}")
    print("=" * 60)
    
    return error_count == 0


if __name__ == "__main__":
    success = rebuild_failed()
    exit(0 if success else 1)
