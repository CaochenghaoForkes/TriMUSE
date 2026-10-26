#pragma once

#include "properties/external/burnup_fima_fraction/burnup_fima_fraction_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantBurnupFimaFraction final : public BurnupFimaFractionModel, private ConstantModel
{
public:
    explicit ConstantBurnupFimaFraction(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
