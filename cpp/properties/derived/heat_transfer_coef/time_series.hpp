#pragma once

#include <vector>

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"

namespace trimuse
{

class TimeSeriesHeatTransferCoef final : public HeatTransferCoefModel
{
public:
    explicit TimeSeriesHeatTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;

private:
    std::vector<double> times_;
    std::vector<double> values_;
};

}
