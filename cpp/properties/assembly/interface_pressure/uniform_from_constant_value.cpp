#include "properties/assembly/interface_pressure/uniform_from_constant_value.hpp"

namespace trimuse
{

UniformInterfacePressure::UniformInterfacePressure(const ModelParameterMap& parameters)
    :
    pressure_(std::get<double>(parameters.at("constant_value")))
{
}

double UniformInterfacePressure::evaluate(const PropertiesStatePoint&) const
{
    return pressure_;
}

}
