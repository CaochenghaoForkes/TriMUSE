#pragma once

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSpecificHeat final : public SpecificHeatModel, private ConstantModel
{
public:
    explicit ConstantSpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
