#include "physics/mechanics/interface_pressure/single_face_interface_pressure.hpp"

namespace trimuse
{

SingleFaceInterfacePressure::SingleFaceInterfacePressure(const ModelParameterMap& parameters)
    :
    region_attribute_(static_cast<int>(std::get<double>(parameters.at("region"))))
{
}

std::vector<MechanicsInterfacePressureValue> SingleFaceInterfacePressure::evaluate(const PropertiesFieldData& properties, const MeshData::Interface& interface) const
{
    const int side = interface.sides[0].region_attribute == region_attribute_ ? 0 : 1;
    return {{side, std::get<double>(properties.interface_values.at("assembly.interface_pressure").at(interface.face).value())}}; // 正压力表示压向指定区域，法向分解由积分器完成
}

}
