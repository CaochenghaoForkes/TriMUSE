#pragma once

#include "properties/external/energy_per_fission/energy_per_fission_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantEnergyPerFission final : public EnergyPerFissionModel, private ConstantModel
{
public:
    explicit ConstantEnergyPerFission(const ModelParameterMap& parameters, const std::string& value_type);
    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
