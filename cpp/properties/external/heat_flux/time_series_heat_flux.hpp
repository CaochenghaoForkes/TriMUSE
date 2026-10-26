#pragma once

#include "properties/external/heat_flux/heat_flux_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesHeatFlux final : public HeatFluxModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesHeatFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
