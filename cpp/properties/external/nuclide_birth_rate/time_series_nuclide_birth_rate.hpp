#pragma once

#include "properties/external/nuclide_birth_rate/nuclide_birth_rate_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesNuclideBirthRate final : public NuclideBirthRateModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesNuclideBirthRate(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
