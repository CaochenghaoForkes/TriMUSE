#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class ThermalDirichletBoundary
{
public:
    static double evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute);
};

}
