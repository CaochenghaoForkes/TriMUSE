#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

class ThermalInterfaceContext; // 前向声明 cpp/physics/thermal/interface/thermal_interface_context.hpp
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

    virtual void assemble(const ThermalInterfaceContext& context, ThermalInterfaceAssembly& assembly) const = 0; // 具体模型必须实现的操作 context 提供当前界面片、两侧场、几何变换及积分点取值接口 assembly 接收模型给出的自由度连接，最终建立约束矩阵 P 
};

}
