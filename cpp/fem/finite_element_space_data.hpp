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

    int order() const; // 插值阶次

    int vector_dimension() const; // 场变量分量数

    int scalar_dof_count() const;  // 标量有限元空间的自由度数量(有限元空间的插值节点数, 不是几何空间的)

    int dof_count() const; // 普通自由度（直接存储的全部自由度）：考虑所有场分量后的局部总体向量长度 scalar_dof_count * vector_dimension

    int true_dof_count() const; // 真实自由度（消除约束和重复表示后的真实自由度数量）：处理网格协调关系、悬挂节点关系和并行共享关系，得到独立的有限元空间自由度，还没施加边界 // free_dof（自由未知量）：N_free = N_true - N_essential
                                
    mfem::FiniteElementSpace& mfem_space();

    const mfem::FiniteElementSpace& mfem_space() const;

private:
    MeshData& mesh_data_;

    int order_;

    int vector_dimension_;

    std::unique_ptr<mfem::FiniteElementCollection> collection_;

    std::unique_ptr<mfem::FiniteElementSpace> space_;
};

}
