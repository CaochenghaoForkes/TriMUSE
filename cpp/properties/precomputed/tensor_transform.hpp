#pragma once

#include "fem/fem_system.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

// 对称二阶张量的六分量顺序为 [11,22,33,23,13,12]，剪应变使用张量分量
void initialize_tensor_transform(PropertiesFieldData& properties, const FemSystem& fem);

}
