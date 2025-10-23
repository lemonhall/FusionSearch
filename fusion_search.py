#!/usr/bin/env python3
"""
混合搜索引擎 - Python调度层
职责：
1. 调用外部API获取Query向量
2. 调用C的BM25搜索
3. 调用C的向量检索
4. 融合排序并返回结果

C语言负责：
- 文档加载和分词
- BM25索引构建和检索
- 向量索引加载和检索
- 文档内容查询

Python只负责：
- API调用（获取Query向量）
- 调度C的两个搜索接口
- 融合排序
"""

import os
import ctypes
import requests
from typing import List, Dict, Optional, Tuple

# ============================================================================
# API调用 - 获取Query向量
# ============================================================================

class EmbeddingAPI:
    """向量化API封装"""
    
    def __init__(self, api_key: Optional[str] = None):
        self.api_key = api_key or os.getenv("SILICONFLOW_API_KEY", "")
        
        if not self.api_key:
            raise ValueError(
                "API密钥未设置！\n"
                "请设置环境变量: export SILICONFLOW_API_KEY='your-key'"
            )
        
        self.api_url = "https://api.siliconflow.cn/v1/embeddings"
        self.model = "BAAI/bge-m3"  # 1024维，免费
    
    def get_embedding(self, text: str) -> List[float]:
        """调用API获取文本向量"""
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
        
        except Exception as e:
            raise RuntimeError(f"API调用失败: {e}")

# ============================================================================
# C库FFI封装
# ============================================================================

