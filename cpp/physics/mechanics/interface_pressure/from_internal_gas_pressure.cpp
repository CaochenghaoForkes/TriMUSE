#include "physics/mechanics/interface_pressure/from_internal_gas_pressure.hpp"

namespace trimuse
{

MechanicsFromInternalGasPressure::MechanicsFromInternalGasPressure(const ModelParameterMap& parameters)
    :
    gas_region_(static_cast<int>(std::get<double>(parameters.at("gas_region"))))
{
}

MechanicsInterfacePressureValue MechanicsFromInternalGasPressure::evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const
{
    const int side = interface.sides[0].region_attribute == gas_region_ ? 1 : 0; // 选取非储气区域的受压侧
    const double pressure = std::get<double>(properties.region_values.at("assembly.internal_gas_pressure").at(gas_region_).value());
    return {side, pressure};
}

}
