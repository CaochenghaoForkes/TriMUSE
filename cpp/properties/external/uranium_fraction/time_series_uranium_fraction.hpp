#pragma once

#include "properties/external/uranium_fraction/uranium_fraction_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesUraniumFraction final : public UraniumFractionModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesUraniumFraction(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
