#pragma once

#include "common/model_parameters.hpp"
#include "properties/properties_field_data.hpp"

namespace trimuse
{

class MechanicsStiffnessModel
{
public:
    virtual ~MechanicsStiffnessModel() = default;

    virtual ElasticityTensorValue evaluate(const PropertiesFieldData& properties, std::size_t point) const = 0;
};

}
