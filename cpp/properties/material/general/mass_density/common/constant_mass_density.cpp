#include "properties/material/general/mass_density/common/constant_mass_density.hpp"

namespace trimuse
{

ConstantMassDensity::ConstantMassDensity(const ModelParameterMap& parameters)
    : constant_value_(std::get<double>(parameters.at("constant_value")))
{
}

double ConstantMassDensity::evaluate(const PropertiesStatePoint&) const
{
    return constant_value_;
}

}
