#include "physics/thermal/boundary/thermal_boundary_model.hpp"

namespace trimuse
{

double ThermalBoundaryModel::scalar(const PropertiesFieldData& properties, const std::string& name, int zone) // 得到属性表中名为 name 的区域属性在 zone 区域的值
{
    return std::get<double>(properties.zone_values.at(name).at(zone).value());
}

}
