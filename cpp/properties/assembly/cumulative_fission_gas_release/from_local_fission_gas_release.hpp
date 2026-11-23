#pragma once

#include "properties/assembly/cumulative_fission_gas_release/cumulative_fission_gas_release_model_manager.hpp"

namespace trimuse
{

class FromLocalFissionGasRelease final : public CumulativeFissionGasReleaseModel
{
public:
    explicit FromLocalFissionGasRelease(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
