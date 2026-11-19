#include "physics/mechanics/interface_pressure/double_face_interface_pressure.hpp"

namespace trimuse
{

DoubleFaceInterfacePressure::DoubleFaceInterfacePressure(const ModelParameterMap&)
{
}

std::vector<MechanicsInterfacePressureValue> DoubleFaceInterfacePressure::evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const
{
    const double pressure = std::get<double>(properties.interface_values.at("assembly.interface_pressure").at(interface.face).value());
    return {{0, pressure}, {1, pressure}}; // 同一压力分别作用于两侧，各侧使用自己的外法向
}

}
