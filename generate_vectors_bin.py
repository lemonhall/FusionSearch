#!/usr/bin/env python3
"""
直接从JSONL生成vectors.bin
绕过C的JSON解析问题
"""

import json
import struct

print("=" * 60)
print("向量索引二进制文件生成工具")
print("=" * 60)
print()

input_file = "data/recipes_vector.jsonl"
output_file = "vectors.bin"

print(f"读取: {input_file}")

# 读取所有向量
vectors = []
doc_ids = []

with open(input_file, 'r', encoding='utf-8') as f:
    for i, line in enumerate(f):
        if not line.strip():
            continue
        
        try:
            doc = json.loads(line)
            if 'embedding' in doc:
                vectors.append(doc['embedding'])
                doc_ids.append(i)
                print(f"  [{i+1}] {doc.get('title', 'Unknown')[:40]} - {len(doc['embedding'])}维")
        except Exception as e:
            print(f"  ⚠ 行 {i+1} 解析失败: {e}")

print()
print(f"✓ 读取 {len(vectors)} 个向量")

if not vectors:
    print("❌ 未找到任何向量数据")
    exit(1)

# 验证维度一致性
dimensions = [len(v) for v in vectors]
if len(set(dimensions)) > 1:
    print(f"❌ 向量维度不一致: {set(dimensions)}")
    exit(1)

dimension = dimensions[0]
print(f"✓ 向量维度: {dimension}")
print()

# 写入二进制文件
print(f"生成: {output_file}")

with open(output_file, 'wb') as f:
    count = len(vectors)
    
    # 写头部：count + dimension
    f.write(struct.pack('I', count))
    f.write(struct.pack('I', dimension))
    
    # 写每个向量
    for doc_id, vec in zip(doc_ids, vectors):
        # 写doc_id
        f.write(struct.pack('I', doc_id))
        # 写向量数据
        f.write(struct.pack(f'{dimension}f', *vec))

print()
print("=" * 60)
print(f"✅ 生成完成！")
print(f"   文件: {output_file}")
print(f"   文档数: {count}")
print(f"   维度: {dimension}")
print(f"   大小: {8 + count * (4 + dimension * 4)} 字节")
print("=" * 60)
