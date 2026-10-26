#include "properties/external/environment_nuclide_concentration/time_series_environment_nuclide_concentration.hpp"

namespace trimuse
{

TimeSeriesEnvironmentNuclideConcentration::TimeSeriesEnvironmentNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type)
    : TimeSeriesModel(parameters, value_type)
{
}

StateValue TimeSeriesEnvironmentNuclideConcentration::evaluate(const PropertiesStatePoint& point) const
{
    return TimeSeriesModel::evaluate(point);
}

}
