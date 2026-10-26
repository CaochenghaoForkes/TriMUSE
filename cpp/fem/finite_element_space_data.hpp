#pragma once

#include <memory>
#include <mfem.hpp>

#include "mesh_data.hpp"

namespace trimuse
{

class FiniteElementSpaceData // 选择单元内基函数 把局部空间连接成总体空间 建立自由度及其向总体矩阵的映射
{
public:
    FiniteElementSpaceData(MeshData& mesh_data, int order, int vector_dimension = 1);

    int order() const; // 物理场的有限元插值阶次

    int vector_dimension() const; // 场变量的分量数 如温度是1 应变是空间维度数

    int scalar_dof_count() const;  // 全网格上的标量插值节点数；相邻单元共享的节点只计一次

    int dof_count() const; // 普通自由度（直接存储的全部自由度）：考虑所有场分量后的总体向量长度 scalar_dof_count * vector_dimension

    int true_dof_count() const; // 真实自由度（消除约束和重复表示后的真实自由度数量）：处理网格协调关系、悬挂节点关系和并行共享关系，得到独立的有限元空间自由度，还没施加边界
                                
    mfem::FiniteElementSpace& mfem_space();

    const mfem::FiniteElementSpace& mfem_space() const;

private:
    MeshData& mesh_data_;

    int order_; // 物理场的有限元插值阶次

    int vector_dimension_; // 场变量的分量数 如温度是1 应变是空间维度数

    std::unique_ptr<mfem::FiniteElementCollection> collection_; // 创建一套指定插值阶次、指定单元维数的 H1 协调有限元集合

    std::unique_ptr<mfem::FiniteElementSpace> space_; // 把网格、有限元集合和场的分量数结合起来，建立真正的有限元空间，并安排自由度编号
};

}
