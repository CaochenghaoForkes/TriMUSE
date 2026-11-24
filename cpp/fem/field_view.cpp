#include "fem/field_view.hpp"

namespace trimuse
{

FieldView::FieldView(const RegionFieldData* field)
    :
    field_(field)
{
}

StateValue FieldView::evaluate(int region_attribute, int parent_element, const mfem::IntegrationPoint& point) const // 给定原网格的区域编号、单元编号和参考积分点，统一取得分域场在该点的值 注意这个 point 是单元中的参考坐标
{
    const auto* field = &field_->field_data(region_attribute).mfem_grid_function();
    const int element = field_->local_element(parent_element);
    if (field->FESpace()->GetVDim() == 1)
    {
        return field->GetValue(element, point); // 标量场直接返回一个数值
    }
    mfem::Vector components; // 创建一个 MFEM 向量，用来接收各分量
    field->GetVectorValue(element, point, components); // 在该单元的参考积分点计算所有分量 注意这个 point 是单元中的参考坐标
    return VectorValue(components.GetData(), components.GetData() + components.Size()); // 用一段数据的起止指针构造 VectorValue
}

}
