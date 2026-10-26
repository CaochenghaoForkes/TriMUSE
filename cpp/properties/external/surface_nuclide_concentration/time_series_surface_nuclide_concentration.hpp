#pragma once

#include "properties/external/surface_nuclide_concentration/surface_nuclide_concentration_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesSurfaceNuclideConcentration final : public SurfaceNuclideConcentrationModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesSurfaceNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
