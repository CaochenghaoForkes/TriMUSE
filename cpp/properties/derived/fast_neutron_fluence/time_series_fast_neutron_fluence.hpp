#pragma once

#include "properties/derived/fast_neutron_fluence/fast_neutron_fluence_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesFastNeutronFluence final : public FastNeutronFluenceModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesFastNeutronFluence(const ModelParameterMap& parameters);
    
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
