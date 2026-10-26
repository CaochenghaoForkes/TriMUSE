#pragma once

#include "properties/derived/mass_transfer_coef/mass_transfer_coef_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesMassTransferCoef final : public MassTransferCoefModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesMassTransferCoef(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
