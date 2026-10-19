#pragma once

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"

namespace trimuse
{

class ConstantMassDensity final : public MassDensityModel
{
public:
    explicit ConstantMassDensity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double constant_value_;
};

}
