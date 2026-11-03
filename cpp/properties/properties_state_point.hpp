#pragma once

#include <string>
#include <unordered_map>

#include "common/state_value.hpp"

namespace trimuse
{

// 当前计算点与已取好的数值; 属性键使用完整名称, 求解场仍使用 temperature/concentration 等场名
// 求解场只在组装此临时状态时读取
struct PropertiesStatePoint
{
    int region_attribute = 0;
    int zone_attribute = 0; // 边界物理组编号，与材料区域编号分别使用
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::unordered_map<std::string, StateValue> values; // 属性名或场名 -> 单时空点上的 <double/vector/2d_tensor/3d_tensor>

    double scalar(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 double

    const VectorValue& vector(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 vector

    const Tensor2DValue& tensor_2d(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 Tensor2DValue

    const Tensor3DValue& tensor_3d(const std::string& name) const; // 先根据 name 找到对应的 variant 型的 StateValue 再从这个 variant 中期望取出其中存储的 Tensor3DValue
};

}
