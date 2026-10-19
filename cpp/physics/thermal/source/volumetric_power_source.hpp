#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class VolumetricPowerSource
{
public:
    static double evaluate(const PropertiesFieldData& properties_field_data, const PropertiesFieldData::IntegrationPointKey& key);
};

}
