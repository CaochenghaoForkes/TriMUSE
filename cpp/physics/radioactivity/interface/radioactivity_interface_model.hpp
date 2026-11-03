#pragma once

#include "common/model_parameters.hpp"
#include "physics/common/interface/interface_context.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

struct RadioactivityInterfaceConfig // 一个内部材料界面的模型配置
{
    int boundary_attribute; // 原网格中的界面边界属性编号
    std::string model_name = "perfect_contact"; // 未指定时采用完美接触
    ModelParameterMap parameters; // 模型构造参数
};

class RadioactivityInterfaceModel // 每个模型处理一片双侧界面，并登记它要求的自由度约束
{
public:
    virtual ~RadioactivityInterfaceModel() = default;
    virtual void assemble(const InterfaceContext& context, DofConstraints& constraints) const = 0; // context 提供原网格界面及两侧信息
};

}
