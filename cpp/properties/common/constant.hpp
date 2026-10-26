#pragma once

#include "common/model_parameters.hpp"
#include "common/state_value.hpp"
#include "properties/properties_state_point.hpp"

namespace trimuse
{

class ConstantModel
{
public:
    explicit ConstantModel(const ModelParameterMap& parameters, const std::string& value_type = "scalar"); // 从完整参数字典创建

    ConstantModel(const ModelParameterValue& value, const std::string& value_type); // 调用者已拿到数值 不需要再包装为字典 从单个参数值直接创建 value_type: [scalar,vector,tensor2d,tensor3d], 张量分量按行排列

    StateValue evaluate(const PropertiesStatePoint& point) const; // 返回常值

private:
    StateValue constant_value_; // 保存常值
}; 

}
