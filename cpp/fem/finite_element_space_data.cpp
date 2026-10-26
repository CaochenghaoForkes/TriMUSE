#include "fem/finite_element_space_data.hpp"

namespace trimuse
{

FiniteElementSpaceData::FiniteElementSpaceData(MeshData& mesh_data, int order, int vector_dimension)
    : FiniteElementSpaceData(mesh_data.mfem_mesh(), order, vector_dimension)
{
}

FiniteElementSpaceData::FiniteElementSpaceData(mfem::Mesh& mesh, int order, int vector_dimension)
    : mesh_(mesh),
      order_(order),
      vector_dimension_(vector_dimension)
{
    // std::make_unique<...> 创建某个类型的对象并返回它的智能指针

    // 创建一套指定插值阶次、指定单元维数的 H1 协调有限元集合
    // H1: 一个函数空间的名字 
    // H1 函数本身及其一阶弱导数都平方可积 \[H^1(\Omega)=\left\{u:\ u\in L^2(\Omega),\\frac{\partial u}{\partial x_i}\in L^2(\Omega)\right\}\]
    // H1 表现为: 1.每个单元内部用多项式插值 2.相邻单元共享界面上的场值，保证整体连续 3.不要求梯度跨单元连续
    collection_ = std::make_unique<mfem::H1_FECollection>(order_, mesh_.Dimension());

    // 把网格、有限元集合和场的分量数结合起来，建立真正的有限元空间，并安排自由度编号
    // 参数1在哪张网格上建立空间(有哪些单元、单元间如何连接、哪些顶点边面由相邻单元共享) 参数2使用哪种有限元(对线段、三角形或四面体，采样什么阶次的有限元) 参数3场有几个分量
    space_ = std::make_unique<mfem::FiniteElementSpace>(&mesh_, collection_.get(), vector_dimension_);
}

int FiniteElementSpaceData::order() const
{
    return order_; // 物理场的有限元插值阶次
}

int FiniteElementSpaceData::vector_dimension() const
{
    return vector_dimension_; // 场变量的分量数 如温度是1 应变是空间维度数
}

int FiniteElementSpaceData::scalar_dof_count() const
{
    return space_->GetNDofs(); // 全网格上的标量插值节点数；相邻单元共享的节点只计一次
}

int FiniteElementSpaceData::dof_count() const
{
    return space_->GetVSize(); // 普通自由度（直接存储的全部自由度）：考虑所有场分量后的总体向量长度 scalar_dof_count * vector_dimension
}

int FiniteElementSpaceData::true_dof_count() const
{
    return space_->GetTrueVSize(); // 真实自由度（消除约束和重复表示后的真实自由度数量）：处理网格协调关系、悬挂节点关系和并行共享关系，得到独立的有限元空间自由度，还没施加边界
}

mfem::FiniteElementSpace& FiniteElementSpaceData::mfem_space()
{
    return *space_;
}

const mfem::FiniteElementSpace& FiniteElementSpaceData::mfem_space() const
{
    return *space_;
}

}
