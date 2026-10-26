#pragma once

#include "properties/external/nuclide_flux/nuclide_flux_model_manager.hpp"
#include "properties/common/constant.hpp"

namespace trimuse
{

class ConstantNuclideFlux final : public NuclideFluxModel, private ConstantModel
{
public:
    explicit ConstantNuclideFlux(const ModelParameterMap& parameters, const std::string& value_type);

    StateValue evaluate(const PropertiesStatePoint& point) const override;
};

}
