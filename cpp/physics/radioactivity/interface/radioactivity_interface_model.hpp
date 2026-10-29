#pragma once

#include "common/model_parameters.hpp"
#include "physics/common/interface/interface_context.hpp"
#include "physics/common/constraint/dof_constraints.hpp"

namespace trimuse
{

struct RadioactivityInterfaceConfig
{
    int boundary_attribute;
    std::string model_name = "perfect_contact";
    ModelParameterMap parameters;
};

class RadioactivityInterfaceModel
{
public:
    virtual ~RadioactivityInterfaceModel() = default;
    virtual void assemble(const InterfaceContext& context, DofConstraints& constraints) const = 0;
};

}