class CFusionSearch:
    """C搜索引擎FFI封装"""
    
    def __init__(self, lib_path: str = "./libfusion.so"):
        """初始化C库"""
        # 根据操作系统选择库文件
        if not lib_path or lib_path == "./libfusion.so":
            if os.name == 'nt':  # Windows
                lib_path = "./fusion.dll"
            else:  # Linux/WSL
                lib_path = "./libfusion.so"
        
        if not os.path.exists(lib_path):
            raise FileNotFoundError(
                f"C库文件未找到: {lib_path}\n"
                f"请先编译：make lib"
            )
        
        self.lib = ctypes.CDLL(lib_path)
        self._setup_signatures()
        self.index_ptr = 0
        self.vector_index_ptr = 0
    
    def _setup_signatures(self):
        """设置C函数签名"""
        
        # ffi_index_load
        self.lib.ffi_index_load.argtypes = [ctypes.c_char_p]
        self.lib.ffi_index_load.restype = ctypes.c_size_t
        
        # ffi_vector_index_load
        self.lib.ffi_vector_index_load.argtypes = [ctypes.c_char_p]
        self.lib.ffi_vector_index_load.restype = ctypes.c_size_t
        
        # ffi_bm25_search
        self.lib.ffi_bm25_search.argtypes = [
            ctypes.c_size_t,                  # index_ptr
            ctypes.c_char_p,                  # query
            ctypes.c_uint32,                  # k
            ctypes.POINTER(ctypes.c_uint32),  # out_doc_ids
            ctypes.POINTER(ctypes.c_float)    # out_scores
        ]
        self.lib.ffi_bm25_search.restype = ctypes.c_size_t
        
        # ffi_vector_search
        self.lib.ffi_vector_search.argtypes = [
            ctypes.c_size_t,                  # index_ptr
            ctypes.POINTER(ctypes.c_float),   # query_embedding
            ctypes.c_uint32,                  # dimension
            ctypes.c_uint32,                  # k
            ctypes.POINTER(ctypes.c_uint32),  # out_doc_ids
            ctypes.POINTER(ctypes.c_float)    # out_scores
        ]
        self.lib.ffi_vector_search.restype = ctypes.c_size_t
        
        # ffi_get_document
        self.lib.ffi_get_document.argtypes = [
            ctypes.c_size_t,     # index_ptr
            ctypes.c_uint32,     # doc_id
            ctypes.c_char_p,     # out_title
            ctypes.c_size_t,     # title_size
            ctypes.c_char_p,     # out_content
            ctypes.c_size_t      # content_size
        ]
        self.lib.ffi_get_document.restype = ctypes.c_int
        
        # ffi_index_free
        self.lib.ffi_index_free.argtypes = [ctypes.c_size_t]
        self.lib.ffi_index_free.restype = None
        
        # ffi_vector_index_free
        self.lib.ffi_vector_index_free.argtypes = [ctypes.c_size_t]
        self.lib.ffi_vector_index_free.restype = None
    
    def load_index(self, jsonl_file: str):
        """加载BM25索引"""
        self.index_ptr = self.lib.ffi_index_load(jsonl_file.encode('utf-8'))
        if self.index_ptr == 0:
            raise RuntimeError("Failed to load BM25 index")
    
    def load_vector_index(self, vector_file: str):
        """加载向量索引"""
        self.vector_index_ptr = self.lib.ffi_vector_index_load(vector_file.encode('utf-8'))
        if self.vector_index_ptr == 0:
            raise RuntimeError("Failed to load vector index")
    
    def bm25_search(self, query: str, k: int = 10) -> List[Tuple[int, float]]:
        """调用C的BM25搜索"""
        doc_ids = (ctypes.c_uint32 * k)()
        scores = (ctypes.c_float * k)()
        
        count = self.lib.ffi_bm25_search(
            self.index_ptr,
            query.encode('utf-8'),
            k,
            doc_ids,
            scores
        )
        
        return [(doc_ids[i], scores[i]) for i in range(count)]
    
    def vector_search(self, query_embedding: List[float], k: int = 10) -> List[Tuple[int, float]]:
        """调用C的向量检索"""
        dimension = len(query_embedding)
        embedding_array = (ctypes.c_float * dimension)(*query_embedding)
        doc_ids = (ctypes.c_uint32 * k)()
        scores = (ctypes.c_float * k)()
        
        count = self.lib.ffi_vector_search(
            self.vector_index_ptr,
            embedding_array,
            dimension,
            k,
            doc_ids,
            scores
        )
        
        return [(doc_ids[i], scores[i]) for i in range(count)]
    
    def get_document(self, doc_id: int) -> Dict[str, str]:
        """查询文档内容"""
        title_buffer = ctypes.create_string_buffer(256)
        content_buffer = ctypes.create_string_buffer(2048)
        
        result = self.lib.ffi_get_document(
            self.index_ptr,
            doc_id,
            title_buffer,
            256,
            content_buffer,
            2048
        )
        
        if result != 0:
            return {"title": "", "content": ""}
        
        return {
            "title": title_buffer.value.decode('utf-8'),
            "content": content_buffer.value.decode('utf-8')
        }
    
    def __del__(self):
        """清理资源"""
        if self.index_ptr:
            self.lib.ffi_index_free(self.index_ptr)
        if self.vector_index_ptr:
            self.lib.ffi_vector_index_free(self.vector_index_ptr)

# ============================================================================
# 混合搜索引擎
# ============================================================================

