#pragma once

#include "properties/external/power/power_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantPower final : public PowerModel, private ConstantModel
{
public:
    explicit ConstantPower(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
