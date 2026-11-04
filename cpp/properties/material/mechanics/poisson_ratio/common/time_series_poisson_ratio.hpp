#pragma once

#include "properties/material/mechanics/poisson_ratio/poisson_ratio_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesPoissonRatio final : public PoissonRatioModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesPoissonRatio(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
