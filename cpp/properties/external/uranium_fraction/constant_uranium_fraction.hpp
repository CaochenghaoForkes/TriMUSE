#pragma once

#include "properties/external/uranium_fraction/uranium_fraction_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantUraniumFraction final : public UraniumFractionModel, private ConstantModel
{
public:
    explicit ConstantUraniumFraction(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
