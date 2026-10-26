#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantConductivity final : public ConductivityModel, private ConstantModel
{
public:
    explicit ConstantConductivity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
