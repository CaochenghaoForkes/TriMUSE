#include "fem/field_view.hpp"

namespace trimuse
{

FieldView::FieldView(const mfem::GridFunction* field) : field_(field)
{
}

FieldView::FieldView(const RegionFieldData* field) : field_(field)
{
}

StateValue FieldView::evaluate(int region_attribute, int parent_element, const mfem::IntegrationPoint& point) const // 给定原网格的区域编号、单元编号和参考积分点，统一取得连续场或分域场在该点的值
{
    const mfem::GridFunction* field; // 稍后指向实际需要读取的 GridFunction
    int element = parent_element; // 该 GridFunction 所在网格中的单元编号
    if (const auto* region = std::get_if<const RegionFieldData*>(&field_)) // 判断保存的是不是分域场
    {
        field = &(*region)->field_data(region_attribute).mfem_grid_function(); // 在分域场中选择 region_attribute 对应的区域场数据 取得该区域的 GridFunction 引用的地址保存到 field
        element = (*region)->local_element(parent_element); // 把原网格单元编号转换成该区域子网格中的局部单元编号
    }
    else
    {
        field = std::get<const mfem::GridFunction*>(field_); // 连续场则直接取出场指针
    }
    if (field->FESpace()->GetVDim() == 1)
    {
        return field->GetValue(element, point); // 标量场直接返回一个数值
    }
    mfem::Vector components; // 创建一个 MFEM 向量，用来接收各分量
    field->GetVectorValue(element, point, components); // 在该单元的参考积分点计算所有分量 注意这个 point 是单元中的参考坐标
    return VectorValue(components.GetData(), components.GetData() + components.Size()); // 用一段数据的起止指针构造 VectorValue
}

}
