#pragma once

#include "properties/external/nuclide_birth_rate/nuclide_birth_rate_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantNuclideBirthRate final : public NuclideBirthRateModel, private ConstantModel
{
public:
    explicit ConstantNuclideBirthRate(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
