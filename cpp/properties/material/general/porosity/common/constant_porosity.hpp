#pragma once

#include "properties/material/general/porosity/porosity_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantPorosity final : public PorosityModel, private ConstantModel
{
public:
    explicit ConstantPorosity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
