#include "properties/material/thermal/specific_heat/common/constant_specific_heat.hpp"

namespace trimuse
{

ConstantSpecificHeat::ConstantSpecificHeat(const ModelParameterMap& parameters)
    : constant_value_(std::get<double>(parameters.at("constant_value")))
{
}

double ConstantSpecificHeat::evaluate(const PropertiesStatePoint&) const
{
    return constant_value_;
}

}
