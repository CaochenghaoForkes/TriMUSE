#pragma once

#include <vector>

#include "properties/material/general/mass_density/mass_density_model_manager.hpp"

namespace trimuse
{

class TimeSeriesMassDensity final : public MassDensityModel
{
public:
    explicit TimeSeriesMassDensity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    std::vector<double> times_;
    std::vector<double> values_;
};

}
