#pragma once

#include "properties/assembly/cumulative_co_release/cumulative_co_release_model_manager.hpp"

namespace trimuse
{

class FromLocalPanamaOpf final : public CumulativeCoReleaseModel
{
public:
    explicit FromLocalPanamaOpf(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;
};

}
