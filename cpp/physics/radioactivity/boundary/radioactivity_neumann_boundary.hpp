#pragma once

#include "properties/properties_field_data.hpp"

namespace trimuse
{

class RadioactivityNeumannBoundary
{
public:
    // j_n = -D grad(c) · n, 外流为正; 弱形式右端系数为 -j_n。
    static double evaluate(const PropertiesFieldData& properties_field_data, int boundary_attribute);
};

}
