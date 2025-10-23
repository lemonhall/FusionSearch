#!/usr/bin/env python3
"""
混合搜索引擎 - Python FFI 封装
功能：
1. 调用外部API获取query向量化（Python）
2. 调用C语言API执行BM25搜索（通过ctypes）
3. 调用C语言API执行向量召回（通过ctypes）
4. 融合排序并返回结果

依赖：
    - requests (用于API调用)
    - ctypes (Python内置，用于FFI)
"""

import os
import json
import ctypes
import requests
import time
from typing import List, Dict, Optional, Tuple
from pathlib import Path

# ============================================================================
# C 结构体定义（映射到 C 代码）
# ============================================================================

class VectorResult(ctypes.Structure):
    """对应 C 中的 VectorResult"""
    _fields_ = [
        ("doc_id", ctypes.c_uint32),
        ("similarity", ctypes.c_float)
    ]

class SearchResult(ctypes.Structure):
    """对应 C 中的 SearchResult"""
    _fields_ = [
        ("docId", ctypes.c_uint32),
        ("score", ctypes.c_float),
        ("title", ctypes.c_char_p),
        ("snippet", ctypes.c_char_p)
    ]

class SearchResultSet(ctypes.Structure):
    """对应 C 中的 SearchResultSet"""
    _fields_ = [
        ("results", ctypes.POINTER(SearchResult)),
        ("count", ctypes.c_size_t),
        ("executionTime", ctypes.c_float)
    ]

# ============================================================================
# C 库加载
# ============================================================================

class CSearchEngine:
    """C 搜索引擎 FFI 封装"""
    
    def __init__(self, lib_path: str = "./libfusion.so"):
        """
        初始化 C 库
        
        Args:
            lib_path: 共享库路径（Linux: .so, macOS: .dylib, Windows: .dll）
        """
        # 根据操作系统选择库文件
        if os.name == 'nt':  # Windows
            lib_path = "./fusion.dll"
        elif os.uname().sysname == 'Darwin':  # macOS
            lib_path = "./libfusion.dylib"
        else:  # Linux
            lib_path = "./libfusion.so"
        
        if not os.path.exists(lib_path):
            raise FileNotFoundError(
                f"C库文件未找到: {lib_path}\n"
                f"请先编译：make lib"
            )
        
        self.lib = ctypes.CDLL(lib_path)
        self._setup_function_signatures()
    
    def _setup_function_signatures(self):
        """设置 C 函数签名（参数和返回值类型）"""
        
        # vector_search
        self.lib.vector_search.argtypes = [
            ctypes.c_void_p,                # VectorIndex*
            ctypes.POINTER(ctypes.c_float), # query_embedding
            ctypes.c_uint32,                # k
            ctypes.POINTER(ctypes.c_size_t) # result_count
        ]
        self.lib.vector_search.restype = ctypes.POINTER(VectorResult)
        
        # search_engine_search (BM25)
        self.lib.search_engine_search.argtypes = [
            ctypes.c_void_p,    # SearchEngine*
            ctypes.c_char_p,    # query
            ctypes.c_int,       # SearchMode
            ctypes.c_size_t     # maxResults
        ]
        self.lib.search_engine_search.restype = ctypes.POINTER(SearchResultSet)
        
        # search_free_results
        self.lib.search_free_results.argtypes = [ctypes.POINTER(SearchResultSet)]
        self.lib.search_free_results.restype = None

# ============================================================================
# API 调用（获取 query 向量）
# ============================================================================

