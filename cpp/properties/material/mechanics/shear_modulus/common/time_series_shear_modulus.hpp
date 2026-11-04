#pragma once

#include "properties/material/mechanics/shear_modulus/shear_modulus_model_manager.hpp"
#include "properties/common/time_series.hpp"

namespace trimuse
{

class TimeSeriesShearModulus final : public ShearModulusModel, private TimeSeriesModel
{
public:
    explicit TimeSeriesShearModulus(const ModelParameterMap& parameters);

    double evaluate(const PropertiesStatePoint& point) const override;
};

}
