#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

struct RobinValues
{
    double matrix_coefficient; // h
    double rhs_coefficient; // h 乘环境温度
};

class ThermalRobinBoundary
{
public:
    static RobinValues evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute);
};

}
