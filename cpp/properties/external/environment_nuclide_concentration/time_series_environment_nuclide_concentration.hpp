#pragma once

#include "properties/external/environment_nuclide_concentration/environment_nuclide_concentration_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesEnvironmentNuclideConcentration final : public EnvironmentNuclideConcentrationModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesEnvironmentNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
