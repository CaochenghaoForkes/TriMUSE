#pragma once

#include <vector>

#include "properties/material/thermal/specific_heat/specific_heat_model_manager.hpp"

namespace trimuse
{

class TimeSeriesSpecificHeat final : public SpecificHeatModel
{
public:
    explicit TimeSeriesSpecificHeat(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& state_point) const override;

private:
    std::vector<double> times_;
    std::vector<double> values_;
};

}
