#pragma once

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantMassDensity final : public MassDensityModel, private ConstantModel
{
public:
    explicit ConstantMassDensity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
