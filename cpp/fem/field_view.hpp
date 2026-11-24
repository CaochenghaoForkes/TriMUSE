#pragma once

#include "common/state_value.hpp"
#include "fem/region_field_data.hpp"

namespace trimuse
{

class FieldView // 统一访问分域场，内部完成区域选择、单元编号映射和标量/向量取值，不复制场数据
{
public:
    FieldView(const RegionFieldData* field);
    StateValue evaluate(int region_attribute, int parent_element, const mfem::IntegrationPoint& point) const; // 给定原网格的区域编号、单元编号和参考积分点，统一取得分域场在该点的值

private:
    const RegionFieldData* field_; // 分域场的只读引用
};

}
