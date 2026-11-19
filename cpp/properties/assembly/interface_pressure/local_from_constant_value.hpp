#pragma once

#include "common/angular_range.hpp"
#include "properties/assembly/interface_pressure/interface_pressure_model_manager.hpp"

namespace trimuse
{

class LocalInterfacePressure final : public InterfacePressureProperty
{
public:
    explicit LocalInterfacePressure(const ModelParameterMap& parameters);
    double evaluate(const PropertiesStatePoint& point) const override;

private:
    AngularRanges angles_;
    std::vector<double> pressures_;
};

}
