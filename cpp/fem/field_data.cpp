#include "fem/field_data.hpp"

namespace trimuse
{

FieldData::FieldData(FiniteElementSpaceData& finite_element_space, double initial_value)
    : finite_element_space_(finite_element_space),
      grid_function_(&finite_element_space_.mfem_space()) // grid_function 存储整个有限元空间中全部自由度值的向量(不仅包含未知节点,还包含施加Dirichlet条件的已知边界自由度)，同时它还关联着对应的有限元空间；它不只是普通向量：结合有限元空间，它能在单元内计算场值
{
    assign(initial_value);
}

int FieldData::size() const
{
    return grid_function_.Size(); // 返回整个场自由度向量的长度 包含已知的 Dirichlet 边界自由度和待求解的未知自由度 对于向量场，包含所有分量的自由度
}

void FieldData::assign(double value)
{
    grid_function_ = value; // 给全部自由度赋予一个相同的值
}

double FieldData::value(int index) const
{
    return grid_function_[index]; // 是根据自由度索引，读取该自由度对应的值(节点上的温度或浓度或应力或应变)
}

void FieldData::set_value(int index, double value)
{
    grid_function_[index] = value; // 是根据自由度索引，设置该自由度对应的值(节点上的温度或浓度或应力或应变)
}

mfem::GridFunction& FieldData::mfem_grid_function()
{
    return grid_function_;
}

const mfem::GridFunction& FieldData::mfem_grid_function() const
{
    return grid_function_;
}

}
