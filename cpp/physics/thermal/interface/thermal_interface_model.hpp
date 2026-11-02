#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

class InterfaceContext; // 前向声明 cpp/physics/common/interface/interface_context.hpp
class ThermalInterfaceAssembly; // 前向声明 cpp/physics/thermal/interface/thermal_interface_assembly.hpp

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
    
    virtual bool requires_region_field() const { return true; } // 判断该模型是否要求分域场，默认要求

    virtual void assemble(const InterfaceContext& context, ThermalInterfaceAssembly& assembly) const = 0; // context 提供界面和双侧数据; assembly 汇集模型给出的自由度约束或传热积分贡献
};

}
