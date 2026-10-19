#pragma once

#include <string>
#include <unordered_map>

#include "common/state_value.hpp"

namespace trimuse
{

// 当前计算点与已取好的数值, 求解场只在组装此临时状态时读取
struct PropertiesStatePoint
{
    int region_attribute = 0;
    int zone_attribute = 0; // 边界物理组编号，与材料区域编号分别使用
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    std::unordered_map<std::string, StateValue> values;

    double scalar(const std::string& name) const;

    const VectorValue& vector(const std::string& name) const;

    const Tensor2DValue& tensor_2d(const std::string& name) const;

    const Tensor3DValue& tensor_3d(const std::string& name) const;
};

}
