#pragma once

#include "properties/external/fast_neutron_flux/fast_neutron_flux_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesFastNeutronFlux final : public FastNeutronFluxModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesFastNeutronFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
