#pragma once

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantHeatTransferCoef final : public HeatTransferCoefModel, private ConstantModel
{
public:
    explicit ConstantHeatTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
