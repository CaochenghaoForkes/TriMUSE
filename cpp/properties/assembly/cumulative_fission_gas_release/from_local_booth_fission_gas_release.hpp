#pragma once

#include "properties/assembly/cumulative_fission_gas_release/cumulative_fission_gas_release_model_manager.hpp"

namespace trimuse
{

class FromLocalBoothFissionGasRelease final : public CumulativeFissionGasReleaseModel
{
public:
    explicit FromLocalBoothFissionGasRelease(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
