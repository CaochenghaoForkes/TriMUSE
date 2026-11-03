#pragma once

#include <map>
#include <memory>

namespace trimuse
{

template<class Value> 
struct AssemblyCache // Value 可以是 mfem::SparseMatrix 或 mfem::Vector 按 Region/Zone 保存只读组装贡献；更新时替换指针，保留旧状态快照 parts：按区域编号或边界分区编号保存各自的组装贡献 sum：这些贡献相加后的完整矩阵或向量
{
    std::map<int, std::shared_ptr<const Value>> parts; // 区域名 -> 该区域缓存的整体矩阵/整体向量贡献
    std::shared_ptr<const Value> sum; // 全部区域贡献相加后的完整矩阵或向量
};

}
