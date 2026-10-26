#pragma once

#include "properties/external/environment_nuclide_concentration/environment_nuclide_concentration_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantEnvironmentNuclideConcentration final : public EnvironmentNuclideConcentrationModel, private ConstantModel
{
public:
    explicit ConstantEnvironmentNuclideConcentration(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
