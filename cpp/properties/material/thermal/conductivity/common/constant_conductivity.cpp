#include "properties/material/thermal/conductivity/common/constant_conductivity.hpp"

namespace trimuse
{

ConstantConductivity::ConstantConductivity(const ModelParameterMap& parameters)
    : constant_value_(std::get<double>(parameters.at("constant_value")))
{
}

double ConstantConductivity::evaluate(const PropertiesStatePoint&) const
{
    return constant_value_;
}

}
