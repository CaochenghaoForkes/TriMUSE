#pragma once

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"

namespace trimuse
{

class ConstantSpecificHeat final : public SpecificHeatModel
{
public:
    explicit ConstantSpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double constant_value_;
};

}
