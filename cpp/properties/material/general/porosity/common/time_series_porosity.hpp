#pragma once

#include <vector>

#include "properties/material/general/porosity/porosity_model_manager.hpp"

namespace trimuse
{

class TimeSeriesPorosity final : public PorosityModel
{
public:
    explicit TimeSeriesPorosity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    std::vector<double> times_;
    std::vector<double> values_;
};

}
