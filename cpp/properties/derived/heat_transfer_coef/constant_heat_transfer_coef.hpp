#pragma once

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantHeatTransferCoef final : public HeatTransferCoefModel, private ConstantModel // 多重继承 属于“换热系数模型”，可以通过这个基类的指针统一管理和调用 内部复用公共常数模型的参数处理和存储逻辑，但不向外暴露这层继承关系 
{
public:
    explicit ConstantHeatTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
