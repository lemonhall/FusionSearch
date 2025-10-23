#!/usr/bin/env python3
"""
搜索 API - 独立运行的混合搜索引擎
功能：
1. 调用外部API获取query向量化（Python）
2. 读取C生成的向量索引文件
3. 暴力余弦相似度检索（Python实现）
4. 调用C的BM25搜索（通过命令行或直接解析索引）
5. 融合排序返回结果

设计理念：
- C负责：文档加载、分词、BM25索引构建、向量文件生成
- Python负责：Query向量化、向量检索、混合排序
- 数据交换：通过文件（vectors.bin）或 JSON

依赖：
    pip install requests numpy
"""

import os
import json
import struct
import requests
import time
import numpy as np
from typing import List, Dict, Optional, Tuple
from pathlib import Path

# ============================================================================
# 向量化 API 调用
# ============================================================================

class EmbeddingAPI:
    """向量化 API 封装"""
    
    def __init__(self, api_key: Optional[str] = None):
        self.api_key = api_key or os.getenv("SILICONFLOW_API_KEY", "")
        
        if not self.api_key:
            raise ValueError(
                "API密钥未设置！\n"
                "请设置环境变量: export SILICONFLOW_API_KEY='your-key'"
            )
        
        self.api_url = "https://api.siliconflow.cn/v1/embeddings"
        self.model = "BAAI/bge-large-zh-v1.5"  # 1024维
    
    def get_embedding(self, text: str) -> np.ndarray:
        """
        调用 API 获取文本向量
        
        Args:
            text: 输入文本
            
        Returns:
            向量数组（numpy）
        """
        headers = {
            'Authorization': f'Bearer {self.api_key}',
            'Content-Type': 'application/json'
        }
        
        payload = {
            'model': self.model,
            'input': text
        }
        
        try:
            response = requests.post(
                self.api_url, 
                headers=headers, 
                json=payload, 
                timeout=30
            )
            response.raise_for_status()
            
            data = response.json()
            embedding = data['data'][0]['embedding']
            
            return np.array(embedding, dtype=np.float32)
        
        except requests.exceptions.HTTPError as e:
            raise RuntimeError(f"API调用失败: {e}\n响应: {e.response.text}")
        except requests.exceptions.Timeout:
            raise RuntimeError("API请求超时")
        except Exception as e:
            raise RuntimeError(f"未知错误: {e}")

# ============================================================================
# 向量索引加载（读取 C 生成的二进制文件）
# ============================================================================

class VectorIndex:
    """向量索引（纯 Python 实现）"""
    
    def __init__(self, vectors_file: str = "vectors.bin"):
        """
        加载向量索引
        
        文件格式（C生成）：
        - [4 bytes] count (uint32)
        - [4 bytes] dimension (uint32)
        - 对每个向量：
          - [4 bytes] doc_id (uint32)
          - [dimension * 4 bytes] embedding (float[])
        """
        self.vectors = []  # List[(doc_id, embedding)]
        self.dimension = 0
        
        if not os.path.exists(vectors_file):
            print(f"⚠ 向量文件未找到: {vectors_file}")
            print(f"   将无法使用向量检索功能")
            return
        
        self._load_from_file(vectors_file)
        print(f"✓ 加载向量索引: {len(self.vectors)} 个文档, {self.dimension} 维")
    
    def _load_from_file(self, file_path: str):
        """从二进制文件加载向量"""
        with open(file_path, 'rb') as f:
            # 读取头部
            count_bytes = f.read(4)
            dim_bytes = f.read(4)
            
            count = struct.unpack('I', count_bytes)[0]
            self.dimension = struct.unpack('I', dim_bytes)[0]
            
            # 读取所有向量
            for _ in range(count):
                doc_id_bytes = f.read(4)
                doc_id = struct.unpack('I', doc_id_bytes)[0]
                
                # 读取向量
                embedding_bytes = f.read(self.dimension * 4)
                embedding = struct.unpack(f'{self.dimension}f', embedding_bytes)
                embedding_array = np.array(embedding, dtype=np.float32)
                
                self.vectors.append((doc_id, embedding_array))
    
    def search(self, query_embedding: np.ndarray, top_k: int = 10) -> List[Tuple[int, float]]:
        """
        暴力向量检索
        
        Args:
            query_embedding: 查询向量
            top_k: 返回结果数量
            
        Returns:
            [(doc_id, similarity), ...]（按相似度降序）
        """
        if len(self.vectors) == 0:
            return []
        
        # 计算所有文档的余弦相似度
        similarities = []
        for doc_id, doc_embedding in self.vectors:
            sim = self._cosine_similarity(query_embedding, doc_embedding)
            similarities.append((doc_id, sim))
        
        # 排序并返回 Top-K
        similarities.sort(key=lambda x: x[1], reverse=True)
        return similarities[:top_k]
    
    @staticmethod
    def _cosine_similarity(vec1: np.ndarray, vec2: np.ndarray) -> float:
        """计算余弦相似度"""
        dot_product = np.dot(vec1, vec2)
        norm1 = np.linalg.norm(vec1)
        norm2 = np.linalg.norm(vec2)
        
        if norm1 == 0 or norm2 == 0:
            return 0.0
        
        similarity = dot_product / (norm1 * norm2)
        
        # 归一化到 [0, 1]
        return max(0.0, min(1.0, similarity))

