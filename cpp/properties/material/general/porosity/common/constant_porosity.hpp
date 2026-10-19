#pragma once

#include "properties/material/general/porosity/porosity_model_manager.hpp"

namespace trimuse
{

class ConstantPorosity final : public PorosityModel
{
public:
    explicit ConstantPorosity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    double constant_value_;
};

}
