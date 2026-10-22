#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RadioactivityRobinValues
{
    double matrix_coefficient; // h_m
    double rhs_coefficient; // h_m c_infinity
};

class RadioactivityRobinBoundary
{
public:
    static RadioactivityRobinValues evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute);
};

}
