#include "properties/material/general/porosity/common/constant_porosity.hpp"

namespace trimuse
{

ConstantPorosity::ConstantPorosity(const ModelParameterMap& parameters)
    : constant_value_(std::get<double>(parameters.at("constant_value")))
{
}

double ConstantPorosity::evaluate(const PropertiesStatePoint&) const
{
    return constant_value_;
}

}
