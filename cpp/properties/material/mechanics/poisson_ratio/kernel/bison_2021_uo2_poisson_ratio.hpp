#pragma once

#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"

namespace trimuse
{

// UO2 各向同性泊松比，返回 [nu_rt, nu_tt]
class Bison2021Uo2PoissonRatio final : public PoissonRatioModel
{
public:
    explicit Bison2021Uo2PoissonRatio(const ModelParameterMap& parameters);
    VectorValue evaluate(const PropertiesStatePoint& state_point) const override;
};

}
