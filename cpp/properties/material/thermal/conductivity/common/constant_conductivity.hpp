#pragma once

#include "properties/material/thermal/conductivity/conductivity_model_manager.hpp"

namespace trimuse
{

class ConstantConductivity final : public ConductivityModel
{
public:
    explicit ConstantConductivity(const ModelParameterMap& parameters);
    
    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double constant_value_;
};

}
