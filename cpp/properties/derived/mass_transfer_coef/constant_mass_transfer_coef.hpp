#pragma once

#include "properties/derived/mass_transfer_coef/mass_transfer_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantMassTransferCoef final : public MassTransferCoefModel, private ConstantModel
{
public:
    explicit ConstantMassTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
