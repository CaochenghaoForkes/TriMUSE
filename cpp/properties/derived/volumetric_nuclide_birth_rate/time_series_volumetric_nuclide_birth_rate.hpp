#pragma once

#include "properties/derived/volumetric_nuclide_birth_rate/volumetric_nuclide_birth_rate_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesVolumetricNuclideBirthRate final : public VolumetricNuclideBirthRateModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesVolumetricNuclideBirthRate(const ModelParameterMap& parameters);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