# ============================================================================
# BM25 搜索（调用 C 程序）
# ============================================================================

class BM25Search:
    """BM25 搜索（调用 C 程序）"""
    
    def __init__(self, c_executable: str = "./search_engine"):
        """
        初始化 BM25 搜索
        
        Args:
            c_executable: C 程序路径
        """
        self.c_executable = c_executable
        
        if not os.path.exists(c_executable):
            raise FileNotFoundError(
                f"C程序未找到: {c_executable}\n"
                f"请先编译：make"
            )
    
    def search(self, query: str, top_k: int = 10) -> List[Dict]:
        """
        调用 C 程序执行 BM25 搜索
        
        返回格式示例：
        使用临时文件交换数据（或解析 C 程序的 stdout）
        
        TODO: 需要修改 C 程序支持命令行查询或 JSON 输出
        """
        # 临时方案：返回空列表（需要集成 C 程序）
        print("⚠ BM25搜索暂未实现，需要修改C程序支持JSON输出")
        return []

# ============================================================================
# 混合搜索引擎
# ============================================================================

class HybridSearchEngine:
    """混合搜索引擎（BM25 + 向量检索）"""
    
    def __init__(
        self, 
        embedding_api: Optional[EmbeddingAPI] = None,
        vector_index: Optional[VectorIndex] = None,
        bm25_search: Optional[BM25Search] = None
    ):
        """
        初始化混合搜索引擎
        
        Args:
            embedding_api: API 客户端
            vector_index: 向量索引
            bm25_search: BM25 搜索引擎
        """
        self.embedding_api = embedding_api or EmbeddingAPI()
        self.vector_index = vector_index or VectorIndex()
        self.bm25_search = bm25_search
        
        # 加载文档元数据（从 C 生成的 JSON 文件）
        self.documents = self._load_documents()
    
    def _load_documents(self) -> Dict[int, Dict]:
        """
        加载文档元数据
        
        期望格式（C 生成的 documents.json）：
        [
            {"id": 1, "title": "...", "content": "..."},
            {"id": 2, "title": "...", "content": "..."}
        ]
        """
        doc_file = "documents.json"
        
        if not os.path.exists(doc_file):
            print(f"⚠ 文档元数据未找到: {doc_file}")
            return {}
        
        with open(doc_file, 'r', encoding='utf-8') as f:
            docs_list = json.load(f)
        
        # 转换为 doc_id -> document 的映射
        docs_map = {doc['id']: doc for doc in docs_list}
        print(f"✓ 加载文档元数据: {len(docs_map)} 个文档")
        
        return docs_map
    
    def search(
        self, 
        query: str, 
        top_k: int = 10,
        use_bm25: bool = True,
        use_vector: bool = True,
        bm25_weight: float = 0.5,
        vector_weight: float = 0.5
    ) -> List[Dict]:
        """
        执行混合搜索
        
        Args:
            query: 搜索查询
            top_k: 返回结果数量
            use_bm25: 是否使用 BM25
            use_vector: 是否使用向量检索
            bm25_weight: BM25权重（0-1）
            vector_weight: 向量权重（0-1）
            
        Returns:
            排序后的搜索结果列表
        """
        print()
        print("=" * 70)
        print(f"🔍 混合搜索: {query}")
        print("=" * 70)
        print()
        
        bm25_results = []
        vector_results = []
        
        # ========================================
        # 1. 向量检索
        # ========================================
        if use_vector and len(self.vector_index.vectors) > 0:
            print("📡 调用 API 获取 query 向量...")
            start_time = time.time()
            
            try:
                query_vector = self.embedding_api.get_embedding(query)
                api_time = time.time() - start_time
                print(f"   ✓ 完成（{len(query_vector)}维，耗时 {api_time:.3f}s）")
                print()
                
                print("🎯 执行向量召回...")
                vector_results = self.vector_index.search(query_vector, top_k * 2)
                print(f"   ✓ 返回 {len(vector_results)} 个结果")
                print()
                
            except Exception as e:
                print(f"   ❌ 向量检索失败: {e}")
                print()
        
        # ========================================
        # 2. BM25 搜索（暂未实现）
        # ========================================
        if use_bm25 and self.bm25_search:
            print("📚 执行 BM25 搜索...")
            bm25_results = self.bm25_search.search(query, top_k * 2)
            print(f"   ✓ 返回 {len(bm25_results)} 个结果")
            print()
        
        # ========================================
        # 3. 融合排序
        # ========================================
        print("🔀 融合排序...")
        final_results = self._merge_results(
            bm25_results, 
            vector_results,
            bm25_weight,
            vector_weight,
            top_k
        )
        print(f"   ✓ 最终返回 {len(final_results)} 个结果")
        print()
        
        return final_results
    
    def _merge_results(
        self, 
        bm25_results: List[Dict],
        vector_results: List[Tuple[int, float]],
        bm25_weight: float,
        vector_weight: float,
        top_k: int
    ) -> List[Dict]:
        """
        融合 BM25 和向量检索结果
        
        融合策略：加权求和
        final_score = bm25_weight * norm(bm25_score) + vector_weight * vector_score
        """
        # 构建 doc_id -> 结果的映射
        doc_map = {}
        
        # 处理 BM25 结果
        max_bm25 = max((r.get('score', 0) for r in bm25_results), default=1.0)
        
        for r in bm25_results:
            doc_id = r['doc_id']
            doc_map[doc_id] = {
                'doc_id': doc_id,
                'title': r.get('title', ''),
                'snippet': r.get('snippet', ''),
                'bm25_score': r.get('score', 0),
                'bm25_norm': r.get('score', 0) / max_bm25 if max_bm25 > 0 else 0,
                'vector_score': 0.0
            }
        
        # 添加向量分数
        for doc_id, similarity in vector_results:
            if doc_id in doc_map:
                doc_map[doc_id]['vector_score'] = similarity
            else:
                # 获取文档元数据
                doc = self.documents.get(doc_id, {})
                
                doc_map[doc_id] = {
                    'doc_id': doc_id,
                    'title': doc.get('title', f'Document {doc_id}'),
                    'snippet': doc.get('content', '')[:150] + '...',
                    'bm25_score': 0.0,
                    'bm25_norm': 0.0,
                    'vector_score': similarity
                }
        
        # 计算最终分数
        for doc in doc_map.values():
            doc['final_score'] = (
                bm25_weight * doc['bm25_norm'] + 
                vector_weight * doc['vector_score']
            )
        
        # 排序并返回 Top-K
        sorted_results = sorted(
            doc_map.values(), 
            key=lambda x: x['final_score'], 
            reverse=True
        )
        
        return sorted_results[:top_k]

