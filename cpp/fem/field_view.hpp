#pragma once

#include <variant>

#include "common/state_value.hpp"
#include "fem/region_field_data.hpp"

namespace trimuse
{

class FieldView // 统一访问连续场或分域场，内部完成区域选择、单元编号映射和标量/向量取值，不复制场数据
{
public:
    FieldView(const mfem::GridFunction* field);

    FieldView(const RegionFieldData* field);

    StateValue evaluate(int region_attribute, int parent_element, const mfem::IntegrationPoint& point) const; // 给定原网格的区域编号、单元编号和参考积分点，统一取得连续场或分域场在该点的值

private:
    std::variant<const mfem::GridFunction*, const RegionFieldData*> field_; // 连续场或分域场
};

}
