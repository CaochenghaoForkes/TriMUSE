#pragma once

#include "properties/assembly/interface_pressure/interface_pressure_model_manager.hpp"

namespace trimuse
{

class UniformInterfacePressure final : public InterfacePressureProperty
{
public:
    explicit UniformInterfacePressure(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;

private:
    double pressure_;
};

}
