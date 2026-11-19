#pragma once

#include "properties/material/general/bacon_anisotropy_factor/bacon_anisotropy_factor_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesBaconAnisotropyFactor final : public BaconAnisotropyFactorModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesBaconAnisotropyFactor(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
