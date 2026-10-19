#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class ThermalNeumannBoundary
{
public:
    // q_n = -k grad(T) · n, 外流为正; 弱形式右端系数为 -q_n。
    static double evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute);
};

}
