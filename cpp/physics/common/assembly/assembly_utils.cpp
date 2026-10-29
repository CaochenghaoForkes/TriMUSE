#include "physics/common/assembly/assembly_utils.hpp"

namespace trimuse
{

std::shared_ptr<mfem::SparseMatrix> sum_matrices(const std::map<int, std::shared_ptr<const mfem::SparseMatrix>>& parts, int size) // 把各区域或边界已经组装好的矩阵相加，生成一张新的整体矩阵 parts: 区域或边界编号 -> 该部分组装贡献的矩阵 size: 整体矩阵的行列数
{
    auto sum = std::make_shared<mfem::SparseMatrix>(size); // 创建 size × size 的稀疏矩阵，并由 shared_ptr(共享所有权智能指针) 管理
    for (const auto& [region, matrix] : parts) // 遍历各部分矩阵
    {
        const auto* rows = matrix->GetI(); // 每行数据在存储数组中的起止偏移 长度为 size+1
        const auto* columns = matrix->GetJ(); // 每个已存储条目的列编号
        const auto* data = matrix->GetData(); // 每个已存储条目的数值
        // CSR 稀疏矩阵格式
        /*
                        2 0 5
                    A = 0 3 0
                        4 0 6
            data    = {2, 5, 3, 4, 6}; // 已存储条目的数值
            columns = {0, 2, 1, 0, 2}; // 各条目的列编号
            rows    = {0, 2, 3, 5};    // 各行的起始偏移，末项是条目总数
        */
        for (int row = 0; row < size; ++row)
        for (int j = rows[row]; j < rows[row + 1]; ++j) // j 是 CSR 数据数组下标
        { 
            sum->Add(row, columns[j], data[j]); 
        }
    }
    sum->Finalize(); // 将新矩阵整理为 CSR 格式，供后续矩阵运算使用
    return sum;
}

std::shared_ptr<mfem::Vector> sum_vectors(const std::map<int, std::shared_ptr<const mfem::Vector>>& parts, int size) // 将各区域或边界的向量贡献相加，返回一个新的整体向量
{
    auto sum = std::make_shared<mfem::Vector>(size); // 创建长度为 size 的向量，由共享智能指针管理
    *sum = 0.0; // 全部分量置零
    for (const auto& [region, vector] : parts) 
    { 
        *sum += *vector;  // 逐个取出贡献向量，解引用后按分量相加
    }
    return sum;
}

}
