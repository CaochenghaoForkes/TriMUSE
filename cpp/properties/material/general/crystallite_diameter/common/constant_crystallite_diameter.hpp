#pragma once

#include "properties/material/general/crystallite_diameter/crystallite_diameter_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantCrystalliteDiameter final : public CrystalliteDiameterModel, private ConstantModel
{
public:
    explicit ConstantCrystalliteDiameter(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
