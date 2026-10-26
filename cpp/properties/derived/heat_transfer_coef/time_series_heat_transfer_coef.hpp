#pragma once

#include "properties/derived/heat_transfer_coef/heat_transfer_coef_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesHeatTransferCoef final : public HeatTransferCoefModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesHeatTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