class FusionSearchEngine:
    """混合搜索引擎（Python调度层）"""
    
    def __init__(
        self,
        jsonl_file: str = "data/recipes_vector.jsonl",
        vector_file: str = "vectors.bin",
        lib_path: str = "./libfusion.so"
    ):
        """初始化搜索引擎"""
        print("初始化混合搜索引擎...")
        
        # 初始化API客户端
        self.api = EmbeddingAPI()
        
        # 初始化C库
        self.c_engine = CFusionSearch(lib_path)
        
        # 加载索引
        print(f"加载BM25索引: {jsonl_file}")
        self.c_engine.load_index(jsonl_file)
        
        print(f"加载向量索引: {vector_file}")
        self.c_engine.load_vector_index(vector_file)
        
        print("初始化完成！\n")
    
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
            use_bm25: 是否使用BM25
            use_vector: 是否使用向量检索
            bm25_weight: BM25权重
            vector_weight: 向量权重
        
        Returns:
            融合排序后的结果列表
        """
        print(f"🔍 搜索: {query}\n")
        
        bm25_results = []
        vector_results = []
        
        # 1. BM25搜索
        if use_bm25:
            print("📚 执行BM25搜索...")
            bm25_results = self.c_engine.bm25_search(query, top_k * 2)
            print(f"   返回 {len(bm25_results)} 个结果\n")
        
        # 2. 向量检索
        if use_vector:
            print("📡 调用API获取Query向量...")
            try:
                query_vector = self.api.get_embedding(query)
                print(f"   ✓ 获取向量（{len(query_vector)}维）\n")
                
                print("🎯 执行向量检索...")
                vector_results = self.c_engine.vector_search(query_vector, top_k * 2)
                print(f"   返回 {len(vector_results)} 个结果\n")
            
            except Exception as e:
                print(f"   ❌ 向量检索失败: {e}\n")
        
        # 3. 融合排序
        print("🔀 融合排序...")
        final_results = self._merge_results(
            bm25_results,
            vector_results,
            bm25_weight,
            vector_weight,
            top_k
        )
        print(f"   返回 {len(final_results)} 个最终结果\n")
        
        return final_results
    
    def _merge_results(
        self,
        bm25_results: List[Tuple[int, float]],
        vector_results: List[Tuple[int, float]],
        bm25_weight: float,
        vector_weight: float,
        top_k: int
    ) -> List[Dict]:
        """融合BM25和向量检索结果"""
        doc_scores = {}
        
        # 归一化BM25分数
        max_bm25 = max((score for _, score in bm25_results), default=1.0)
        for doc_id, score in bm25_results:
            doc_scores[doc_id] = {
                'bm25_score': score / max_bm25 if max_bm25 > 0 else 0,
                'vector_score': 0.0
            }
        
        # 添加向量分数
        for doc_id, score in vector_results:
            if doc_id in doc_scores:
                doc_scores[doc_id]['vector_score'] = score
            else:
                doc_scores[doc_id] = {
                    'bm25_score': 0.0,
                    'vector_score': score
                }
        
        # 计算最终分数并排序
        ranked = []
        for doc_id, scores in doc_scores.items():
            final_score = (
                bm25_weight * scores['bm25_score'] +
                vector_weight * scores['vector_score']
            )
            
            # 查询文档内容
            doc = self.c_engine.get_document(doc_id)
            
            ranked.append({
                'doc_id': doc_id,
                'title': doc['title'],
                'content': doc['content'][:200] + '...',
                'bm25_score': scores['bm25_score'],
                'vector_score': scores['vector_score'],
                'final_score': final_score
            })
        
        ranked.sort(key=lambda x: x['final_score'], reverse=True)
        
        return ranked[:top_k]

# ============================================================================
# 命令行接口
# ============================================================================

def main():
    """命令行入口"""
    print("=" * 70)
    print("混合搜索引擎 - Python调度层")
    print("=" * 70)
    print()
    
    # 初始化引擎
    try:
        engine = FusionSearchEngine()
    except Exception as e:
        print(f"❌ 初始化失败: {e}")
        return
    
    # 交互式搜索
    while True:
        try:
            query = input("\n请输入搜索内容（输入 'quit' 退出）: ").strip()
            
            if query.lower() in ['quit', 'exit', 'q']:
                print("\n👋 再见！")
                break
            
            if not query:
                continue
            
            # 执行搜索
            results = engine.search(query, top_k=5)
            
            # 显示结果
            print("=" * 70)
            print("📄 搜索结果")
            print("=" * 70)
            print()
            
            if not results:
                print("未找到相关结果")
            else:
                for i, result in enumerate(results, 1):
                    print(f"{i}. {result['title']}")
                    print(f"   Final: {result['final_score']:.4f} "
                          f"(BM25: {result['bm25_score']:.4f}, "
                          f"Vector: {result['vector_score']:.4f})")
                    print(f"   {result['content'][:100]}...")
                    print()
            
            print("=" * 70)
        
        except KeyboardInterrupt:
            print("\n\n👋 再见！")
            break
        except Exception as e:
            print(f"\n❌ 搜索出错: {e}")

if __name__ == "__main__":
    main()
