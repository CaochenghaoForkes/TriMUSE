#pragma once
#include "physics/radioactivity/common/radioactivity_domain_model.hpp"

namespace trimuse
{

class UnitCapacity final : public RadioactivityDomainModel
{
public:
    explicit UnitCapacity(const ModelParameterMap&) {}

    double evaluate(const PropertiesFieldData&, std::size_t) const override
    {
        return 1.0;
    }
};

}