# ============================================================================
# 命令行接口
# ============================================================================

def main():
    """命令行入口"""
    print()
    print("=" * 70)
    print("混合搜索引擎 - 独立版本")
    print("=" * 70)
    print()
    
    # 初始化引擎
    try:
        engine = HybridSearchEngine()
    except Exception as e:
        print(f"❌ 初始化失败: {e}")
        return
    
    print()
    print("=" * 70)
    print("✅ 搜索引擎初始化完成")
    print("=" * 70)
    print()
    
    # 交互式搜索
    while True:
        try:
            query = input("\n🔍 请输入搜索内容（输入 'quit' 退出）: ").strip()
            
            if query.lower() in ['quit', 'exit', 'q']:
                print("\n👋 再见！")
                break
            
            if not query:
                continue
            
            # 执行搜索
            results = engine.search(query, top_k=5, use_bm25=False, use_vector=True)
            
            # 显示结果
            print()
            print("=" * 70)
            print("📄 搜索结果")
            print("=" * 70)
            print()
            
            if not results:
                print("   未找到相关结果")
            else:
                for i, result in enumerate(results, 1):
                    print(f"{i}. {result['title']}")
                    print(f"   Doc ID: {result['doc_id']}")
                    print(f"   Final Score: {result['final_score']:.4f} "
                          f"(BM25: {result['bm25_norm']:.4f}, Vector: {result['vector_score']:.4f})")
                    print(f"   {result['snippet'][:100]}...")
                    print()
            
            print("=" * 70)
        
        except KeyboardInterrupt:
            print("\n\n👋 再见！")
            break
        except Exception as e:
            print(f"\n❌ 搜索出错: {e}")

if __name__ == "__main__":
    main()
