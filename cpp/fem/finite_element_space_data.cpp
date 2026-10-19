#include "fem/finite_element_space_data.hpp"

namespace trimuse
{

FiniteElementSpaceData::FiniteElementSpaceData(MeshData& mesh_data, int order, int vector_dimension)
    : mesh_data_(mesh_data),
      order_(order),
      vector_dimension_(vector_dimension)
{
    // 建立指定维数、指定阶次的 H1 有限元基函数(拉格朗日基函数)集合
    collection_ = std::make_unique<mfem::H1_FECollection>(order_, mesh_data_.dimension());

    // 在公共网格上建立各单元自由度, 共享关系和局部到总体映射
    space_ = std::make_unique<mfem::FiniteElementSpace>(&mesh_data_.mfem_mesh(), collection_.get(), vector_dimension_);
}

int FiniteElementSpaceData::order() const
{
    return order_;
}

int FiniteElementSpaceData::vector_dimension() const
{
    return vector_dimension_;
}

int FiniteElementSpaceData::scalar_dof_count() const
{
    return space_->GetNDofs();
}

int FiniteElementSpaceData::dof_count() const
{
    return space_->GetVSize();
}

int FiniteElementSpaceData::true_dof_count() const
{
    return space_->GetTrueVSize();
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
