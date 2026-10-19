#pragma once

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"

namespace trimuse
{

class ConstantHeatTransferCoef final : public HeatTransferCoefModel
{
public:
    explicit ConstantHeatTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;

private:
    double value_;
};

}
