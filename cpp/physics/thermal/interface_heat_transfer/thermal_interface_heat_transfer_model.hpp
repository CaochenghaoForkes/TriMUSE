#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

class InterfaceContext; // 前向声明 cpp/physics/common/interface/interface_context.hpp

struct ThermalInterfaceConfig // 界面模型配置: 哪个材料界面(boundary_attribute) 使用什么模型(model_name)，以及模型参数(parameters)
{
    int boundary_attribute;
    std::string model_name = "perfect_contact";
    ModelParameterMap parameters;
};

class ThermalInterfaceModel // 所有热界面模型的抽象基类
{
public:
    virtual ~ThermalInterfaceModel() = default; // 虚析构函数
    virtual bool is_perfect_contact(const InterfaceContext& context) const = 0;
};

}
