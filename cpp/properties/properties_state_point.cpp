#include "properties/properties_state_point.hpp"

namespace trimuse
{

double PropertiesStatePoint::scalar(const std::string& name) const
{
    return std::get<double>(values.at(name));
}

const VectorValue& PropertiesStatePoint::vector(const std::string& name) const
{
    return std::get<VectorValue>(values.at(name));
}

const Tensor2DValue& PropertiesStatePoint::tensor_2d(const std::string& name) const
{
    return std::get<Tensor2DValue>(values.at(name));
}

const Tensor3DValue& PropertiesStatePoint::tensor_3d(const std::string& name) const
{
    return std::get<Tensor3DValue>(values.at(name));
}

}
