#pragma once

#include "properties/external/nuclide_flux/nuclide_flux_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesNuclideFlux final : public NuclideFluxModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesNuclideFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
