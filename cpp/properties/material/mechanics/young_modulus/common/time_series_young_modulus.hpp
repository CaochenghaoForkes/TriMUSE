#pragma once

#include "properties/material/mechanics/young_modulus/young_modulus_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesYoungModulus final : public YoungModulusModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesYoungModulus(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