class EmbeddingAPI:
    """向量化 API 封装"""
    
    def __init__(self, api_key: Optional[str] = None):
        """
        初始化 API 客户端
        
        Args:
            api_key: API密钥（如果为空则从环境变量读取）
        """
        self.api_key = api_key or os.getenv("SILICONFLOW_API_KEY", "")
        
        if not self.api_key:
            raise ValueError(
                "API密钥未设置！\n"
                "请设置环境变量: export SILICONFLOW_API_KEY='your-key'"
            )
        
        self.api_url = "https://api.siliconflow.cn/v1/embeddings"
        self.model = "BAAI/bge-large-zh-v1.5"  # 1024维
    
    def get_embedding(self, text: str) -> List[float]:
        """
        调用 API 获取文本向量
        
        Args:
            text: 输入文本
            
        Returns:
            向量列表（浮点数）
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
            
            return embedding
        
        except requests.exceptions.HTTPError as e:
            raise RuntimeError(f"API调用失败: {e}\n响应: {e.response.text}")
        except requests.exceptions.Timeout:
            raise RuntimeError("API请求超时")
        except Exception as e:
            raise RuntimeError(f"未知错误: {e}")

# ============================================================================
# 混合搜索引擎
# ============================================================================

class HybridSearchEngine:
    """混合搜索引擎（BM25 + 向量检索）"""
    
    def __init__(
        self, 
        c_engine_ptr: int,
        vector_index_ptr: int,
        c_lib: CSearchEngine,
        embedding_api: EmbeddingAPI
    ):
        """
        初始化混合搜索引擎
        
        Args:
            c_engine_ptr: C SearchEngine 指针地址
            vector_index_ptr: C VectorIndex 指针地址
            c_lib: C 库封装对象
            embedding_api: API 客户端
        """
        self.c_engine_ptr = c_engine_ptr
        self.vector_index_ptr = vector_index_ptr
        self.c_lib = c_lib
        self.embedding_api = embedding_api
    
    def search(
        self, 
        query: str, 
        top_k: int = 10,
        bm25_weight: float = 0.5,
        vector_weight: float = 0.5
    ) -> List[Dict]:
        """
        执行混合搜索
        
        Args:
            query: 搜索查询
            top_k: 返回结果数量
            bm25_weight: BM25权重（0-1）
            vector_weight: 向量权重（0-1）
            
        Returns:
            排序后的搜索结果列表
        """
        print(f"🔍 混合搜索: {query}")
        print()
        
        # ========================================
        # 1. 调用 API 获取 query 向量
        # ========================================
        print("📡 调用 API 获取 query 向量...")
        start_time = time.time()
        
        try:
            query_vector = self.embedding_api.get_embedding(query)
            api_time = time.time() - start_time
            print(f"   ✓ 完成（{len(query_vector)}维，耗时 {api_time:.3f}s）")
        except Exception as e:
            print(f"   ❌ API调用失败: {e}")
            # 降级到纯 BM25 搜索
            print("   ⚠ 降级到纯 BM25 搜索")
            return self._bm25_only_search(query, top_k)
        
        print()
        
        # ========================================
        # 2. 调用 C API 执行 BM25 搜索
        # ========================================
        print("📚 执行 BM25 搜索...")
        bm25_results = self._call_bm25_search(query, top_k)
        print(f"   ✓ 返回 {len(bm25_results)} 个结果")
        print()
        
        # ========================================
        # 3. 调用 C API 执行向量召回
        # ========================================
        print("🎯 执行向量召回...")
        vector_results = self._call_vector_search(query_vector, top_k)
        print(f"   ✓ 返回 {len(vector_results)} 个结果")
        print()
        
        # ========================================
        # 4. 融合排序
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
    
    def _call_bm25_search(self, query: str, top_k: int) -> List[Dict]:
        """调用 C API 执行 BM25 搜索"""
        
        # SearchMode: SEARCH_BM25 = 2
        SEARCH_BM25 = 2
        
        result_set_ptr = self.c_lib.lib.search_engine_search(
            self.c_engine_ptr,
            query.encode('utf-8'),
            SEARCH_BM25,
            top_k
        )
        
        if not result_set_ptr:
            return []
        
        result_set = result_set_ptr.contents
        results = []
        
        for i in range(result_set.count):
            result = result_set.results[i]
            results.append({
                'doc_id': result.docId,
                'bm25_score': result.score,
                'title': result.title.decode('utf-8') if result.title else '',
                'snippet': result.snippet.decode('utf-8') if result.snippet else ''
            })
        
        # 释放 C 内存
        self.c_lib.lib.search_free_results(result_set_ptr)
        
        return results
    
    def _call_vector_search(self, query_vector: List[float], top_k: int) -> List[Dict]:
        """调用 C API 执行向量召回"""
        
        # 转换为 C float 数组
        embedding_array = (ctypes.c_float * len(query_vector))(*query_vector)
        result_count = ctypes.c_size_t()
        
        results_ptr = self.c_lib.lib.vector_search(
            self.vector_index_ptr,
            embedding_array,
            top_k,
            ctypes.byref(result_count)
        )
        
        if not results_ptr:
            return []
        
        results = []
        for i in range(result_count.value):
            result = results_ptr[i]
            results.append({
                'doc_id': result.doc_id,
                'vector_score': result.similarity
            })
        
        # 释放 C 内存（需要在 C 端添加释放函数）
        # TODO: 添加 vector_free_results(results_ptr)
        
        return results
    
    def _merge_results(
        self, 
        bm25_results: List[Dict],
        vector_results: List[Dict],
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
        
        # 归一化 BM25 分数（最大值归一化）
        max_bm25 = max((r['bm25_score'] for r in bm25_results), default=1.0)
        
        for r in bm25_results:
            doc_id = r['doc_id']
            doc_map[doc_id] = {
                'doc_id': doc_id,
                'title': r.get('title', ''),
                'snippet': r.get('snippet', ''),
                'bm25_score': r['bm25_score'],
                'bm25_norm': r['bm25_score'] / max_bm25 if max_bm25 > 0 else 0,
                'vector_score': 0.0
            }
        
        # 添加向量分数
        for r in vector_results:
            doc_id = r['doc_id']
            if doc_id in doc_map:
                doc_map[doc_id]['vector_score'] = r['vector_score']
            else:
                # 向量召回但 BM25 没有的文档（可能是语义相关但词汇不匹配）
                doc_map[doc_id] = {
                    'doc_id': doc_id,
                    'title': '',  # 需要从 C 获取
                    'snippet': '',
                    'bm25_score': 0.0,
                    'bm25_norm': 0.0,
                    'vector_score': r['vector_score']
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
    
    def _bm25_only_search(self, query: str, top_k: int) -> List[Dict]:
        """降级方案：纯 BM25 搜索"""
        results = self._call_bm25_search(query, top_k)
        for r in results:
            r['final_score'] = r['bm25_score']
            r['vector_score'] = 0.0
        return results

# ============================================================================
# 使用示例
# ============================================================================

if __name__ == "__main__":
    print("=" * 70)
    print("混合搜索引擎 - Python FFI 演示")
    print("=" * 70)
    print()
    
    # 注意：这只是示例，实际使用需要从 C 主程序传入指针
    print("⚠ 警告：此脚本仅作演示，实际使用需要从 C 程序传入引擎指针")
    print()
    print("完整工作流程：")
    print("1. C 程序加载文档，构建 BM25 索引 + 向量索引")
    print("2. C 程序调用 Python（通过子进程或嵌入式 Python）")
    print("3. Python 接收引擎指针，执行混合搜索")
    print("4. Python 返回结果给 C 程序展示")
    print()
    print("=" * 70)
