#pragma once

#include "common/model_parameters.hpp"

namespace trimuse
{

class InterfaceContext;
class DofConstraints;

struct MechanicsInterfaceContactConfig // 材料界面的物理标识、模型名称及构造参数
{
    int boundary_attribute;
    std::string model_name = "perfect_contact";
    ModelParameterMap parameters;
};

class MechanicsInterfaceContactModel // 每个模型处理一块界面面片，登记两侧位移的约束关系
{
public:
    virtual ~MechanicsInterfaceContactModel() = default;
    virtual void assemble(const InterfaceContext& context, DofConstraints& constraints) const = 0;
};

}
