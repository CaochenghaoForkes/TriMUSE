#pragma once

#include "properties/material/general/porosity/porosity_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesPorosity final : public PorosityModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesPorosity(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
