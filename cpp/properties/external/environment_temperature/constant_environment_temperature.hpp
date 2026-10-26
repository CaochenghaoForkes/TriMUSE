#pragma once

#include "properties/external/environment_temperature/environment_temperature_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantEnvironmentTemperature final : public EnvironmentTemperatureModel, private ConstantModel
{
public:
    explicit ConstantEnvironmentTemperature(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
