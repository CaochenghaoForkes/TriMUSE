#include "physics/radioactivity/boundary/radioactivity_boundary_model.hpp"

namespace trimuse
{

double RadioactivityBoundaryModel::scalar(const PropertiesFieldData& properties, const std::string& name, int zone) // 读取该边界分区上的标量属性
{
    return std::get<double>(properties.zone_values.at(name).at(zone).value());
}

}
