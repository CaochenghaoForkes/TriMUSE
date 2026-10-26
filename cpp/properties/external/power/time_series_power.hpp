#pragma once

#include "properties/external/power/power_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesPower final : public PowerModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesPower(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
