#pragma once

#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantPoissonRatio final : public PoissonRatioModel, private ConstantModel
{
public:
    explicit ConstantPoissonRatio(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
