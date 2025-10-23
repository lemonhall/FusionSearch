import json

# 失败的索引（根据输出）
failed_indices = [1, 5, 7, 18, 19, 22, 38, 39]

# 读取文档
with open('data/recipes.jsonl', 'r', encoding='utf-8') as f:
    docs = [json.loads(line) for line in f]

print("失败的菜谱长度分析：")
print("=" * 60)

for idx in failed_indices:
    doc = docs[idx - 1]  # 索引从0开始
    title = doc['title']
    length = len(doc['content'])
    print(f"{idx:2d}. {title:35s} {length:5d} 字符")

print("=" * 60)
print(f"\n最短失败: {min(len(docs[i-1]['content']) for i in failed_indices)} 字符")
print(f"最长失败: {max(len(docs[i-1]['content']) for i in failed_indices)} 字符")
print(f"\n建议截断长度: 1500 字符")
