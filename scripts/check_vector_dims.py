#!/usr/bin/env python3
"""检查向量维度"""

import json
from collections import Counter

dims = []

with open('data/recipes_vector.jsonl', 'r', encoding='utf-8') as f:
    for i, line in enumerate(f, 1):
        if not line.strip() or not line.startswith('{'):
            continue
        
        try:
            doc = json.loads(line)
            if 'embedding' in doc:
                dim = len(doc['embedding'])
                dims.append(dim)
                print(f"Doc {i}: {dim}维 - {doc.get('title', 'No title')[:40]}")
        except Exception as e:
            print(f"Doc {i}: 解析失败 - {e}")

print("\n" + "="*60)
print("统计:")
print("="*60)

if dims:
    counter = Counter(dims)
    for dim, count in counter.most_common():
        print(f"  {dim}维: {count}个文档")
    
    print(f"\n  平均维度: {sum(dims)/len(dims):.1f}")
    print(f"  最小维度: {min(dims)}")
    print(f"  最大维度: {max(dims)}")
else:
    print("  未找到任何向量数据")
