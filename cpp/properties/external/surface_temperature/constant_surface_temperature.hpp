#pragma once

#include "properties/external/surface_temperature/surface_temperature_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantSurfaceTemperature final : public SurfaceTemperatureModel, private ConstantModel
{
public:
    explicit ConstantSurfaceTemperature(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
